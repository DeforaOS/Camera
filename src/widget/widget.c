/* $Id$ */
/* Copyright (c) 2015 Pierre Pronchery <khorben@defora.org> */
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



#include <gtk/gtk.h>
#include <System.h>
#include <Desktop.h>
#include "../overlay.h"
#include "../camera.h"

#include "../overlay.c"
#include "../camera.c"


/* CameraWidget */
/* private */
/* types */
typedef struct _DesktopWidgetPlugin
{
	GtkWidget * window;
	Camera * camera;
} CameraWidget;


/* prototypes */
static CameraWidget * _camera_init(char const * name);
static void _camera_destroy(CameraWidget * camera);

static GtkWidget * _camera_get_widget(CameraWidget * camera);


/* public */
/* variables */
DesktopWidgetDefinition widget =
{
	"Camera",
#ifndef EMBEDDED
	"camera-web",
#else
	"camera-photo",
#endif
	NULL,
	_camera_init,
	_camera_destroy,
	_camera_get_widget
};


/* private */
/* functions */
/* camera_init */
static CameraWidget * _camera_init(char const * name)
{
	CameraWidget * camera;

	if((camera = object_new(sizeof(*camera))) == NULL)
		return NULL;
	camera->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_realize(camera->window);
	if((camera->camera = camera_new(camera->window, NULL, NULL)) == NULL)
	{
		_camera_destroy(camera);
		return NULL;
	}
	return camera;
}


/* camera_destroy */
static void _camera_destroy(CameraWidget * camera)
{
	if(camera->camera != NULL)
		camera_delete(camera->camera);
	if(camera->window)
		gtk_widget_destroy(camera->window);
	object_delete(camera);
}


/* accessors */
/* camera_get_widget */
static GtkWidget * _camera_get_widget(CameraWidget * camera)
{
	return camera_get_widget(camera->camera);
}
