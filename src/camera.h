/* $Id$ */
/* Copyright (c) 2012-2013 Pierre Pronchery <khorben@defora.org> */
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



#ifndef CAMERA_CAMERA_H
# define CAMERA_CAMERA_H

# include "overlay.h"


/* public */
/* types */
typedef struct _Camera Camera;

typedef enum _CameraSnapshotFormat
{
	CSF_PNG = 0,
	CSF_JPEG
} CameraSnapshotFormat;


/* functions */
Camera * camera_new(GtkWidget * window, GtkAccelGroup * group,
		char const * device, int flip);
void camera_delete(Camera * camera);

/* accessors */
GtkWidget * camera_get_widget(Camera * camera);

/* useful */
void camera_open_gallery(Camera * camera);

void camera_preferences(Camera * camera);
void camera_properties(Camera * camera);
int camera_snapshot(Camera * camera, CameraSnapshotFormat format);

void camera_start(Camera * camera);
void camera_stop(Camera * camera);

CameraOverlay * camera_add_overlay(Camera * camera, char const * filename,
		int opacity);

#endif /* !CAMERA_CAMERA_H */
