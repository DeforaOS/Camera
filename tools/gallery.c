/* $Id$ */
/* Copyright (c) 2013-2020 Pierre Pronchery <khorben@defora.org> */
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
#ifndef PROGNAME_BROWSER
# define PROGNAME_BROWSER	"browser"
#endif
#ifndef PROGNAME_GALLERY
# define PROGNAME_GALLERY	"gallery"
#endif
#ifndef PREFIX
# define PREFIX			"/usr/local"
#endif
#ifndef BINDIR
# define BINDIR			PREFIX "/bin"
#endif
#ifndef DATADIR
# define DATADIR		PREFIX "/share"
#endif
#ifndef LOCALEDIR
# define LOCALEDIR		DATADIR "/locale"
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
	char const browser[] = BINDIR "/" PROGNAME_BROWSER;
	char const dcim[] = "DCIM";
	char * path;
#if GTK_CHECK_VERSION(2, 6, 0)
	char * argv[] = { PROGNAME_BROWSER, "-T", "--", NULL, NULL };
	const int arg = 3;
#else
	char * argv[] = { PROGNAME_BROWSER, "--", NULL, NULL };
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
	fprintf(stderr, "%s: %s%s%s\n", PROGNAME_GALLERY,
			(message != NULL) ? message : "",
			(message != NULL) ? ": " : "", strerror(errno));
	return ret;
}


/* usage */
static int _usage(void)
{
	fprintf(stderr, _("Usage: %s\n"), PROGNAME_GALLERY);
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
