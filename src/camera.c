/* $Id$ */
/* Copyright (c) 2012-2016 Pierre Pronchery <khorben@defora.org> */
/* This file is part of DeforaOS Desktop Camera */
/* This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. */



#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#ifdef __NetBSD__
# include <sys/videoio.h>
# include <paths.h>
#else
# include <linux/videodev2.h>
#endif
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <libintl.h>
#include <gtk/gtk.h>
#if GTK_CHECK_VERSION(3, 0, 0)
# include <gtk/gtkx.h>
#endif
#include <gdk/gdkkeysyms.h>
#include <System.h>
#include <Desktop.h>
#include "camera.h"
#include "../config.h"
#define _(string) gettext(string)
#define N_(string) (string)

/* constants */
#ifndef PROGNAME
# define PROGNAME	"camera"
#endif
#ifndef PREFIX
# define PREFIX		"/usr/local"
#endif
#ifndef BINDIR
# define BINDIR		PREFIX "/bin"
#endif

/* macros */
#ifndef MIN
# define MIN(a, b)	((a) < (b) ? (a) : (b))
#endif


/* Camera */
/* private */
/* types */
typedef struct _CameraBuffer
{
	void * start;
	size_t length;
} CameraBuffer;

struct _Camera
{
	String * device;
	gboolean hflip;
	gboolean vflip;
	gboolean ratio;
	GdkInterpType interp;
	CameraSnapshotFormat snapshot_format;
	int snapshot_quality;

	guint source;
	int fd;
	struct v4l2_capability cap;
	struct v4l2_format format;

	/* I/O channel */
	GIOChannel * channel;

	/* input data */
	/* XXX for mmap() */
	CameraBuffer * buffers;
	size_t buffers_cnt;
	char * raw_buffer;
	size_t raw_buffer_cnt;

	/* RGB data */
	unsigned char * rgb_buffer;
	size_t rgb_buffer_cnt;

	/* decoding */
	int yuv_amp;

	/* overlays */
	CameraOverlay ** overlays;
	size_t overlays_cnt;

	/* widgets */
	GtkWidget * widget;
	GtkWidget * window;
	PangoFontDescription * bold;
#if !GTK_CHECK_VERSION(3, 0, 0)
	GdkGC * gc;
#endif
#if GTK_CHECK_VERSION(2, 18, 0)
	GtkWidget * infobar;
	GtkWidget * infobar_label;
#endif
	GtkWidget * area;
	GtkAllocation area_allocation;
	GdkPixbuf * pixbuf;
#if !GTK_CHECK_VERSION(3, 0, 0)
	GdkPixmap * pixmap;
#endif
	/* preferences */
	GtkWidget * pr_window;
	GtkWidget * pr_hflip;
	GtkWidget * pr_vflip;
	GtkWidget * pr_ratio;
	GtkWidget * pr_interp;
	GtkWidget * pr_sformat;
	/* properties */
	GtkWidget * pp_window;
};


/* constants */
#ifdef _PATH_VIDEO0
# define VIDEO_DEVICE	_PATH_VIDEO0
#else
# define VIDEO_DEVICE	"/dev/video0"
#endif

typedef enum _CameraToolbar
{
	CT_SNAPSHOT = 0,
	CT_SEPARATOR1,
	CT_GALLERY,
	CT_SEPARATOR2,
#ifdef EMBEDDED
	CT_PROPERTIES,
	CT_SEPARATOR3,
	CT_PREFERENCES
#else
	CT_PROPERTIES
#endif
} CameraToolbar;


/* prototypes */
/* accessors */
static String * _camera_get_config_filename(Camera * camera, char const * name);

/* useful */
static int _camera_error(Camera * camera, char const * message, int ret);

static int _camera_ioctl(Camera * camera, unsigned long request,
		void * data);

/* callbacks */
static gboolean _camera_on_can_mmap(GIOChannel * channel,
		GIOCondition condition, gpointer data);
static gboolean _camera_on_can_read(GIOChannel * channel,
		GIOCondition condition, gpointer data);
#if GTK_CHECK_VERSION(3, 0, 0)
static gboolean _camera_on_drawing_area_draw(GtkWidget * widget, cairo_t * cr,
		gpointer data);
static void _camera_on_drawing_area_size_allocate(GtkWidget * widget,
		GdkRectangle * allocation, gpointer data);
#else
static gboolean _camera_on_drawing_area_configure(GtkWidget * widget,
		GdkEventConfigure * event, gpointer data);
static gboolean _camera_on_drawing_area_expose(GtkWidget * widget,
		GdkEventExpose * event, gpointer data);
#endif
static void _camera_on_fullscreen(gpointer data);
static void _camera_on_gallery(gpointer data);
static gboolean _camera_on_open(gpointer data);
#ifdef EMBEDDED
static void _camera_on_preferences(gpointer data);
#endif
static void _camera_on_properties(gpointer data);
static gboolean _camera_on_refresh(gpointer data);
static void _camera_on_snapshot(gpointer data);


/* variables */
static DesktopToolbar _camera_toolbar[] =
{
	{ N_("Snapshot"), G_CALLBACK(_camera_on_snapshot), "camera-photo", 0, 0,
		NULL },
	{ "", NULL, NULL, 0, 0, NULL },
	{ N_("Gallery"), G_CALLBACK(_camera_on_gallery), "image-x-generic", 0,
		0, NULL },
#ifdef EMBEDDED
	{ "", NULL, NULL, 0, 0, NULL },
	{ N_("Properties"), G_CALLBACK(_camera_on_properties),
		GTK_STOCK_PROPERTIES, GDK_MOD1_MASK, GDK_KEY_Return, NULL },
	{ "", NULL, NULL, 0, 0, NULL },
	{ N_("Preferences"), G_CALLBACK(_camera_on_preferences),
		GTK_STOCK_PREFERENCES, GDK_CONTROL_MASK, GDK_KEY_P, NULL },
#else
	{ "", NULL, NULL, 0, 0, NULL },
	{ N_("Properties"), G_CALLBACK(_camera_on_properties),
		GTK_STOCK_PROPERTIES, 0, 0, NULL },
#endif
	{ "", NULL, NULL, 0, 0, NULL },
	{ NULL, NULL, NULL, 0, 0, NULL }
};


