/* $Id$ */
/* Copyright (c) 2012-2020 Pierre Pronchery <khorben@defora.org> */
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



#ifndef CAMERA_CAMERA_H
# define CAMERA_CAMERA_H

# include "overlay.h"


/* Camera */
/* defaults */
# define CAMERA_CONFIG_FILE	".camera"


/* public */
/* types */
typedef struct _Camera Camera;

typedef enum _CameraSnapshotFormat
{
	CSF_DEFAULT = 0,
	CSF_PNG,
	CSF_JPEG
} CameraSnapshotFormat;
# define CSF_LAST CSF_JPEG
# define CSF_COUNT (CSF_LAST + 1)


/* functions */
Camera * camera_new(GtkWidget * window, GtkAccelGroup * group,
		char const * device);
void camera_delete(Camera * camera);

/* accessors */
char const * camera_get_device(Camera * camera);
GtkWidget * camera_get_widget(Camera * camera);

void camera_set_aspect_ratio(Camera * camera, gboolean ratio);
int camera_set_device(Camera * camera, char const * device);
void camera_set_hflip(Camera * camera, gboolean flip);
void camera_set_vflip(Camera * camera, gboolean flip);

/* useful */
void camera_open_gallery(Camera * camera);

int camera_snapshot(Camera * camera, CameraSnapshotFormat format);

void camera_show_preferences(Camera * camera, gboolean show);
void camera_show_properties(Camera * camera, gboolean show);

int camera_load(Camera * camera);
int camera_save(Camera * camera);

void camera_start(Camera * camera);
void camera_stop(Camera * camera);

CameraOverlay * camera_add_overlay(Camera * camera, char const * filename,
		int opacity);

#endif /* !CAMERA_CAMERA_H */
