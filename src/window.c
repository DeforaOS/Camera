/* $Id$ */
static char const _copyright[] =
"Copyright © 2012-2013 Pierre Pronchery <khorben@defora.org>";
/* This file is part of DeforaOS Desktop Camera */
static char const _license[] =
"This program is free software: you can redistribute it and/or modify\n"
"it under the terms of the GNU General Public License as published by\n"
"the Free Software Foundation, version 3 of the License.\n"
"\n"
"This program is distributed in the hope that it will be useful,\n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
"GNU General Public License for more details.\n"
"\n"
"You should have received a copy of the GNU General Public License\n"
"along with this program.  If not, see <http://www.gnu.org/licenses/>.";



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
#define N_(string) (string)

/* constants */
#ifndef PROGNAME
# define PROGNAME	"camera"
#endif


/* CameraWindow */
/* private */
/* types */
struct _CameraWindow
{
	Camera * camera;

	/* widgets */
	GtkWidget * window;
};


/* prototypes */
/* callbacks */
static void _camerawindow_on_close(gpointer data);
static gboolean _camerawindow_on_closex(gpointer data);
static void _camerawindow_on_contents(gpointer data);

#ifndef EMBEDDED
/* menus */
static void _camerawindow_on_file_close(gpointer data);
static void _camerawindow_on_file_gallery(gpointer data);
static void _camerawindow_on_file_properties(gpointer data);
static void _camerawindow_on_file_snapshot(gpointer data);
static void _camerawindow_on_edit_preferences(gpointer data);
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

#ifdef EMBEDDED
static const DesktopAccel _camerawindow_accel[] =
{
	{ G_CALLBACK(_camerawindow_on_close), GDK_CONTROL_MASK, GDK_KEY_W },
	{ G_CALLBACK(_camerawindow_on_contents), 0, GDK_KEY_F1 },
	{ NULL, 0, 0 }
};
#endif

#ifndef EMBEDDED
/* menus */
static const DesktopMenu _camerawindow_menu_file[] =
{
	{ "Take _snapshot", G_CALLBACK(_camerawindow_on_file_snapshot),
		"camera-photo", 0, 0 },
	{ "", NULL, NULL, 0, 0 },
	{ "_Gallery", G_CALLBACK(_camerawindow_on_file_gallery),
		"image-x-generic", 0, 0 },
	{ "", NULL, NULL, 0, 0 },
	{ "_Properties", G_CALLBACK(_camerawindow_on_file_properties),
		GTK_STOCK_PROPERTIES, GDK_MOD1_MASK, GDK_KEY_Return },
	{ "", NULL, NULL, 0, 0 },
	{ "_Close", G_CALLBACK(_camerawindow_on_file_close), GTK_STOCK_CLOSE,
		GDK_CONTROL_MASK, GDK_KEY_W },
	{ NULL, NULL, NULL, 0, 0 }
};

static const DesktopMenu _camerawindow_menu_edit[] =
{
	{ "_Preferences", G_CALLBACK(_camerawindow_on_edit_preferences),
		GTK_STOCK_PREFERENCES, GDK_CONTROL_MASK, GDK_KEY_P },
	{ NULL, NULL, NULL, 0, 0 }
};

static const DesktopMenu _camerawindow_menu_help[] =
{
	{ "_Contents", G_CALLBACK(_camerawindow_on_help_contents),
		"help-contents", 0, GDK_KEY_F1 },
	{ "_About", G_CALLBACK(_camerawindow_on_help_about), GTK_STOCK_ABOUT, 0, 0 },
	{ NULL, NULL, NULL, 0, 0 }
};

static const DesktopMenubar _camerawindow_menubar[] =
{
	{ "_File", _camerawindow_menu_file },
	{ "_Edit", _camerawindow_menu_edit },
	{ "_Help", _camerawindow_menu_help },
	{ NULL, NULL }
};
#endif


/* public */
/* functions */
/* camerawindow_new */
CameraWindow * camerawindow_new(char const * device, int flip)
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
	if(camera->window != NULL)
	{
		gtk_widget_realize(camera->window);
		camera->camera = camera_new(camera->window, group, device,
				flip);
	}
	if(camera->camera == NULL)
	{
		camerawindow_delete(camera);
		return NULL;
	}
	gtk_window_add_accel_group(GTK_WINDOW(camera->window), group);
	g_object_unref(group);
#if GTK_CHECK_VERSION(2, 6, 0)
	gtk_window_set_icon_name(GTK_WINDOW(camera->window), "camera-photo");
#endif
	gtk_window_set_title(GTK_WINDOW(camera->window), "Camera");
	g_signal_connect_swapped(camera->window, "delete-event", G_CALLBACK(
				_camerawindow_on_closex), camera);
#if GTK_CHECK_VERSION(3, 0, 0)
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
	vbox = gtk_vbox_new(FALSE, 0);
#endif
#ifdef EMBEDDED
	desktop_accel_create(_camerawindow_accel, camera, group);
#endif
#ifndef EMBEDDED
	/* menubar */
	widget = desktop_menubar_create(_camerawindow_menubar, camera, group);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
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


/* useful */
/* camerawindow_add_overlay */
CameraOverlay * camerawindow_add_overlay(CameraWindow * camera,
		char const * filename, int opacity)
{
	return camera_add_overlay(camera->camera, filename, opacity);
}


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
	desktop_help_contents(PACKAGE, PROGNAME);
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

	camera_properties(camera->camera);
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

	camera_preferences(camera->camera);
}


/* camerawindow_on_help_about */
static void _camerawindow_on_help_about(gpointer data)
{
	CameraWindow * camera = data;
	GtkWidget * widget;
	char const comments[] = N_("Simple camera application for the DeforaOS"
			" desktop");

	widget = desktop_about_dialog_new();
	gtk_window_set_transient_for(GTK_WINDOW(widget), GTK_WINDOW(
				camera->window));
	desktop_about_dialog_set_authors(widget, _authors);
	desktop_about_dialog_set_comments(widget, _(comments));
	desktop_about_dialog_set_copyright(widget, _copyright);
	desktop_about_dialog_set_license(widget, _license);
	desktop_about_dialog_set_logo_icon_name(widget, "camera-photo");
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