/* public */
/* functions */
/* camera_new */
Camera * camera_new(GtkWidget * window, GtkAccelGroup * group,
		char const * device)
{
	Camera * camera;
	GtkWidget * vbox;
	GtkWidget * widget;
	GtkToolItem * toolitem;

	if((camera = object_new(sizeof(*camera))) == NULL)
		return NULL;
	camera->device = (device != NULL)
		? string_new(device) : string_new(VIDEO_DEVICE);
	camera->hflip = FALSE;
	camera->vflip = FALSE;
	camera->ratio = TRUE;
	camera->interp = GDK_INTERP_BILINEAR;
	camera->snapshot_format = CSF_PNG;
	camera->snapshot_quality = 100;
	camera->source = 0;
	camera->fd = -1;
	memset(&camera->cap, 0, sizeof(camera->cap));
	camera->channel = NULL;
	camera->buffers = NULL;
	camera->buffers_cnt = 0;
	camera->raw_buffer = NULL;
	camera->raw_buffer_cnt = 0;
	camera->rgb_buffer = NULL;
	camera->rgb_buffer_cnt = 0;
	camera->yuv_amp = 255;
	camera->overlays = NULL;
	camera->overlays_cnt = 0;
	camera->widget = NULL;
	camera->window = window;
	camera->bold = NULL;
#if !GTK_CHECK_VERSION(3, 0, 0)
	camera->gc = NULL;
#endif
	camera->pr_window = NULL;
	camera->pp_window = NULL;
	/* check for errors */
	if(camera->device == NULL)
	{
		camera_delete(camera);
		return NULL;
	}
	/* create the window */
	camera->bold = pango_font_description_new();
	pango_font_description_set_weight(camera->bold, PANGO_WEIGHT_BOLD);
#if !GTK_CHECK_VERSION(3, 0, 0)
	camera->gc = gdk_gc_new(window->window); /* XXX */
#endif
	camera->widget = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	vbox = camera->widget;
	/* toolbar */
	widget = desktop_toolbar_create(_camera_toolbar, camera, group);
	gtk_widget_set_sensitive(GTK_WIDGET(_camera_toolbar[0].widget), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(_camera_toolbar[2].widget), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(_camera_toolbar[4].widget), FALSE);
	toolitem = gtk_tool_button_new_from_stock(GTK_STOCK_FULLSCREEN);
	g_signal_connect_swapped(toolitem, "clicked", G_CALLBACK(
				_camera_on_fullscreen), camera);
	gtk_toolbar_insert(GTK_TOOLBAR(widget), toolitem, -1);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
#if GTK_CHECK_VERSION(2, 18, 0)
	/* infobar */
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
	camera->pixbuf = NULL;
#if !GTK_CHECK_VERSION(3, 0, 0)
	camera->pixmap = NULL;
#endif
#if GTK_CHECK_VERSION(3, 0, 0)
	g_signal_connect(camera->area, "draw", G_CALLBACK(
				_camera_on_drawing_area_draw), camera);
	g_signal_connect(camera->area, "size-allocate", G_CALLBACK(
				_camera_on_drawing_area_size_allocate), camera);
#else
	g_signal_connect(camera->area, "configure-event", G_CALLBACK(
				_camera_on_drawing_area_configure), camera);
	g_signal_connect(camera->area, "expose-event", G_CALLBACK(
				_camera_on_drawing_area_expose), camera);
#endif
	gtk_box_pack_start(GTK_BOX(vbox), camera->area, TRUE, TRUE, 0);
	gtk_widget_show_all(vbox);
	camera_start(camera);
	return camera;
}


/* camera_delete */
void camera_delete(Camera * camera)
{
	size_t i;

	camera_stop(camera);
	if(camera->pp_window != NULL)
		gtk_widget_destroy(camera->pp_window);
	if(camera->pr_window != NULL)
		gtk_widget_destroy(camera->pr_window);
	for(i = 0; i < camera->overlays_cnt; i++)
		cameraoverlay_delete(camera->overlays[i]);
	free(camera->overlays);
	if(camera->channel != NULL)
	{
		/* XXX we ignore errors at this point */
		g_io_channel_shutdown(camera->channel, TRUE, NULL);
		g_io_channel_unref(camera->channel);
	}
	if(camera->pixbuf != NULL)
		g_object_unref(camera->pixbuf);
#if !GTK_CHECK_VERSION(3, 0, 0)
	if(camera->pixmap != NULL)
		g_object_unref(camera->pixmap);
	if(camera->gc != NULL)
		g_object_unref(camera->gc);
#endif
	if(camera->bold != NULL)
		pango_font_description_free(camera->bold);
	if(camera->fd >= 0)
		close(camera->fd);
	if((char *)camera->rgb_buffer != camera->raw_buffer)
		free(camera->rgb_buffer);
	for(i = 0; i < camera->buffers_cnt; i++)
		if(camera->buffers[i].start != MAP_FAILED)
			munmap(camera->buffers[i].start,
					camera->buffers[i].length);
	free(camera->buffers);
	free(camera->raw_buffer);
	string_delete(camera->device);
	object_delete(camera);
}


/* accessors */
/* camera_get_widget */
GtkWidget * camera_get_widget(Camera * camera)
{
	return camera->widget;
}


/* camera_set_aspect_ratio */
void camera_set_aspect_ratio(Camera * camera, gboolean ratio)
{
	camera->ratio = ratio;
}


/* camera_set_hflip */
void camera_set_hflip(Camera * camera, gboolean flip)
{
	camera->hflip = flip;
}


/* camera_set_vflip */
void camera_set_vflip(Camera * camera, gboolean flip)
{
	camera->vflip = flip;
}


/* useful */
/* camera_add_overlay */
CameraOverlay * camera_add_overlay(Camera * camera, char const * filename,
		int opacity)
{
	CameraOverlay ** p;

	if((p = realloc(camera->overlays, (camera->overlays_cnt + 1)
					* sizeof(*p))) == NULL)
		return NULL;
	camera->overlays = p;
	if((camera->overlays[camera->overlays_cnt] = cameraoverlay_new(
					filename, opacity)) == NULL)
		return NULL;
	return camera->overlays[camera->overlays_cnt++];
}


/* camera_load */
char const * _load_variable(Camera * camera, Config * config,
		char const * section, char const * variable);

int camera_load(Camera * camera)
{
	int ret = 0;
	char * filename;
	Config * config;
	char const * p;
	char * q;
	char const jpeg[] = "jpeg";
	int i;

	if((filename = _camera_get_config_filename(camera, CAMERA_CONFIG_FILE))
			== NULL)
		return -1;
	if((config = config_new()) == NULL
			|| config_load(config, filename) != 0)
		ret = -1;
	else
	{
		/* horizontal flipping */
		camera->hflip = FALSE;
		if((p = _load_variable(camera, config, NULL, "hflip")) != NULL
				&& strtoul(p, NULL, 0) != 0)
			camera->hflip = TRUE;
		/* vertical flipping */
		camera->vflip = FALSE;
		if((p = _load_variable(camera, config, NULL, "vflip")) != NULL
				&& strtoul(p, NULL, 0) != 0)
			camera->vflip = TRUE;
		/* aspect ratio */
		camera->ratio = TRUE;
		if((p = _load_variable(camera, config, NULL, "ratio")) != NULL
				&& strtoul(p, NULL, 0) == 0)
			camera->ratio = FALSE;
		/* snapshot format */
		camera->snapshot_format = CSF_PNG;
		if((p = _load_variable(camera, config, "snapshot", "format"))
				!= NULL
				&& strcmp(p, jpeg) == 0)
			camera->snapshot_format = CSF_JPEG;
		/* snapshot quality */
		camera->snapshot_quality = 100;
		if((p = _load_variable(camera, config, "snapshot", "quality"))
				!= NULL
				&& p[0] != '\0' && (i = strtol(p, &q, 10)) >= 0
				&& *q == '\0' && i <= 100)
			camera->snapshot_quality = i;
		/* FIXME also implement interpolation and overlay images */
	}
	if(config != NULL)
		config_delete(config);
	free(filename);
	return ret;
}

