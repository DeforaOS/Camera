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



#include <unistd.h>
#include <stdio.h>
#include <locale.h>
#include <libintl.h>
#include <gtk/gtk.h>
#include <System.h>
#include "camera.h"
#include "../config.h"
#define _(string) gettext(string)


/* constants */
#ifndef PREFIX
# define PREFIX		"/usr/local"
#endif
#ifndef DATADIR
# define DATADIR	PREFIX "/share"
#endif
#ifndef LOCALEDIR
# define LOCALEDIR	DATADIR "/locale"
#endif


/* private */
/* prototypes */
static int _camera(char const * device, char const * overlay);

static int _usage(void);


/* functions */
/* camera */
static int _camera(char const * device, char const * overlay)
{
	Camera * camera;

	if((camera = camera_new(device)) == NULL)
		return error_print(PACKAGE);
	if(overlay != NULL)
		camera_add_overlay(camera, overlay, 50);
	gtk_main();
	camera_delete(camera);
	return 0;
}


/* usage */
static int _usage(void)
{
	fputs(_("Usage: camera [-d device][-O filename]\n"
"  -d	Video device to open\n"
"  -O	Use this file as an overlay\n"), stderr);
	return 1;
}


/* public */
/* functions */
/* main */
int main(int argc, char * argv[])
{
	int o;
	char const * device = NULL;
	char const * overlay = NULL;

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	gtk_init(&argc, &argv);
	while((o = getopt(argc, argv, "d:O:")) != -1)
		switch(o)
		{
			case 'd':
				device = optarg;
				break;
			case 'O':
				overlay = optarg;
				break;
			default:
				return _usage();
		}
	if(optind != argc)
		return _usage();
	return (_camera(device, overlay) == 0) ? 0 : 2;
}
