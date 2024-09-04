/* $Id$ */
static char const _copyright[] =
"Copyright © 2012-2020 Pierre Pronchery <khorben@defora.org>";
/* This file is part of DeforaOS Desktop Camera */
static char const _license[] =
"Redistribution and use in source and binary forms, with or without\n"
"modification, are permitted provided that the following conditions are\n"
"met:\n"
"\n"
"1. Redistributions of source code must retain the above copyright notice,\n"
"   this list of conditions and the following disclaimer.\n"
"\n"
"2. Redistributions in binary form must reproduce the above copyright notice,\n"
"   this list of conditions and the following disclaimer in the documentation\n"
"   and/or other materials provided with the distribution.\n"
"\n"
"THIS SOFTWARE IS PROVIDED BY ITS AUTHORS AND CONTRIBUTORS \"AS IS\" AND ANY\n"
"EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED\n"
"WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE\n"
"DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR ANY\n"
"DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES\n"
"(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;\n"
"LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND\n"
"ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT\n"
"(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF\n"
"THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.";



#include <stdlib.h>
#include <libintl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <System.h>
#include <Desktop.h>
#include "camera.h"
#include "window.h"
#include "../config.h"
#define _(string) gettext(string)
#define N_(string) string

/* constants */
#ifndef PROGNAME_CAMERA
# define PROGNAME_CAMERA	"camera"
#endif


/* CameraWindow */
/* private */
/* types */
struct _CameraWindow
{
	Camera * camera;
	int fullscreen;

	/* widgets */
	GtkWidget * window;
#ifndef EMBEDDED
	GtkWidget * menubar;
#endif
};


/* prototypes */
/* callbacks */
static void _camerawindow_on_close(gpointer data);
static gboolean _camerawindow_on_closex(gpointer data);
static void _camerawindow_on_contents(gpointer data);
static void _camerawindow_on_fullscreen(gpointer data);
static gboolean _camerawindow_on_window_state(GtkWidget * widget,
		GdkEvent * event, gpointer data);

#ifndef EMBEDDED
/* menus */
static void _camerawindow_on_file_close(gpointer data);
static void _camerawindow_on_file_gallery(gpointer data);
static void _camerawindow_on_file_properties(gpointer data);
static void _camerawindow_on_file_snapshot(gpointer data);
static void _camerawindow_on_edit_preferences(gpointer data);
static void _camerawindow_on_view_fullscreen(gpointer data);
static void _camerawindow_on_help_about(gpointer data);
static void _camerawindow_on_help_contents(gpointer data);
#endif


/* constants */
#ifndef EMBEDDED
static char const * _authors[] =
{
	"Pierre Pronchery <khorben@defora.org>",
	NULL
};
#endif

static const DesktopAccel _camerawindow_accel[] =
{
#ifdef EMBEDDED
	{ G_CALLBACK(_camerawindow_on_close), GDK_CONTROL_MASK, GDK_KEY_W },
	{ G_CALLBACK(_camerawindow_on_contents), 0, GDK_KEY_F1 },
#endif
	{ G_CALLBACK(_camerawindow_on_fullscreen), 0, GDK_KEY_F11 },
	{ NULL, 0, 0 }
};

#ifndef EMBEDDED
/* menus */
static const DesktopMenu _camerawindow_menu_file[] =
{
	{ N_("Take _snapshot"), G_CALLBACK(_camerawindow_on_file_snapshot),
		"camera-photo", 0, 0 },
	{ "", NULL, NULL, 0, 0 },
	{ N_("_Gallery"), G_CALLBACK(_camerawindow_on_file_gallery),
		"image-x-generic", 0, 0 },
	{ "", NULL, NULL, 0, 0 },
	{ N_("_Properties"), G_CALLBACK(_camerawindow_on_file_properties),
		GTK_STOCK_PROPERTIES, GDK_MOD1_MASK, GDK_KEY_Return },
	{ "", NULL, NULL, 0, 0 },
	{ N_("_Close"), G_CALLBACK(_camerawindow_on_file_close),
		GTK_STOCK_CLOSE, GDK_CONTROL_MASK, GDK_KEY_W },
	{ NULL, NULL, NULL, 0, 0 }
};

static const DesktopMenu _camerawindow_menu_edit[] =
{
	{ N_("_Preferences"), G_CALLBACK(_camerawindow_on_edit_preferences),
		GTK_STOCK_PREFERENCES, GDK_CONTROL_MASK, GDK_KEY_P },
	{ NULL, NULL, NULL, 0, 0 }
};

static const DesktopMenu _camerawindow_menu_view[] =
{
	{ N_("_Fullscreen"), G_CALLBACK(_camerawindow_on_view_fullscreen),
#if GTK_CHECK_VERSION(2, 8, 0)
		GTK_STOCK_FULLSCREEN, 0, 0 },
#else
		NULL, 0, 0 },