char const * _load_variable(Camera * camera, Config * config,
		char const * section, char const * variable)
{
	char const * ret;

	/* check for any value specific to this camera */
	if(section == NULL)
		if((ret = config_get(config, camera->device, variable)) != NULL)
			return ret;
	/* return the global value set (if any) */
	return config_get(config, section, variable);
}


/* camera_open_gallery */
void camera_open_gallery(Camera * camera)
{
	char * argv[] = { BINDIR "/gallery", "gallery", NULL };
	const GSpawnFlags flags = G_SPAWN_FILE_AND_ARGV_ZERO;
	GError * error = NULL;

	if(g_spawn_async(NULL, argv, NULL, flags, NULL, NULL, NULL, &error)
			!= TRUE && error != NULL)
	{
		_camera_error(camera, error->message, 1);
		g_error_free(error);
	}
}


/* camera_save */
static int _save_variable_bool(Camera * camera, Config * config,
		char const * section, char const * variable, gboolean value);
static int _save_variable_int(Camera * camera, Config * config,
		char const * section, char const * variable, int value);
static int _save_variable_string(Camera * camera, Config * config,
		char const * section, char const * variable,
		char const * value);

int camera_save(Camera * camera)
{
	int ret = -1;
	char * filename;
	Config * config;
	char const * sformats[CSF_COUNT] = { NULL, "png", "jpeg" };

	if((filename = _camera_get_config_filename(camera, CAMERA_CONFIG_FILE))
			== NULL)
		return -1;
	if((config = config_new()) != NULL
			&& access(filename, R_OK) == 0
			&& config_load(config, filename) == 0)
	{
		/* XXX may fail */
		_save_variable_bool(camera, config, NULL, "hflip",
				camera->hflip);
		_save_variable_bool(camera, config, NULL, "vflip",
				camera->vflip);
		_save_variable_bool(camera, config, NULL, "ratio",
				camera->ratio);
		_save_variable_string(camera, config, "snapshot", "format",
				sformats[camera->snapshot_format]);
		_save_variable_int(camera, config, "snapshot", "quality",
				camera->snapshot_quality);
		/* FIXME also implement interpolation and overlay images */
		ret = config_save(config, filename);
	}
	if(config != NULL)
		config_delete(config);
	free(filename);
	return ret;
}

static int _save_variable_bool(Camera * camera, Config * config,
		char const * section, char const * variable, gboolean value)
{
	if(section == NULL)
		section = camera->device;
	return config_set(config, section, variable, value ? "1" : "0");
}

static int _save_variable_int(Camera * camera, Config * config,
		char const * section, char const * variable, int value)
{
	char buf[16];

	if(section == NULL)
		section = camera->device;
	snprintf(buf, sizeof(buf), "%d", value);
	return config_set(config, section, variable, buf);
}

static int _save_variable_string(Camera * camera, Config * config,
		char const * section, char const * variable,
		char const * value)
{
	if(section == NULL)
		section = camera->device;
	return config_set(config, section, variable, value);
}


/* camera_show_preferences */
static void _preferences_apply(Camera * camera);
static void _preferences_cancel(Camera * camera);
static void _preferences_save(Camera * camera);
static void _preferences_window(Camera * camera);
/* callbacks */
static void _preferences_on_response(GtkWidget * widget, gint arg1,
		gpointer data);

void camera_show_preferences(Camera * camera, gboolean show)
{
	if(camera->pr_window == NULL)
		_preferences_window(camera);
	if(show)
		gtk_window_present(GTK_WINDOW(camera->pr_window));
	else
		gtk_widget_hide(camera->pr_window);
}

static void _preferences_apply(Camera * camera)
{
	GtkTreeModel * model;
	GtkTreeIter iter;

	camera->hflip = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
				camera->pr_hflip));
	camera->vflip = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
				camera->pr_vflip));
	camera->ratio = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
				camera->pr_ratio));
	/* interpolation */
	if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(camera->pr_interp),
				&iter) == TRUE)
	{
		model = gtk_combo_box_get_model(GTK_COMBO_BOX(
					camera->pr_interp));
		gtk_tree_model_get(model, &iter, 0, &camera->interp, -1);
	}
	/* snapshot format */
	if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(camera->pr_sformat),
				&iter) == TRUE)
	{
		model = gtk_combo_box_get_model(GTK_COMBO_BOX(
					camera->pr_sformat));
		gtk_tree_model_get(model, &iter, 0, &camera->snapshot_format,
				-1);
	}
}

static void _preferences_cancel(Camera * camera)
{
	GtkTreeModel * model;
	GtkTreeIter iter;
	gboolean valid;
	GdkInterpType interp;
	CameraSnapshotFormat format;

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(camera->pr_hflip),
			camera->hflip);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(camera->pr_vflip),
			camera->vflip);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(camera->pr_ratio),
			camera->ratio);
	/* interpolation */
	model = gtk_combo_box_get_model(GTK_COMBO_BOX(camera->pr_interp));
	for(valid = gtk_tree_model_get_iter_first(model, &iter); valid == TRUE;
			valid = gtk_tree_model_iter_next(model, &iter))
	{
		gtk_tree_model_get(model, &iter, 0, &interp, -1);
		if(interp == camera->interp)
			break;
	}
	if(valid)
		gtk_combo_box_set_active_iter(GTK_COMBO_BOX(camera->pr_interp),
				&iter);
	else
		gtk_combo_box_set_active(GTK_COMBO_BOX(camera->pr_interp), 0);
	/* snapshot format */
	model = gtk_combo_box_get_model(GTK_COMBO_BOX(camera->pr_sformat));
	for(valid = gtk_tree_model_get_iter_first(model, &iter); valid == TRUE;
			valid = gtk_tree_model_iter_next(model, &iter))
	{
		gtk_tree_model_get(model, &iter, 0, &format, -1);
		if(format == camera->snapshot_format)
			break;
	}
	if(valid)
		gtk_combo_box_set_active_iter(GTK_COMBO_BOX(camera->pr_sformat),
				&iter);
	else
		gtk_combo_box_set_active(GTK_COMBO_BOX(camera->pr_sformat), 0);
}

