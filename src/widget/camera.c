/* $Id$ */
/* Copyright (c) 2015-2020 Pierre Pronchery <khorben@defora.org> */
/* This file is part of DeforaOS Desktop Camera */
/* Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY ITS AUTHORS AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */



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

static int _camera_set_property(CameraWidget * camera, va_list ap);


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
	_camera_get_widget,
	_camera_set_property
};


/* private */
/* functions */
/* camera_init */
static CameraWidget * _camera_init(char const * name)
{
	CameraWidget * camera;
	(void) name;

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


/* camera_set_property */
static int _camera_set_property(CameraWidget * camera, va_list ap)
{
	String const * property;
	gboolean b;

	while((property = va_arg(ap, String const *)) != NULL)
		if(strcmp(property, "aspect-ratio") == 0)
		{
			b = va_arg(ap, gboolean);
			camera_set_aspect_ratio(camera->camera, b);
		}
		else if(strcmp(property, "hflip") == 0)
		{
			b = va_arg(ap, gboolean);
			camera_set_hflip(camera->camera, b);
		}
		else if(strcmp(property, "vflip") == 0)
		{
			b = va_arg(ap, gboolean);
			camera_set_vflip(camera->camera, b);
		}
	return 0;
}