#endif
	{ NULL, NULL, NULL, 0, 0 }
};

static const DesktopMenu _camerawindow_menu_help[] =
{
	{ N_("_Contents"), G_CALLBACK(_camerawindow_on_help_contents),
		"help-contents", 0, GDK_KEY_F1 },
#if GTK_CHECK_VERSION(2, 6, 0)
	{ N_("_About"), G_CALLBACK(_camerawindow_on_help_about),
		GTK_STOCK_ABOUT, 0, 0 },
#else
	{ N_("_About"), G_CALLBACK(_camerawindow_on_help_about), NULL, 0, 0 },
#endif
	{ NULL, NULL, NULL, 0, 0 }
};

static const DesktopMenubar _camerawindow_menubar[] =
{
	{ N_("_File"), _camerawindow_menu_file },
	{ N_("_Edit"), _camerawindow_menu_edit },
	{ N_("_View"), _camerawindow_menu_view },
	{ N_("_Help"), _camerawindow_menu_help },
	{ NULL, NULL }
};
#endif


/* public */
/* functions */
/* camerawindow_new */
CameraWindow * camerawindow_new(char const * device)
{
	CameraWindow * camera;
	GtkAccelGroup * group;
	GtkWidget * vbox;
	GtkWidget * widget;

	if((camera = object_new(sizeof(*camera))) == NULL)
		return NULL;
	group = gtk_accel_group_new();
	camera->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	camera->camera = NULL;
	camera->fullscreen = 0;
	if(camera->window != NULL)
	{
		gtk_widget_realize(camera->window);
		camera->camera = camera_new(camera->window, group, device);
	}
	if(camera->camera == NULL)
	{
		camerawindow_delete(camera);
		return NULL;
	}
	gtk_window_add_accel_group(GTK_WINDOW(camera->window), group);
	g_object_unref(group);
#ifndef EMBEDDED
# if GTK_CHECK_VERSION(2, 6, 0)
	gtk_window_set_icon_name(GTK_WINDOW(camera->window), "camera-web");
# endif
	gtk_window_set_title(GTK_WINDOW(camera->window), _("Webcam"));
#else
# if GTK_CHECK_VERSION(2, 6, 0)
	gtk_window_set_icon_name(GTK_WINDOW(camera->window), "camera-photo");
# endif
	gtk_window_set_title(GTK_WINDOW(camera->window), _("Camera"));
#endif
	gtk_window_set_default_size(GTK_WINDOW(camera->window), 640, 480);
	g_signal_connect_swapped(camera->window, "delete-event", G_CALLBACK(
				_camerawindow_on_closex), camera);
	g_signal_connect(camera->window, "window-state-event", G_CALLBACK(
				_camerawindow_on_window_state), camera);
#if GTK_CHECK_VERSION(3, 0, 0)
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
	vbox = gtk_vbox_new(FALSE, 0);
#endif
	desktop_accel_create(_camerawindow_accel, camera, group);
#ifndef EMBEDDED
	/* menubar */
	camera->menubar = desktop_menubar_create(_camerawindow_menubar, camera,
			group);
	gtk_box_pack_start(GTK_BOX(vbox), camera->menubar, FALSE, TRUE, 0);
#endif
	widget = camera_get_widget(camera->camera);
	gtk_box_pack_start(GTK_BOX(vbox), widget, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(camera->window), vbox);
	gtk_widget_show_all(camera->window);
	return camera;
}


/* camera_delete */
void camerawindow_delete(CameraWindow * camera)
{
	if(camera->camera != NULL)
		camera_delete(camera->camera);
	if(camera->window != NULL)
		gtk_widget_destroy(camera->window);
	object_delete(camera);
}


/* accessors */
/* camerawindow_set_aspect_ratio */
void camerawindow_set_aspect_ratio(CameraWindow * camera, gboolean ratio)
{
	camera_set_aspect_ratio(camera->camera, ratio);
}


/* camerawindow_set_fullscreen */
void camerawindow_set_fullscreen(CameraWindow * camera, int fullscreen)
{
	if(fullscreen)
		gtk_window_fullscreen(GTK_WINDOW(camera->window));
	else
		gtk_window_unfullscreen(GTK_WINDOW(camera->window));
}


/* camerawindow_set_hflip */
void camerawindow_set_hflip(CameraWindow * camera, gboolean flip)
{
	camera_set_hflip(camera->camera, flip);
}


/* camerawindow_set_vflip */
void camerawindow_set_vflip(CameraWindow * camera, gboolean flip)
{
	camera_set_vflip(camera->camera, flip);
}