static void _preferences_save(Camera * camera)
{
	camera_save(camera);
}

static void _preferences_window(Camera * camera)
{
	GtkWidget * dialog;
	GtkWidget * notebook;
	GtkWidget * vbox;
	GtkWidget * widget;
	GtkListStore * store;
	GtkTreeIter iter;
	GtkCellRenderer * renderer;
	const struct {
		GdkInterpType type;
		char const * name;
	} interp[] =
	{
		{ GDK_INTERP_NEAREST, N_("Nearest") },
		{ GDK_INTERP_TILES, N_("Tiles") },
		{ GDK_INTERP_BILINEAR, N_("Bilinear") },
		{ GDK_INTERP_HYPER, N_("Hyperbolic") },
	};
	const struct {
		CameraSnapshotFormat format;
		char const * name;
	} sformats[CSF_COUNT - 1] =
	{
		{ CSF_JPEG, "JPEG" },
		{ CSF_PNG, "PNG" }
	};
	size_t i;

	dialog = gtk_dialog_new_with_buttons(_("Preferences"),
			GTK_WINDOW(camera->window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_APPLY, GTK_RESPONSE_APPLY,
			GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
	camera->pr_window = dialog;
	g_signal_connect(dialog, "response", G_CALLBACK(
				_preferences_on_response), camera);
	notebook = gtk_notebook_new();
	/* picture */
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 4);
	camera->pr_hflip = gtk_check_button_new_with_mnemonic(
			_("Flip _horizontally"));
	gtk_box_pack_start(GTK_BOX(vbox), camera->pr_hflip, FALSE, TRUE, 0);
	camera->pr_vflip = gtk_check_button_new_with_mnemonic(
			_("Flip _vertically"));
	gtk_box_pack_start(GTK_BOX(vbox), camera->pr_vflip, FALSE, TRUE, 0);
	camera->pr_ratio = gtk_check_button_new_with_mnemonic(
			_("Keep aspect _ratio"));
	gtk_box_pack_start(GTK_BOX(vbox), camera->pr_ratio, FALSE, TRUE, 0);
	/* interpolation */
	widget = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	gtk_box_pack_start(GTK_BOX(widget), gtk_label_new(_("Interpolation: ")),
			FALSE, TRUE, 0);
	store = gtk_list_store_new(2, G_TYPE_UINT, G_TYPE_STRING);
	for(i = 0; i < sizeof(interp) / sizeof(*interp); i++)
	{
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 0, interp[i].type,
				1, _(interp[i].name), -1);
	}
	camera->pr_interp = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(camera->pr_interp), renderer,
			TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(camera->pr_interp),
			renderer, "text", 1, NULL);
	gtk_box_pack_start(GTK_BOX(widget), camera->pr_interp, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox,
			gtk_label_new(_("Picture")));
	/* snapshots */
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 4);
	/* format */
	widget = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	gtk_box_pack_start(GTK_BOX(widget), gtk_label_new(_("Format: ")),
			FALSE, TRUE, 0);
	store = gtk_list_store_new(2, G_TYPE_UINT, G_TYPE_STRING);
	for(i = 0; i < sizeof(sformats) / sizeof(*sformats); i++)
	{
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 0, sformats[i].format,
				1, sformats[i].name, -1);
	}
	camera->pr_sformat = gtk_combo_box_new_with_model(
			GTK_TREE_MODEL(store));
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(camera->pr_sformat),
			renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(camera->pr_sformat),
			renderer, "text", 1, NULL);
	gtk_box_pack_start(GTK_BOX(widget), camera->pr_sformat, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox,
			gtk_label_new(_("Snapshots")));
#if GTK_CHECK_VERSION(2, 14, 0)
	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
#else
	vbox = dialog->vbox;
#endif
	gtk_box_set_spacing(GTK_BOX(vbox), 4);
	gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);
	gtk_widget_show_all(vbox);
	_preferences_cancel(camera);
}

static void _preferences_on_response(GtkWidget * widget, gint arg1,
		gpointer data)
{
	Camera * camera = data;

	switch(arg1)
	{
		case GTK_RESPONSE_APPLY:
			_preferences_apply(camera);
			break;
		case GTK_RESPONSE_OK:
			gtk_widget_hide(widget);
			_preferences_apply(camera);
			_preferences_save(camera);
			break;
		case GTK_RESPONSE_DELETE_EVENT:
			camera->pr_window = NULL;
			break;
		case GTK_RESPONSE_CANCEL:
		default:
			gtk_widget_hide(widget);
			_preferences_cancel(camera);
			break;
	}
}


/* camera_show_properties */
static GtkWidget * _properties_label(Camera * camera, GtkSizeGroup * group,
		char const * label, char const * value);
static void _properties_window(Camera * camera);
/* callbacks */
static void _properties_on_response(gpointer data);

void camera_show_properties(Camera * camera, gboolean show)
{
	if(camera->rgb_buffer == NULL)
		/* ignore the action */
		return;
	if(show)
	{
		if(camera->pp_window == NULL)
			_properties_window(camera);
		gtk_window_present(GTK_WINDOW(camera->pp_window));
	}
	else
	{
		if(camera->pp_window != NULL)
			gtk_widget_destroy(camera->pp_window);
		camera->pp_window = NULL;
	}
}

