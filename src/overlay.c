/* $Id$ */
/* Copyright (c) 2013 Pierre Pronchery <khorben@defora.org> */
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



#include <System.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "overlay.h"


/* CameraOverlay */
/* protected */
/* types */
struct _CameraOverlay
{
	GdkPixbuf * pixbuf;
	int width;
	int height;
	int opacity;
};


/* public */
/* functions */
/* cameraoverlay_new */
CameraOverlay * cameraoverlay_new(char const * filename, int opacity)
{
	CameraOverlay * overlay;
	GError * error = NULL;

	if((overlay = object_new(sizeof(*overlay))) == NULL)
		return NULL;
	if((overlay->pixbuf = gdk_pixbuf_new_from_file(filename, &error))
			== NULL)
	{
		error_set("%s", error->message);
		g_error_free(error);
		cameraoverlay_delete(overlay);
		return NULL;
	}
	overlay->width = gdk_pixbuf_get_width(overlay->pixbuf);
	overlay->height = gdk_pixbuf_get_height(overlay->pixbuf);
	overlay->opacity = opacity;
	return overlay;
}


/* cameraoverlay_delete */
void cameraoverlay_delete(CameraOverlay * overlay)
{
	if(overlay->pixbuf != NULL)
		g_object_unref(overlay->pixbuf);
	object_delete(overlay);
}


/* accessors */
/* cameraoverlay_get_opacity */
int cameraoverlay_get_opacity(CameraOverlay * overlay)
{
	return overlay->opacity;
}


/* cameraoverlay_set_opacity */
void cameraoverlay_set_opacity(CameraOverlay * overlay, int opacity)
{
	overlay->opacity = opacity;
}


/* useful */
void cameraoverlay_blit(CameraOverlay * overlay, GdkPixbuf * dest)
{
	int width = gdk_pixbuf_get_width(dest);
	int height = gdk_pixbuf_get_height(dest);
	gdouble wratio = width;
	gdouble hratio = height;

	wratio /= overlay->width;
	hratio /= overlay->height;
	gdk_pixbuf_composite(overlay->pixbuf, dest, 0, 0,
			width, height, 0, 0, wratio, hratio,
			GDK_INTERP_BILINEAR, overlay->opacity);
}
