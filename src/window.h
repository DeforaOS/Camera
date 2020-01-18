/* $Id$ */
/* Copyright (c) 2013-2014 Pierre Pronchery <khorben@defora.org> */
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



#ifndef CAMERA_WINDOW_H
# define CAMERA_WINDOW_H

# include "overlay.h"


/* public */
/* types */
typedef struct _CameraWindow CameraWindow;


/* functions */
CameraWindow * camerawindow_new(char const * device);
void camerawindow_delete(CameraWindow * camera);

/* accessors */
void camerawindow_set_aspect_ratio(CameraWindow * camera, gboolean ratio);

void camerawindow_set_fullscreen(CameraWindow * camera, int fullscreen);

void camerawindow_set_hflip(CameraWindow * camera, gboolean flip);
void camerawindow_set_vflip(CameraWindow * camera, gboolean flip);

/* useful */
CameraOverlay * camerawindow_add_overlay(CameraWindow * camera,
		char const * filename, int opacity);

int camerawindow_load(CameraWindow * window);
int camerawindow_save(CameraWindow * window);

#endif /* !CAMERA_WINDOW_H */