static GtkWidget * _properties_label(Camera * camera, GtkSizeGroup * group,
		char const * label, char const * value)
{
	GtkWidget * hbox;
	GtkWidget * widget;

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	widget = gtk_label_new(label);
#if GTK_CHECK_VERSION(3, 0, 0)
	gtk_widget_override_font(widget, camera->bold);
#else
	gtk_widget_modify_font(widget, camera->bold);
#endif
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_size_group_add_widget(group, widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	widget = gtk_label_new((value != NULL) ? value : "");
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
	return hbox;
}

static void _properties_window(Camera * camera)
{
	GtkWidget * dialog;
	GtkSizeGroup * group;
	GtkWidget * vbox;
	GtkWidget * hbox;
	char buf[64];
	const struct
	{
		unsigned int capability;
		char const * name;
	} capabilities[] =
	{
		{ V4L2_CAP_VIDEO_CAPTURE,	"capture"	},
		{ V4L2_CAP_VIDEO_OUTPUT,	"output"	},
		{ V4L2_CAP_VIDEO_OVERLAY,	"overlay"	},
		{ V4L2_CAP_TUNER,		"tuner"		},
		{ V4L2_CAP_AUDIO,		"audio"		},
		{ V4L2_CAP_STREAMING,		"streaming"	},
		{ 0,				NULL		}
	};
	unsigned int i;
	char const * sep = "";

	dialog = gtk_message_dialog_new(GTK_WINDOW(camera->window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
#if GTK_CHECK_VERSION(2, 6, 0)
			"%s", _("Properties"));
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
#endif
			"");
	camera->pp_window = dialog;
#if GTK_CHECK_VERSION(2, 10, 0)
	gtk_message_dialog_set_image(GTK_MESSAGE_DIALOG(dialog),
			gtk_image_new_from_stock(GTK_STOCK_PROPERTIES,
				GTK_ICON_SIZE_DIALOG));
#endif
	gtk_window_set_title(GTK_WINDOW(dialog), _("Properties"));
	g_signal_connect_swapped(dialog, "response", G_CALLBACK(
				_properties_on_response), camera);
	group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
#if GTK_CHECK_VERSION(2, 14, 0)
	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
#else
	vbox = dialog->vbox;
#endif
	/* driver */
	snprintf(buf, sizeof(buf), "%-16s", (char *)camera->cap.driver);
	hbox = _properties_label(camera, group, _("Driver: "), buf);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	/* card */
	snprintf(buf, sizeof(buf), "%-32s", (char *)camera->cap.card);
	hbox = _properties_label(camera, group, _("Card: "), buf);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	/* bus info */
	snprintf(buf, sizeof(buf), "%-32s", (char *)camera->cap.bus_info);
	hbox = _properties_label(camera, group, _("Bus info: "), buf);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	/* version */
	snprintf(buf, sizeof(buf), "0x%x", camera->cap.version);
	hbox = _properties_label(camera, group, _("Version: "), buf);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	/* capabilities */
	buf[0] = '\0';
	for(i = 0; capabilities[i].name != NULL; i++)
		if(camera->cap.capabilities & capabilities[i].capability)
		{
			strncat(buf, sep, sizeof(buf) - strlen(buf));
			strncat(buf, capabilities[i].name, sizeof(buf)
					- strlen(buf));
			sep = ", ";
		}
	buf[sizeof(buf) - 1] = '\0';
	hbox = _properties_label(camera, group, _("Capabilities: "), buf);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	gtk_widget_show_all(vbox);
}

static void _properties_on_response(gpointer data)
{
	Camera * camera = data;

	gtk_widget_destroy(camera->pp_window);
	camera->pp_window = NULL;
}


/* camera_snapshot */
static int _snapshot_dcim(Camera * camera, char const * homedir,
		char const * dcim);
static char * _snapshot_path(Camera * camera, char const * homedir,
		char const * dcim, char const * extension);
static int _snapshot_save(Camera * camera, char const * path,
		CameraSnapshotFormat format);

int camera_snapshot(Camera * camera, CameraSnapshotFormat format)
{
	int ret;
	char const * homedir;
	char const dcim[] = "DCIM";
	char const * ext[CSF_COUNT] = { NULL, ".png", ".jpeg" };
	char const * e;
	char * path;

	if(camera->rgb_buffer == NULL)
		/* ignore the action */
		return 0;
	if(format == CSF_DEFAULT)
		format = camera->snapshot_format;
	switch(format)
	{
		case CSF_JPEG:
		case CSF_PNG:
			e = ext[format];
			break;
		default:
			format = CSF_PNG;
			e = ext[format];
			break;
	}
	if((homedir = getenv("HOME")) == NULL)
		homedir = g_get_home_dir();
	if(_snapshot_dcim(camera, homedir, dcim) != 0)
		return -1;
	if((path = _snapshot_path(camera, homedir, dcim, e)) == NULL)
		return -1;
	ret = _snapshot_save(camera, path, format);
	free(path);
	return ret;
}

static int _snapshot_dcim(Camera * camera, char const * homedir,
		char const * dcim)
{
	char * path;

	if((path = g_build_filename(homedir, dcim, NULL)) == NULL)
		return -_camera_error(camera, _("Could not save picture"), 1);
	if(mkdir(path, 0777) != 0 && errno != EEXIST)
	{
		error_set_code(1, "%s: %s: %s", _("Could not save picture"),
				path, strerror(errno));
		free(path);
		return -_camera_error(camera, error_get(NULL), 1);
	}
	free(path);
	return 0;
}

static char * _snapshot_path(Camera * camera, char const * homedir,
		char const * dcim, char const * extension)
{
	struct timeval tv;
	struct tm tm;
	unsigned int i;
	char * filename;
	char * path;

	if(gettimeofday(&tv, NULL) != 0 || gmtime_r(&tv.tv_sec, &tm) == NULL)
	{
		error_set_code(1, "%s: %s", _("Could not save picture"),
				strerror(errno));
		_camera_error(camera, error_get(NULL), 1);
		return NULL;
	}
	for(i = 0; i < 64; i++)
	{
		if((filename = g_strdup_printf("%u%02u%02u-%02u%02u%02u-%03u%s",
						tm.tm_year + 1900,
						tm.tm_mon + 1, tm.tm_mday,
						tm.tm_hour, tm.tm_min,
						tm.tm_sec, i + 1, extension))
				== NULL)
			/* XXX report error */
			return NULL;
		path = g_build_filename(homedir, dcim, filename, NULL);
		g_free(filename);
		if(path == NULL)
		{
			_camera_error(camera, _("Could not save picture"), 1);
			return NULL;
		}
#ifdef DEBUG
		fprintf(stderr, "DEBUG: %s() %s\n", __func__, path);
#endif
		if(access(path, R_OK) != 0 && errno == ENOENT)
			return path;
		g_free(path);
	}
	return NULL;
}

static int _snapshot_save(Camera * camera, char const * path,
		CameraSnapshotFormat format)
{
	struct v4l2_pix_format * pix = &camera->format.fmt.pix;
	GdkPixbuf * pixbuf;
	char buf[16];
	gboolean res;
	GError * error = NULL;

	if((pixbuf = gdk_pixbuf_new_from_data(camera->rgb_buffer,
					GDK_COLORSPACE_RGB, FALSE, 8,
					pix->width, pix->height, pix->width * 3,
					NULL, NULL)) == NULL)
		return -_camera_error(camera, _("Could not save picture"), 1);
	switch(format)
	{
		case CSF_JPEG:
			snprintf(buf, sizeof(buf), "%d",
					camera->snapshot_quality);
			res = gdk_pixbuf_save(pixbuf, path, "jpeg", &error,
					"quality", buf, NULL);
			break;
		case CSF_PNG:
			res = gdk_pixbuf_save(pixbuf, path, "png", &error,
					NULL);
			break;
		default:
			res = FALSE;
			break;
	}
	g_object_unref(pixbuf);
	if(res != TRUE)
	{
		error_set_code(1, "%s: %s", _("Could not save picture"),
				(error != NULL) ? error->message
				: _("Unknown error"));
		g_error_free(error);
		return -_camera_error(camera, error_get(NULL), 1);
	}
	return 0;
}


/* camera_start */
void camera_start(Camera * camera)
{
	if(camera->source != 0)
		return;
	camera->source = g_idle_add(_camera_on_open, camera);
}


/* camera_stop */
void camera_stop(Camera * camera)
{
	if(camera->source != 0)
		g_source_remove(camera->source);
	camera->source = 0;
}


/* private */
/* functions */
/* accessors */
/* camera_get_config_filename */
static String * _camera_get_config_filename(Camera * camera, char const * name)
{
	char const * homedir;
	(void) camera;

	if((homedir = getenv("HOME")) == NULL)
		homedir = g_get_home_dir();
	return string_new_append(homedir, "/", name, NULL);
}


/* useful */
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
			"%s", _("Error"));
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
# endif
			"%s", message);
	gtk_window_set_title(GTK_WINDOW(dialog), _("Error"));
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
#endif
	return ret;
}

