/* $Id$ */
/* Copyright (c) 2013 Pierre Pronchery <khorben@defora.org> */
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
