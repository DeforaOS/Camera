/* $Id$ */
static char const _copyright[] =
"Copyright (c) 2012 Pierre Pronchery <khorben@defora.org>";
/* This file is part of DeforaOS Desktop Camera */
static char const _license[] =
"This program is free software: you can redistribute it and/or modify\n"
"it under the terms of the GNU General Public License as published by\n"
"the Free Software Foundation, version 3 of the License.\n"
"\n"
"This program is distributed in the hope that it will be useful,\n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
"GNU General Public License for more details.\n"
"\n"
"You should have received a copy of the GNU General Public License\n"
"along with this program.  If not, see <http://www.gnu.org/licenses/>.";



#include <sys/ioctl.h>
#ifdef __NetBSD__
# include <sys/videoio.h>
#else
# include <linux/videodev2.h>
#endif
#include <fcntl.h>
#include <unistd.h>
#ifdef DEBUG
# include <stdio.h>
#endif
#include <string.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <System.h>
#include <Desktop.h>
#include "camera.h"
#include "../config.h"


/* Camera */
/* private */
/* types */
struct _Camera
{
	String * device;
	int fd;
	guint source;
	Buffer * buffer;

	/* widgets */
	GtkWidget * window;
#if GTK_CHECK_VERSION(2, 18, 0)
	GtkWidget * infobar;
	GtkWidget * infobar_label;
#endif
	GtkWidget * area;
	GdkPixmap * pixmap;
};


/* prototypes */
static int _camera_error(Camera * camera, char const * message, int ret);

static int _camera_ioctl(Camera * camera, unsigned long request,
		void * data);

/* callbacks */
static gboolean _camera_on_can_read(gpointer data);
static gboolean _camera_on_closex(gpointer data);
static gboolean _camera_on_drawing_area_configure(GtkWidget * widget,
		GdkEventConfigure * event, gpointer data);
static gboolean _camera_on_drawing_area_expose(GtkWidget * widget,
		GdkEventExpose * event, gpointer data);
static gboolean _camera_on_open(gpointer data);
static gboolean _camera_on_refresh(gpointer data);

static void _camera_on_file_close(gpointer data);
static void _camera_on_help_about(gpointer data);


/* constants */
static char const * _authors[] =
{
	"Pierre Pronchery <khorben@defora.org>",
	NULL
};

static const DesktopMenu _camera_menu_file[] =
{
	{ "_Close", G_CALLBACK(_camera_on_file_close), GTK_STOCK_CLOSE, 0, 0 },
	{ NULL, NULL, NULL, 0, 0 }
};

static const DesktopMenu _camera_menu_help[] =
{
	{ "_About", G_CALLBACK(_camera_on_help_about), GTK_STOCK_ABOUT, 0, 0 },
	{ NULL, NULL, NULL, 0, 0 }
};

static const DesktopMenubar _camera_menubar[] =
{
	{ "_File", _camera_menu_file },
	{ "_Help", _camera_menu_help },
	{ NULL, NULL }
};


/* public */
/* functions */
/* camera_new */
Camera * camera_new(char const * device)
{
	Camera * camera;
	GtkAccelGroup * group;
	GtkWidget * vbox;
	GtkWidget * widget;

	if((camera = object_new(sizeof(*camera))) == NULL)
		return NULL;
	if(device == NULL)
		device = "/dev/video0";
	camera->device = string_new(device);
	camera->fd = -1;
	camera->buffer = NULL;
	camera->source = 0;
	camera->window = NULL;
	/* check for errors */
	if(camera->device == NULL)
	{
		camera_delete(camera);
		return NULL;
	}
	/* create the window */
	group = gtk_accel_group_new();
	camera->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_add_accel_group(GTK_WINDOW(camera->window), group);
#if GTK_CHECK_VERSION(2, 6, 0)
	gtk_window_set_icon_name(GTK_WINDOW(camera->window), "camera-video");
#endif
	gtk_window_set_resizable(GTK_WINDOW(camera->window), FALSE);
	gtk_window_set_title(GTK_WINDOW(camera->window), "Camera");
	g_signal_connect_swapped(camera->window, "delete-event", G_CALLBACK(
				_camera_on_closex), camera);
	vbox = gtk_vbox_new(FALSE, 0);
	/* menubar */
	widget = desktop_menubar_create(_camera_menubar, camera, group);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
#if GTK_CHECK_VERSION(2, 18, 0)
	/* errors */
	camera->infobar = gtk_info_bar_new_with_buttons(GTK_STOCK_CLOSE,
			GTK_RESPONSE_CLOSE, NULL);
	gtk_info_bar_set_message_type(GTK_INFO_BAR(camera->infobar),
			GTK_MESSAGE_ERROR);
	g_signal_connect(camera->infobar, "close", G_CALLBACK(gtk_widget_hide),
			NULL);
	g_signal_connect(camera->infobar, "response", G_CALLBACK(
				gtk_widget_hide), NULL);
	widget = gtk_info_bar_get_content_area(GTK_INFO_BAR(camera->infobar));
	camera->infobar_label = gtk_label_new(NULL);
	gtk_widget_show(camera->infobar_label);
	gtk_box_pack_start(GTK_BOX(widget), camera->infobar_label, TRUE, TRUE,
			0);
	gtk_widget_set_no_show_all(camera->infobar, TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), camera->infobar, FALSE, TRUE, 0);