static int _error_text(char const * message, int ret)
{
	fprintf(stderr, "%s: %s\n", PROGNAME, message);
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
/* camera_on_can_mmap */
static gboolean _camera_on_can_mmap(GIOChannel * channel,
		GIOCondition condition, gpointer data)
{
	Camera * camera = data;
	struct v4l2_buffer buf;

	if(channel != camera->channel || condition != G_IO_IN)
		return FALSE;
	if(_camera_ioctl(camera, VIDIOC_DQBUF, &buf) == -1)
	{
		_camera_error(camera, _("Could not save picture"), 1);
		return FALSE;
	}
	camera->raw_buffer = camera->buffers[buf.index].start;
	camera->raw_buffer_cnt = buf.bytesused;
#if 0 /* FIXME the raw buffer is not meant to be free()'d */
	camera->source = g_idle_add(_camera_on_refresh, camera);
	return FALSE;
#else
	_camera_on_refresh(camera);
	camera->raw_buffer = NULL;
	camera->raw_buffer_cnt = 0;
	return TRUE;
#endif
}


/* camera_on_can_read */
static gboolean _camera_on_can_read(GIOChannel * channel,
		GIOCondition condition, gpointer data)
{
	Camera * camera = data;
	GIOStatus status;
	gsize size;
	GError * error = NULL;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if(channel != camera->channel || condition != G_IO_IN)
		return FALSE;
	status = g_io_channel_read_chars(channel, camera->raw_buffer,
			camera->raw_buffer_cnt, &size, &error);
	/* this status can be ignored */
	if(status == G_IO_STATUS_AGAIN)
		return TRUE;
	if(status == G_IO_STATUS_ERROR)
	{
		g_io_channel_shutdown(camera->channel, TRUE, NULL);
		g_io_channel_unref(camera->channel);
		camera->channel = NULL;
		camera->fd = -1;
		_camera_error(camera, error->message, 1);
		g_error_free(error);
		gtk_widget_set_sensitive(GTK_WIDGET(
					_camera_toolbar[CT_SNAPSHOT].widget),
				FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(
					_camera_toolbar[CT_GALLERY].widget),
				FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(
					_camera_toolbar[CT_PROPERTIES].widget),
				FALSE);
		camera->source = 0;
		return FALSE;
	}
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() %lu %lu\n", __func__,
			camera->raw_buffer_cnt, size);
#endif
	camera->source = g_idle_add(_camera_on_refresh, camera);
	return FALSE;
}


#if GTK_CHECK_VERSION(3, 0, 0)
/* camera_on_drawing_area_draw */
static gboolean _camera_on_drawing_area_draw(GtkWidget * widget, cairo_t * cr,
		gpointer data)
{
	Camera * camera = data;
	(void) widget;

	if(camera->pixbuf != NULL)
	{
		gdk_cairo_set_source_pixbuf(cr, camera->pixbuf, 0, 0);
		cairo_paint(cr);
	}
	return TRUE;
}


/* camera_on_drawing_area_size_allocate */
static void _camera_on_drawing_area_size_allocate(GtkWidget * widget,
		GdkRectangle * allocation, gpointer data)
{
	Camera * camera = data;
	(void) widget;

	camera->area_allocation = *allocation;
}

#else

/* camera_on_drawing_area_configure */
static gboolean _camera_on_drawing_area_configure(GtkWidget * widget,
		GdkEventConfigure * event, gpointer data)
{
	(void) event;

	/* XXX this code is inspired from GQcam */
	Camera * camera = data;
	GtkAllocation * allocation = &camera->area_allocation;

	if(camera->pixmap != NULL)
		g_object_unref(camera->pixmap);
	/* FIXME requires Gtk+ 2.18 */
	gtk_widget_get_allocation(widget, allocation);
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() %dx%d\n", __func__, allocation->width,
			allocation->height);
#endif
	camera->pixmap = gdk_pixmap_new(widget->window, allocation->width,
			allocation->height, -1);
	/* FIXME is it not better to scale the previous pixmap for now? */
	gdk_draw_rectangle(camera->pixmap, camera->gc, TRUE, 0, 0,
			allocation->width, allocation->height);
	return TRUE;
}


/* camera_on_drawing_area_expose */
static gboolean _camera_on_drawing_area_expose(GtkWidget * widget,
		GdkEventExpose * event, gpointer data)
{
	/* XXX this code is inspired from GQcam */
	Camera * camera = data;

	gdk_draw_pixmap(widget->window, camera->gc, camera->pixmap,
			event->area.x, event->area.y,
			event->area.x, event->area.y,
			event->area.width, event->area.height);
	return FALSE;
}
#endif


/* camera_on_fullscreen */
static void _camera_on_fullscreen(gpointer data)
{
	Camera * camera = data;
	GdkWindow * window;
	GdkWindowState state;

#if GTK_CHECK_VERSION(2, 14, 0)
	window = gtk_widget_get_window(camera->window);
#else
	window = camera->window->window;
#endif
	state = gdk_window_get_state(window);
	if(state & GDK_WINDOW_STATE_FULLSCREEN)
		gtk_window_unfullscreen(GTK_WINDOW(camera->window));
	else
		gtk_window_fullscreen(GTK_WINDOW(camera->window));
}


/* camera_on_gallery */
static void _camera_on_gallery(gpointer data)
{
	Camera * camera = data;

	camera_open_gallery(camera);
}


/* camera_on_open */
static int _open_setup(Camera * camera);
#ifdef NOTYET
static int _open_setup_mmap(Camera * camera);
#endif
static int _open_setup_read(Camera * camera);

static gboolean _camera_on_open(gpointer data)
{
	Camera * camera = data;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() \"%s\"\n", __func__, camera->device);
