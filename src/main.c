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



#include <unistd.h>
#include <stdio.h>
#include <gtk/gtk.h>
#include <System.h>
#include "camera.h"
#include "../config.h"


/* private */
/* prototypes */
static int _camera(void);

static int _usage(void);


/* functions */
/* camera */
static int _camera(void)
{
	Camera * camera;

	if((camera = camera_new()) == NULL)
		return error_print(PACKAGE);
	gtk_main();
	camera_delete(camera);
	return 0;
}


/* usage */
static int _usage(void)
{
	fputs("Usage: camera\n", stderr);
	return 1;
}


/* public */
/* functions */
/* main */
int main(int argc, char * argv[])
{
	int o;

	gtk_init(&argc, &argv);
	while((o = getopt(argc, argv, "")) != -1)
		switch(o)
		{
			default:
				return _usage();
		}
	return (_camera() == 0) ? 0 : 2;
}