#endif
	camera->area = gtk_drawing_area_new();
	camera->pixmap = NULL;
	g_signal_connect(camera->area, "configure-event", G_CALLBACK(
				_camera_on_drawing_area_configure), camera);
	g_signal_connect(camera->area, "expose-event", G_CALLBACK(
				_camera_on_drawing_area_expose), camera);
	gtk_box_pack_start(GTK_BOX(vbox), camera->area, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(camera->window), vbox);
	gtk_widget_show_all(camera->window);
	camera->source = g_idle_add(_camera_on_open, camera);
	return camera;
}


/* camera_delete */
void camera_delete(Camera * camera)
{
	if(camera->pixmap != NULL)
		g_object_unref(camera->pixmap);
	if(camera->window != NULL)
		gtk_widget_destroy(camera->window);
	if(camera->source != 0)
		g_source_remove(camera->source);
	if(camera->fd >= 0)
		close(camera->fd);
	buffer_delete(camera->buffer);
	string_delete(camera->device);
	object_delete(camera);
}


/* private */
/* functions */
/* camera_error */
static int _error_text(char const * message, int ret);

static int _camera_error(Camera * camera, char const * message, int ret)
{
#if !GTK_CHECK_VERSION(2, 18, 0)
	GtkWidget * dialog;
#endif

	if(camera == NULL)
		return _error_text(message, ret);
#if GTK_CHECK_VERSION(2, 18, 0)
	gtk_label_set_text(GTK_LABEL(camera->infobar_label), message);
	gtk_widget_show(camera->infobar);
#else
	dialog = gtk_message_dialog_new(GTK_WINDOW(camera->window),
			GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
			GTK_BUTTONS_CLOSE,
# if GTK_CHECK_VERSION(2, 6, 0)
			"%s", "Error");
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
# endif
			"%s", message);
	gtk_window_set_title(GTK_WINDOW(dialog), "Error");
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
#endif
	return ret;
}

static int _error_text(char const * message, int ret)
{
	fprintf(stderr, "%s: %s\n", PACKAGE, message);
	return ret;
}


/* camera_ioctl */
static int _camera_ioctl(Camera * camera, unsigned long request,
		void * data)
{
	int ret;

	for(;;)
		if((ret = ioctl(camera->fd, request, data)) != -1
				|| errno != EINTR)
			break;
	return ret;
}


/* callbacks */
/* camera_on_can_read */
static gboolean _camera_on_can_read(gpointer data)
{
	Camera * camera = data;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	camera->source = 0;
	/* FIXME no longer block on read() */
	if(read(camera->fd, buffer_get_data(camera->buffer),
				buffer_get_size(camera->buffer)) <= 0)
	{
		/* this error can be ignored */
		if(errno == EAGAIN)
			return TRUE;
		close(camera->fd);
		camera->fd = -1;
		_camera_error(camera, strerror(errno), 1);
		return FALSE;
	}
	camera->source = g_idle_add(_camera_on_refresh, camera);
	return FALSE;
}


/* camera_on_closex */
static gboolean _camera_on_closex(gpointer data)
{
	Camera * camera = data;

	gtk_widget_hide(camera->window);
	if(camera->source != 0)
		g_source_remove(camera->source);
	camera->source = 0;
	gtk_main_quit();
	return TRUE;
}