#endif
	camera->source = 0;
	if((camera->fd = open(camera->device, O_RDWR)) < 0)
	{
		error_set_code(1, "%s: %s (%s)", camera->device,
				_("Could not open the video capture device"),
				strerror(errno));
		_camera_error(camera, error_get(NULL), 1);
		return FALSE;
	}
	if(_open_setup(camera) != 0)
	{
		_camera_error(camera, error_get(NULL), 1);
		close(camera->fd);
		camera->fd = -1;
		/* FIXME also free camera->buffers */
		return FALSE;
	}
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() %dx%d\n", __func__,
			camera->format.fmt.pix.width,
			camera->format.fmt.pix.height);
#endif
	gtk_widget_set_sensitive(GTK_WIDGET(_camera_toolbar[0].widget), TRUE);
	gtk_widget_set_sensitive(GTK_WIDGET(_camera_toolbar[2].widget), TRUE);
	gtk_widget_set_sensitive(GTK_WIDGET(_camera_toolbar[4].widget), TRUE);
	/* FIXME allow the window to be smaller */
	gtk_widget_set_size_request(camera->area, camera->format.fmt.pix.width,
			camera->format.fmt.pix.height);
	return FALSE;
}

static int _open_setup(Camera * camera)
{
	int ret;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	GError * error = NULL;

	/* check for capabilities */
	if(_camera_ioctl(camera, VIDIOC_QUERYCAP, &camera->cap) == -1)
		return -error_set_code(1, "%s: %s (%s)", camera->device,
				_("Could not obtain the capabilities"),
				strerror(errno));
	if((camera->cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0)
		return -error_set_code(1, "%s: %s", camera->device,
				_("Not a video capture device"));
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
			/* XXX ignore this error for now */
			error_set_code(1, "%s: %s", camera->device,
					_("Cropping not supported"));
	}
	/* obtain the current format */
	if(_camera_ioctl(camera, VIDIOC_G_FMT, &camera->format) == -1)
		return -error_set_code(1, "%s: %s", camera->device,
				_("Could not obtain the video capture format"));
	/* check the current format */
	if(camera->format.type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -error_set_code(1, "%s: %s", camera->device,
				_("Unsupported video capture type"));
	if((camera->cap.capabilities & V4L2_CAP_STREAMING) != 0)
#ifdef NOTYET
		ret = _open_setup_mmap(camera);
#else
		ret = _open_setup_read(camera);
#endif
	else if((camera->cap.capabilities & V4L2_CAP_READWRITE) != 0)
		ret = _open_setup_read(camera);
	else
		ret = -error_set_code(1, "%s: %s", camera->device,
				_("Unsupported capabilities"));
	if(ret != 0)
		return ret;
	/* setup an I/O channel */
	camera->channel = g_io_channel_unix_new(camera->fd);
	if(g_io_channel_set_encoding(camera->channel, NULL, &error)
			!= G_IO_STATUS_NORMAL)
	{
		error_set_code(1, "%s", error->message);
		g_error_free(error);
		return -1;
	}
	g_io_channel_set_buffered(camera->channel, FALSE);
	camera->source = g_io_add_watch(camera->channel, G_IO_IN,
			(camera->buffers != NULL) ? _camera_on_can_mmap
			: _camera_on_can_read, camera);
	return 0;
}

#ifdef NOTYET
static int _open_setup_mmap(Camera * camera)
{
	struct v4l2_requestbuffers req;
	size_t i;
	struct v4l2_buffer buf;

	/* memory mapping support */
	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	if(_camera_ioctl(camera, VIDIOC_REQBUFS, &req) == -1)
		return -error_set_code(1, "%s: %s", camera->device,
				_("Could not request buffers"));
	if(req.count < 2)
		return -error_set_code(1, "%s: %s", camera->device,
				_("Could not obtain enough buffers"));
	if((camera->buffers = malloc(sizeof(*camera->buffers) * req.count))
			== NULL)
		return -error_set_code(1, "%s: %s", camera->device,
				_("Could not allocate buffers"));
	camera->buffers_cnt = req.count;
	/* initialize the buffers */
	memset(camera->buffers, 0, sizeof(*camera->buffers)
			* camera->buffers_cnt);
	for(i = 0; i < camera->buffers_cnt; i++)
		camera->buffers[i].start = MAP_FAILED;
	/* map the buffers */
	for(i = 0; i < camera->buffers_cnt; i++)
	{
		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;
		if(_camera_ioctl(camera, VIDIOC_QUERYBUF, &buf) == -1)
			return -error_set_code(1, "%s: %s", camera->device,
					_("Could not setup buffers"));
		camera->buffers[i].start = mmap(NULL, buf.length,
				PROT_READ | PROT_WRITE, MAP_SHARED, camera->fd,
				buf.m.offset);
		if(camera->buffers[i].start == MAP_FAILED)
			return -error_set_code(1, "%s: %s", camera->device,
					_("Could not map buffers"));
		camera->buffers[i].length = buf.length;
	}
	return 0;
}
#endif

static int _open_setup_read(Camera * camera)
{
	size_t cnt;
	char * p;

	/* FIXME also try to obtain a RGB24 format if possible */
	/* allocate the raw buffer */
	cnt = camera->format.fmt.pix.sizeimage;
	if((p = realloc(camera->raw_buffer, cnt)) == NULL)
		return -error_set_code(1, "%s: %s", camera->device,
				strerror(errno));
	camera->raw_buffer = p;
	camera->raw_buffer_cnt = cnt;
	/* allocate the rgb buffer */
	cnt = camera->format.fmt.pix.width * camera->format.fmt.pix.height * 3;
	if((p = realloc(camera->rgb_buffer, cnt)) == NULL)
		return -error_set_code(1, "%s: %s", camera->device,
				strerror(errno));
	camera->rgb_buffer = (unsigned char *)p;
	camera->rgb_buffer_cnt = cnt;
	return 0;
}


#ifdef EMBEDDED
/* camera_on_preferences */
static void _camera_on_preferences(gpointer data)
{
	Camera * camera = data;

	camera_show_preferences(camera, TRUE);
}
#endif


/* camera_on_properties */
static void _camera_on_properties(gpointer data)
{
	Camera * camera = data;

	camera_show_properties(camera, TRUE);
}


/* camera_on_refresh */
static void _refresh_convert(Camera * camera);
static void _refresh_convert_yuv(int amp, uint8_t y, uint8_t u, uint8_t v,
		uint8_t * r, uint8_t * g, uint8_t * b);
static void _refresh_hflip(Camera * camera, GdkPixbuf ** pixbuf);
static void _refresh_overlays(Camera * camera, GdkPixbuf * pixbuf);
static void _refresh_scale(Camera * camera, GdkPixbuf ** pixbuf);
static void _refresh_vflip(Camera * camera, GdkPixbuf ** pixbuf);

static gboolean _camera_on_refresh(gpointer data)
{
	Camera * camera = data;
#if !GTK_CHECK_VERSION(3, 0, 0)
	GtkAllocation * allocation = &camera->area_allocation;
#endif
	int width = camera->format.fmt.pix.width;
	int height = camera->format.fmt.pix.height;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() 0x%x\n", __func__,
			camera->format.fmt.pix.pixelformat);
#endif
	_refresh_convert(camera);
#if !GTK_CHECK_VERSION(3, 0, 0)
	if(camera->hflip == FALSE
			&& camera->vflip == FALSE
			&& width == allocation->width
			&& height == allocation->height
			&& camera->overlays_cnt == 0)
		/* render directly */
		gdk_draw_rgb_image(camera->pixmap, camera->gc, 0, 0,
				width, height, GDK_RGB_DITHER_NORMAL,
				camera->rgb_buffer, width * 3);
	else
#endif
	{
		if(camera->pixbuf != NULL)
			g_object_unref(camera->pixbuf);
		/* render after scaling */
		camera->pixbuf = gdk_pixbuf_new_from_data(camera->rgb_buffer,
				GDK_COLORSPACE_RGB, FALSE, 8, width, height,
				width * 3, NULL, NULL);
		_refresh_hflip(camera, &camera->pixbuf);
		_refresh_vflip(camera, &camera->pixbuf);
		_refresh_scale(camera, &camera->pixbuf);
		_refresh_overlays(camera, camera->pixbuf);
#if !GTK_CHECK_VERSION(3, 0, 0)
		gdk_pixbuf_render_to_drawable(camera->pixbuf, camera->pixmap,
				camera->gc, 0, 0, 0, 0, -1, -1,
				GDK_RGB_DITHER_NORMAL, 0, 0);
#endif
	}
	/* force a refresh */
	gtk_widget_queue_draw(camera->area);
	camera->source = g_io_add_watch(camera->channel, G_IO_IN,
			(camera->buffers != NULL) ? _camera_on_can_mmap
			: _camera_on_can_read, camera);
	return FALSE;
}

