/* $Id$ */
/* Copyright (c) 2013-2015 Pierre Pronchery <khorben@defora.org> */
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
#include <locale.h>
#include <libintl.h>
#include <errno.h>
#include <glib.h>
#include <gtk/gtk.h> /* for GTK_CHECK_VERSION */
#include "../config.h"
#define _(string) gettext(string)

/* constants */
#ifndef PROGNAME
# define PROGNAME	"gallery"
#endif
#ifndef BROWSER_PROGNAME
# define BROWSER_PROGNAME	"browser"
#endif
#ifndef PREFIX
# define PREFIX		"/usr/local"
#endif
#ifndef BINDIR
# define BINDIR		PREFIX "/bin"
#endif
#ifndef DATADIR
# define DATADIR	PREFIX "/share"
#endif
#ifndef LOCALEDIR
# define LOCALEDIR	DATADIR "/locale"
#endif


/* Gallery */
/* private */
/* prototypes */
static int _gallery(void);

static int _error(char const * message, int ret);
static int _usage(void);


/* functions */
/* gallery */
static int _gallery(void)
{
	char const * homedir;
	char const browser[] = BINDIR "/" BROWSER_PROGNAME;
	char const dcim[] = "DCIM";
	char * path;
#if GTK_CHECK_VERSION(2, 6, 0)
	char * argv[] = { BROWSER_PROGNAME, "-T", "--", NULL, NULL };
	const int arg = 3;
#else
	char * argv[] = { BROWSER_PROGNAME, "--", NULL, NULL };
	const int arg = 2;
#endif

	if((homedir = getenv("HOME")) == NULL)
		homedir = g_get_home_dir();
	if((path = g_build_filename(homedir, dcim, NULL)) == NULL)
		return -_error(NULL, 1);
	/* this error should be caught by the final program */
	mkdir(path, 0777);
	argv[arg] = path;
	execv(browser, argv);
	_error(browser, 1);
	g_free(path);
	return -1;
}


/* error */
static int _error(char const * message, int ret)
{
	fprintf(stderr, "%s: %s%s%s\n", PROGNAME,
			(message != NULL) ? message : "",
			(message != NULL) ? ": " : "", strerror(errno));
	return ret;
}


/* usage */
static int _usage(void)
{
	fprintf(stderr, _("Usage: %s\n"), PROGNAME);
	return 1;
}


/* public */
/* functions */
/* main */
int main(int argc, char * argv[])
{
	int o;

	if(setlocale(LC_ALL, "") == NULL)
		_error("setlocale", 1);
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
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