/* useful */
/* camerawindow_add_overlay */
CameraOverlay * camerawindow_add_overlay(CameraWindow * camera,
		char const * filename, int opacity)
{
	return camera_add_overlay(camera->camera, filename, opacity);
}


/* camerawindow_load */
int camerawindow_load(CameraWindow * camera)
{
	return camera_load(camera->camera);
}


/* camerawindow_save */
int camerawindow_save(CameraWindow * camera)
{
	return camera_save(camera->camera);
}


/* private */
/* callbacks */
/* camerawindow_on_close */
static void _camerawindow_on_close(gpointer data)
{
	CameraWindow * camera = data;

	gtk_widget_hide(camera->window);
	camera_stop(camera->camera);
	gtk_main_quit();
}


/* camerawindow_on_closex */
static gboolean _camerawindow_on_closex(gpointer data)
{
	CameraWindow * camera = data;

	_camerawindow_on_close(camera);
	return TRUE;
}


/* camerawindow_on_contents */
static void _camerawindow_on_contents(gpointer data)
{
	(void) data;

	desktop_help_contents(PACKAGE, PROGNAME_CAMERA);
}


/* camerawindow_on_fullscreen */
static void _camerawindow_on_fullscreen(gpointer data)
{
	CameraWindow * camera = data;

	camerawindow_set_fullscreen(camera, camera->fullscreen ? 0 : 1);
}


/* camerawindow_on_window_state */
static gboolean _camerawindow_on_window_state(GtkWidget * widget,
		GdkEvent * event, gpointer data)
{
	CameraWindow * camera = data;
	GdkEventWindowState * gews;
	(void) widget;

	if(event->type == GDK_WINDOW_STATE)
	{
		gews = &event->window_state;
		camera->fullscreen = (gews->new_window_state
				& GDK_WINDOW_STATE_FULLSCREEN) ? 1 : 0;
#ifndef EMBEDDED
		if(camera->fullscreen)
			gtk_widget_hide(camera->menubar);
		else
			gtk_widget_show(camera->menubar);
#endif
	}
	return FALSE;
}


#ifndef EMBEDDED
/* menus */
/* camerawindow_on_file_close */
static void _camerawindow_on_file_close(gpointer data)
{
	CameraWindow * camera = data;

	_camerawindow_on_close(camera);
}


/* camerawindow_on_file_gallery */
static void _camerawindow_on_file_gallery(gpointer data)
{
	CameraWindow * camera = data;

	camera_open_gallery(camera->camera);
}


/* camerawindow_on_file_properties */
static void _camerawindow_on_file_properties(gpointer data)
{
	CameraWindow * camera = data;

	camera_show_properties(camera->camera, TRUE);
}


/* camerawindow_on_file_snapshot */
static void _camerawindow_on_file_snapshot(gpointer data)
{
	CameraWindow * camera = data;

	camera_snapshot(camera->camera, CSF_PNG);
}


/* camerawindow_on_edit_preferences */
static void _camerawindow_on_edit_preferences(gpointer data)
{
	CameraWindow * camera = data;

	camera_show_preferences(camera->camera, TRUE);
}


/* camerawindow_on_view_fullscreen */
static void _camerawindow_on_view_fullscreen(gpointer data)
{
	CameraWindow * camera = data;

	_camerawindow_on_fullscreen(camera);
}


/* camerawindow_on_help_about */
static void _camerawindow_on_help_about(gpointer data)
{
	CameraWindow * camera = data;
	GtkWidget * widget;
	char const comments[] =
#ifndef EMBEDDED
		N_("Simple webcam application for the DeforaOS desktop");
#else
		N_("Simple camera application for the DeforaOS desktop");
#endif

	widget = desktop_about_dialog_new();
	gtk_window_set_transient_for(GTK_WINDOW(widget), GTK_WINDOW(
				camera->window));
	desktop_about_dialog_set_authors(widget, _authors);
	desktop_about_dialog_set_comments(widget, _(comments));
	desktop_about_dialog_set_copyright(widget, _copyright);
	desktop_about_dialog_set_license(widget, _license);
#ifndef EMBEDDED
	desktop_about_dialog_set_logo_icon_name(widget, "camera-web");
#else
	desktop_about_dialog_set_logo_icon_name(widget, "camera-photo");
#endif
	desktop_about_dialog_set_name(widget, PACKAGE);
	desktop_about_dialog_set_version(widget, VERSION);
	desktop_about_dialog_set_website(widget, "http://www.defora.org/");
	gtk_dialog_run(GTK_DIALOG(widget));
	gtk_widget_destroy(widget);
}


/* camerawindow_on_help_contents */
static void _camerawindow_on_help_contents(gpointer data)
{
	CameraWindow * camera = data;

	_camerawindow_on_contents(camera);
}
#endif