static void _refresh_convert(Camera * camera)
{
	size_t i;
	size_t j;

	switch(camera->format.fmt.pix.pixelformat)
	{
		case V4L2_PIX_FMT_YUYV:
			for(i = 0, j = 0; i + 3 < camera->raw_buffer_cnt;
					i += 4, j += 6)
			{
				/* pixel 0 */
				_refresh_convert_yuv(camera->yuv_amp,
						camera->raw_buffer[i],
						camera->raw_buffer[i + 1],
						camera->raw_buffer[i + 3],
						&camera->rgb_buffer[j + 2],
						&camera->rgb_buffer[j + 1],
						&camera->rgb_buffer[j]);
				/* pixel 1 */
				_refresh_convert_yuv(camera->yuv_amp,
						camera->raw_buffer[i + 2],
						camera->raw_buffer[i + 1],
						camera->raw_buffer[i + 3],
						&camera->rgb_buffer[j + 5],
						&camera->rgb_buffer[j + 4],
						&camera->rgb_buffer[j + 3]);
			}
			break;
		default:
#ifdef DEBUG
			fprintf(stderr, "DEBUG: %s() Unsupported format\n",
					__func__);
#endif
			break;
	}
}

static void _refresh_convert_yuv(int amp, uint8_t y, uint8_t u, uint8_t v,
		uint8_t * r, uint8_t * g, uint8_t * b)
{
	double dr;
	double dg;
	double db;

	dr = amp * (0.004565 * y + 0.007935 * u - 1.088);
	dg = amp * (0.004565 * y - 0.001542 * u - 0.003183 * v + 0.531);
	db = amp * (0.004565 * y + 0.000001 * u + 0.006250 * v - 0.872);
	*r = (dr < 0) ? 0 : ((dr > 255) ? 255 : dr);
	*g = (dg < 0) ? 0 : ((dg > 255) ? 255 : dg);
	*b = (db < 0) ? 0 : ((db > 255) ? 255 : db);
}

static void _refresh_hflip(Camera * camera, GdkPixbuf ** pixbuf)
{
	GdkPixbuf * pixbuf2;

	if(camera->hflip == FALSE)
		return;
	/* XXX could probably be more efficient */
	pixbuf2 = gdk_pixbuf_flip(*pixbuf, TRUE);
	g_object_unref(*pixbuf);
	*pixbuf = pixbuf2;
}

static void _refresh_overlays(Camera * camera, GdkPixbuf * pixbuf)
{
	size_t i;

	for(i = 0; i < camera->overlays_cnt; i++)
		cameraoverlay_blit(camera->overlays[i], pixbuf);
}

static void _refresh_scale(Camera * camera, GdkPixbuf ** pixbuf)
{
	GtkAllocation * allocation = &camera->area_allocation;
	GdkPixbuf * pixbuf2;
	gdouble scale;
	gint width;
	gint height;
	gint x;
	gint y;

	if(allocation->width > 0 && allocation->height > 0
			&& (uint32_t)allocation->width
			== camera->format.fmt.pix.width
			&& (uint32_t)allocation->height
			== camera->format.fmt.pix.height)
		/* no need to scale anything */
		return;
	if(camera->ratio == FALSE)
		pixbuf2 = gdk_pixbuf_scale_simple(*pixbuf, allocation->width,
				allocation->height, camera->interp);
	else
	{
		if((pixbuf2 = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8,
						allocation->width,
						allocation->height)) == NULL)
			/* XXX report errors */
			return;
		/* XXX could be more efficient */
		gdk_pixbuf_fill(pixbuf2, 0);
		scale = (gdouble)allocation->width
			/ camera->format.fmt.pix.width;
		scale = MIN(scale, (gdouble)allocation->height
				/ camera->format.fmt.pix.height);
		width = (gdouble)camera->format.fmt.pix.width * scale;
		width = MIN(width, allocation->width);
		height = (gdouble)camera->format.fmt.pix.height * scale;
		height = MIN(height, allocation->height);
		x = (allocation->width - width) / 2;
		y = (allocation->height - height) / 2;
		gdk_pixbuf_scale(*pixbuf, pixbuf2, x, y, width, height,
				0.0, 0.0, scale, scale, camera->interp);
	}
	g_object_unref(*pixbuf);
	*pixbuf = pixbuf2;
}

static void _refresh_vflip(Camera * camera, GdkPixbuf ** pixbuf)
{
	GdkPixbuf * pixbuf2;

	if(camera->vflip == FALSE)
		return;
	/* XXX could probably be more efficient */
	pixbuf2 = gdk_pixbuf_flip(*pixbuf, FALSE);
	g_object_unref(*pixbuf);
	*pixbuf = pixbuf2;
}


/* camera_on_snapshot */
static void _camera_on_snapshot(gpointer data)
{
	Camera * camera = data;

	camera_snapshot(camera, CSF_DEFAULT);
}
