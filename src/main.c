/* $Id$ */
/* Copyright (c) 2012-2018 Pierre Pronchery <khorben@defora.org> */
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



#include <unistd.h>
#include <stdio.h>
#include <locale.h>
#include <libintl.h>
#include <gtk/gtk.h>
#if GTK_CHECK_VERSION(3, 0, 0)
# include <gtk/gtkx.h>
#endif
#include <System.h>
#include "camera.h"
#include "window.h"
#include "../config.h"
#define _(string) gettext(string)

/* constants */
#ifndef PROGNAME_CAMERA
# define PROGNAME_CAMERA	"camera"
#endif
#ifndef PREFIX
# define PREFIX			"/usr/local"
#endif
#ifndef DATADIR
# define DATADIR		PREFIX "/share"
#endif
#ifndef LOCALEDIR
# define LOCALEDIR		DATADIR "/locale"
#endif


/* private */
/* prototypes */
static int _camera(int embedded, char const * device, int hflip, int vflip,
		int ratio, char const * overlay);

static int _error(char const * message, int ret);
static int _usage(void);


/* functions */
/* camera */
static int _camera_embedded(char const * device, int hflip, int vflip,
		int ratio, char const * overlay);
static void _embedded_on_embedded(gpointer data);

static int _camera(int embedded, char const * device, int hflip, int vflip,
		int ratio, char const * overlay)
{
	CameraWindow * camera;

	if(embedded != 0)
		return _camera_embedded(device, hflip, vflip, ratio, overlay);
	if((camera = camerawindow_new(device)) == NULL)
		return error_print(PACKAGE);
	camerawindow_load(camera);
	if(hflip >= 0)
		camerawindow_set_hflip(camera, hflip ? TRUE : FALSE);
	if(vflip >= 0)
		camerawindow_set_vflip(camera, vflip ? TRUE : FALSE);
	if(ratio >= 0)
		camerawindow_set_aspect_ratio(camera, ratio ? TRUE : FALSE);
	if(overlay != NULL)
		camerawindow_add_overlay(camera, overlay, 50);
	gtk_main();
	camerawindow_delete(camera);
	return 0;
}

static int _camera_embedded(char const * device, int hflip, int vflip,
		int ratio, char const * overlay)
{
	GtkWidget * window;
	GtkWidget * widget;
	Camera * camera;
	unsigned long id;

	window = gtk_plug_new(0);
	gtk_widget_realize(window);
	g_signal_connect_swapped(window, "embedded", G_CALLBACK(
				_embedded_on_embedded), window);
	if((camera = camera_new(window, NULL, device)) == NULL)
	{
		error_print(PACKAGE);
		gtk_widget_destroy(window);
		return -1;
	}
	if(hflip >= 0)
		camera_set_hflip(camera, hflip ? TRUE : FALSE);
	if(vflip >= 0)
		camera_set_vflip(camera, vflip ? TRUE : FALSE);
	if(ratio >= 0)
		camera_set_aspect_ratio(camera, ratio ? TRUE : FALSE);
	if(overlay != NULL)
		camera_add_overlay(camera, overlay, 50);
	widget = camera_get_widget(camera);
	gtk_container_add(GTK_CONTAINER(window), widget);
	id = gtk_plug_get_id(GTK_PLUG(window));
	printf("%lu\n", id);
	fclose(stdout);
	gtk_main();
	camera_delete(camera);
	gtk_widget_destroy(window);
	return 0;
}

static void _embedded_on_embedded(gpointer data)
{
	GtkWidget * widget = data;

	gtk_widget_show(widget);
}


/* error */
static int _error(char const * message, int ret)
{
	fputs(PROGNAME_CAMERA ": ", stderr);
	perror(message);
	return ret;
}


/* usage */
static int _usage(void)
{
	fprintf(stderr, _("Usage: %s [-d device][-O filename][-HhRrVvx]\n"
"  -d	Video device to open\n"
"  -H	Flip horizontally\n"
"  -h	Do not flip horizontally\n"
"  -O	Use this file as an overlay\n"
"  -R	Preserve the aspect ratio when scaling\n"
"  -r	Do not preserve the aspect ratio when scaling\n"
"  -V	Flip vertically\n"
"  -v	Do not flip vertically\n"
"  -x	Start in embedded mode\n"), PROGNAME_CAMERA);
	return 1;
}


/* public */
/* functions */
/* main */
int main(int argc, char * argv[])
{
	int o;
	int embedded = 0;
	char const * device = NULL;
	int hflip = -1;
	int vflip = -1;
	int ratio = -1;
	char const * overlay = NULL;

	if(setlocale(LC_ALL, "") == NULL)
		_error("setlocale", 1);
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	gtk_init(&argc, &argv);
	while((o = getopt(argc, argv, "d:HhO:RrVvx")) != -1)
		switch(o)
		{
			case 'd':
				device = optarg;
				break;
			case 'H':
				hflip = 1;
				break;
			case 'h':
				hflip = 0;
				break;
			case 'O':
				overlay = optarg;
				break;
			case 'R':
				ratio = 1;
				break;
			case 'r':
				ratio = 0;
				break;
			case 'V':
				vflip = 1;
				break;
			case 'v':
				vflip = 0;
				break;
			case 'x':
				embedded = 1;
				break;
			default:
				return _usage();
		}
	if(optind != argc)
		return _usage();
	return (_camera(embedded, device, hflip, vflip, ratio, overlay) == 0)
		? 0 : 2;
}