/* camera_on_drawing_area_configure */
static gboolean _camera_on_drawing_area_configure(GtkWidget * widget,
		GdkEventConfigure * event, gpointer data)
{
	/* XXX this code is inspired from GQcam */
	Camera * camera = data;
	GtkAllocation allocation;
	GdkGC * gc;
	GdkColor black = { 0, 0, 0, 0 };
	GdkColor white = { 0xffffffff, 0xffff, 0xffff, 0xffff };

	if(camera->pixmap != NULL)
		g_object_unref(camera->pixmap);
	/* FIXME requires Gtk+ 2.18 */
	gtk_widget_get_allocation(widget, &allocation);
	camera->pixmap = gdk_pixmap_new(widget->window, allocation.width,
			allocation.height, -1);
	/* FIXME this code is untested */
	gc = gdk_gc_new(widget->window);
	gdk_gc_set_background(gc, &white);
	gdk_gc_set_foreground(gc, &black);
	/* FIXME is it not better to scale the previous pixmap for now? */
	gdk_draw_rectangle(camera->pixmap, gc, TRUE, 0, 0, allocation.width,
			allocation.height);
	g_object_unref(gc);
	return TRUE;
}


/* camera_on_drawing_area_expose */
static gboolean _camera_on_drawing_area_expose(GtkWidget * widget,
		GdkEventExpose * event, gpointer data)
{
	/* XXX this code is inspired from GQcam */
	Camera * camera = data;
	GtkAllocation allocation;
	GdkGC * gc;

	/* FIXME requires Gtk+ 2.18 */
	gtk_widget_get_allocation(widget, &allocation);
	/* FIXME this code is untested */
	gc = gdk_gc_new(widget->window);
	gdk_draw_pixmap(widget->window, gc, camera->pixmap,
			event->area.x, event->area.y,
			event->area.x, event->area.y,
			event->area.width, event->area.height);
	g_object_unref(gc);
	return FALSE;
}


/* camera_on_file_close */
static void _camera_on_file_close(gpointer data)
{
	Camera * camera = data;

	_camera_on_closex(camera);
}


/* camera_on_help_about */
static void _camera_on_help_about(gpointer data)
{
	Camera * camera = data;
	GtkWidget * widget;

	widget = desktop_about_dialog_new();
	gtk_window_set_transient_for(GTK_WINDOW(widget), GTK_WINDOW(
				camera->window));
	desktop_about_dialog_set_authors(widget, _authors);
	desktop_about_dialog_set_comments(widget,
			"Simple camera application for the DeforaOS desktop");
	desktop_about_dialog_set_copyright(widget, _copyright);
	desktop_about_dialog_set_license(widget, _license);
	desktop_about_dialog_set_logo_icon_name(widget, "camera-video");
	desktop_about_dialog_set_name(widget, PACKAGE);
	desktop_about_dialog_set_version(widget, VERSION);
	gtk_dialog_run(GTK_DIALOG(widget));
	gtk_widget_destroy(widget);
}


/* camera_on_open */
static gboolean _camera_on_open(gpointer data)
{
	Camera * camera = data;
	/* XXX let this be configurable */
	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_format format;
	char buf[128];

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() \"%s\"\n", __func__, camera->device);
#endif
	camera->source = 0;
	camera->fd = open(camera->device, O_RDWR);
	camera->buffer = buffer_new(0, NULL);
	camera->source = 0;
	/* check for errors */
	if(camera->buffer == NULL || camera->fd < 0
			|| _camera_ioctl(camera, VIDIOC_QUERYCAP, &cap) == -1
			|| (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0
			/* FIXME also implement mmap() and streaming */
			|| (cap.capabilities & V4L2_CAP_READWRITE) == 0)
	{
		snprintf(buf, sizeof(buf), "%s: %s", camera->device,
				"Could not open the video capture device");
		_camera_error(camera, buf, 1);
		return FALSE;
	}
	/* reset cropping */
	memset(&cropcap, 0, sizeof(cropcap));
	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if(_camera_ioctl(camera, VIDIOC_CROPCAP, &cropcap) == 0)
	{
		/* reset to default */
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect;
		if(_camera_ioctl(camera, VIDIOC_S_CROP, &crop) == -1
				&& errno == EINVAL)
			error_set_code(1, "Cropping not supported");
	}
	/* obtain the current format */
	format.fmt.pix.width = 100;
	format.fmt.pix.height = 75;
	if(_camera_ioctl(camera, VIDIOC_G_FMT, &format) != -1)
		buffer_set_size(camera->buffer, format.fmt.pix.sizeimage);
	gtk_widget_set_size_request(camera->area, format.fmt.pix.width,
			format.fmt.pix.height);
	/* FIXME register only if can really be read */
	_camera_on_can_read(camera);
	return FALSE;
}


/* camera_on_refresh */
static gboolean _camera_on_refresh(gpointer data)
{
	Camera * camera = data;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	camera->source = 0;
	/* FIXME really implement */
	camera->source = g_timeout_add(1000, _camera_on_can_read, camera);
	return FALSE;
}
