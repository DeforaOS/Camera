/* $Id$ */
/* Copyright (c) 2012 Pierre Pronchery <khorben@defora.org> */
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
#ifdef __NetBSD__
# include <sys/videoio.h>
#else
# include <linux/videodev2.h>
#endif
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <System.h>
#include "camera.h"


/* Camera */
/* private */
/* types */
struct _Camera
{
	int fd;
	guint source;
	Buffer * buffer;

	/* widgets */
	GtkWidget * window;
	GtkWidget * area;
};


/* prototypes */
static int _camera_ioctl(Camera * camera, unsigned long request,
		void * data);

/* callbacks */
static gboolean _camera_on_closex(gpointer data);
static gboolean _camera_on_open(gpointer data);
static gboolean _camera_on_refresh(gpointer data);


/* public */
/* functions */
/* camera_new */
Camera * camera_new(void)
{
	Camera * camera;
	GtkWidget * vbox;

	if((camera = object_new(sizeof(*camera))) == NULL)
		return NULL;
	camera->fd = -1;
	camera->buffer = NULL;
	camera->source = 0;
	/* create the window */
	camera->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
#if GTK_CHECK_VERSION(2, 6, 0)
	gtk_window_set_icon_name(GTK_WINDOW(camera->window), "camera-video");
#endif
	gtk_window_set_resizable(GTK_WINDOW(camera->window), FALSE);
	gtk_window_set_title(GTK_WINDOW(camera->window), "Camera");
	g_signal_connect_swapped(camera->window, "delete-event", G_CALLBACK(
				_camera_on_closex), camera);
	vbox = gtk_vbox_new(FALSE, 0);
	camera->area = gtk_drawing_area_new();
	gtk_box_pack_start(GTK_BOX(vbox), camera->area, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(camera->window), vbox);
	gtk_widget_show_all(camera->window);
	camera->source = g_idle_add(_camera_on_open, camera);
	return camera;
}


/* camera_delete */
void camera_delete(Camera * camera)
{
	if(camera->window != NULL)
		gtk_widget_destroy(camera->window);
	if(camera->source != 0)
		g_source_remove(camera->source);
	if(camera->fd >= 0)
		close(camera->fd);
	buffer_delete(camera->buffer);
	object_delete(camera);
}


/* private */
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


/* camera_on_open */
static gboolean _camera_on_open(gpointer data)
{
	Camera * camera = data;
	/* XXX let this be configurable */
	char const device[] = "/dev/video0";
	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_format format;

	camera->source = 0;
	camera->fd = open(device, O_RDWR);
	camera->buffer = buffer_new(0, NULL);
	camera->source = 0;
	/* check for errors */
	if(camera->buffer == NULL || camera->fd < 0
			|| _camera_ioctl(camera, VIDIOC_QUERYCAP, &cap) == -1
			|| (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0
			/* FIXME also implement mmap() and streaming */
			|| (cap.capabilities & V4L2_CAP_READWRITE) == 0)
	{
		error_set_print("camera", 1, "%s: %s", device,
				"Could not open the video capture device");
		camera->source = g_timeout_add(1000, _camera_on_open, camera);
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
	if(_camera_on_refresh(camera) == TRUE)
		camera->source = g_timeout_add(1000, _camera_on_refresh,
				camera);
	return FALSE;
}


/* camera_on_refresh */
static gboolean _camera_on_refresh(gpointer data)
{
	Camera * camera = data;

	/* FIXME no longer block on read() */
	if(read(camera->fd, buffer_get_data(camera->buffer),
				buffer_get_size(camera->buffer)) <= 0)
	{
		/* this error can be ignored */
		if(errno == EAGAIN)
			return TRUE;
		close(camera->fd);
		camera->fd = -1;
		return FALSE;
	}
	/* FIXME implement the rest */
	return TRUE;
}
