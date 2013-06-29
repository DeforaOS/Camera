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



#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <glib.h>
#include <gtk/gtk.h>


/* Gallery */
/* private */
/* prototypes */
static int _gallery(void);


/* functions */
/* gallery */
static int _gallery(void)
{
	char const gallery[] = "gallery";
	char const * homedir;
	char const dcim[] = "DCIM";
	char * path;
#if GTK_CHECK_VERSION(2, 6, 0)
	char * argv[] = { "browser", "-T", "--", NULL, NULL };
	const int arg = 3;
#else
	char * argv[] = { "browser", "--", NULL, NULL };
	const int arg = 2;
#endif

	if((homedir = getenv("HOME")) == NULL)
		homedir = g_get_home_dir();
	if((path = g_build_filename(homedir, dcim, NULL)) == NULL)
	{
		/* XXX errno may not be set */
		fprintf(stderr, "%s: %s\n", gallery, strerror(errno));
		return -1;
	}
	/* this error should be caught by the final program */
	mkdir(path, 0777);
	argv[arg] = path;
	execvp(argv[0], argv);
	fprintf(stderr, "%s: %s: %s\n", gallery, argv[0], strerror(errno));
	g_free(path);
	return -1;
}


/* usage */
static int _usage(void)
{
	fputs("Usage: gallery\n", stderr);
	return 1;
}


/* public */
/* functions */
/* main */
int main(int argc, char * argv[])
{
	int o;

	while((o = getopt(argc, argv, "")) != -1)
		switch(o)
		{
			default:
				return _usage();
		}
	if(optind != argc)
		return _usage();
	return (_gallery() == 0) ? 0 : 2;
}
