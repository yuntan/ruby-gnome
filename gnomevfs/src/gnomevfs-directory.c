/*****************************************************************************
 *
 * gnomevfs-directory.c: GnomeVFS Directory class.
 *
 * Copyright (C) 2003 Nikolai :: lone-star :: Weibull <lone-star@home.se>.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 * Author: Nikolai :: lone-star :: Weibull <lone-star@home.se>
 *
 * Latest Revision: 2003-07-24
 *
 *****************************************************************************/

/* Includes ******************************************************************/

#include "gnomevfs.h"

/* Defines *******************************************************************/

#define _SELF(s)	\
     ((GnomeVFSDirectoryHandle *)RVAL2BOXED(s, GNOMEVFS_TYPE_DIRECTORY))

/* Type Definitions **********************************************************/

/* Function Declarations *****************************************************/

/* Global Variables **********************************************************/

/* Function Implementations **************************************************/

static VALUE
make_directory(argc, argv, self)
	int argc;
	VALUE *argv;
	VALUE self;
{
	VALUE uri;
	VALUE v_perm;
	guint perm;
	GnomeVFSResult result;

	if (rb_scan_args(argc, argv, "11", &uri, &v_perm) == 2) {
		perm = NUM2UINT(v_perm);
	} else {
		perm = 0777;
	}

	if (RTEST(rb_obj_is_kind_of(uri, g_gvfs_uri))) {
		result = gnome_vfs_make_directory_for_uri(RVAL2GVFSURI(uri),
							  perm);
	} else {
		result = gnome_vfs_make_directory(RVAL2CSTR(uri), perm);
	}

	return GVFSRESULT2RVAL(result);
}

static VALUE
remove_directory(self, arg)
	VALUE self, arg;
{
	GnomeVFSResult result;

	if (RTEST(rb_obj_is_kind_of(arg, g_gvfs_uri))) {
		result = gnome_vfs_remove_directory_from_uri(
							RVAL2GVFSURI(arg));
	} else {
		result = gnome_vfs_remove_directory(RVAL2CSTR(arg));
	}

	return GVFSRESULT2RVAL(result);
}

static GnomeVFSDirectoryHandle *
gnome_vfs_directory_copy(gpointer boxed)
{
	return (GnomeVFSDirectoryHandle *)boxed;
}

void
gnome_vfs_directory_free(gpointer boxed)
{
	/*
	 * do nothing for now...
	 * we can probably say that #close() is enough
	 */
}

GType
gnome_vfs_directory_get_type(void)
{
	static GType our_type = 0;

	if (our_type == 0) {
		our_type = g_boxed_type_register_static("GnomeVFSDirectory",
				(GBoxedCopyFunc)gnome_vfs_directory_copy,
				(GBoxedFreeFunc)gnome_vfs_directory_free);
	}

	return our_type;
}

static VALUE
directory_initialize(argc, argv, self)
	int argc;
	VALUE *argv;
	VALUE self;
{
	VALUE uri, r_options;
	GnomeVFSFileInfoOptions options;
	GnomeVFSDirectoryHandle *handle;
	GnomeVFSResult result;
	VALUE r_result;

	if (rb_scan_args(argc, argv, "11", &uri, &r_options) == 2) {
		options = FIX2INT(r_options);
	} else {
		/* XXX: or? */
		options = GNOME_VFS_FILE_INFO_DEFAULT;
	}

	if (RTEST(rb_obj_is_kind_of(uri, g_gvfs_uri))) {
		result = gnome_vfs_directory_open_from_uri(&handle,
						RVAL2GVFSURI(uri), options);
	} else {
		result = gnome_vfs_directory_open(&handle, RVAL2CSTR(uri),
						  options);
	}

	/* ugly, but raise error */
	r_result = GVFSRESULT2RVAL(result);

	G_INITIALIZE(self, handle);
	return Qnil;
}

static VALUE
directory_close(self)
{
	return GVFSRESULT2RVAL(gnome_vfs_directory_close(_SELF(self)));
}

static VALUE
directory_open(argc, argv, self)
	int argc;
	VALUE *argv;
	VALUE self;
{
	VALUE dir;

	dir = rb_funcall2(self, rb_intern("new"), argc, argv);
	if (rb_block_given_p()) {
		rb_ensure(rb_yield, dir, directory_close, dir);
		return Qnil;
	}
	return dir;
}

static VALUE
directory_each(self)
	VALUE self;
{
	GnomeVFSDirectoryHandle *handle = _SELF(self);
	GnomeVFSResult result = GNOME_VFS_OK;
	VALUE r_result;

	while (TRUE) {
		GnomeVFSFileInfo *info;

		info = gnome_vfs_file_info_new();
		result = gnome_vfs_directory_read_next(handle, info);
		if (result != GNOME_VFS_OK) {
			gnome_vfs_file_info_unref(info);
			break;
		}
		rb_yield(GVFSFILEINFO2RVAL(info));
	}

	if (result == GNOME_VFS_OK || result == GNOME_VFS_ERROR_EOF) {
		return Qnil;
	} else {
		r_result = GVFSRESULT2RVAL(result);
		return r_result;
	}
}

static VALUE
directory_read_next(self)
	VALUE self;
{
	GnomeVFSFileInfo *info;
	GnomeVFSResult result;
	VALUE r_result;

	info = gnome_vfs_file_info_new();
	result = gnome_vfs_directory_read_next(_SELF(self), info);
	if (result == GNOME_VFS_OK) {
		return GVFSFILEINFO2RVAL(info);
	} else if (result == GNOME_VFS_ERROR_EOF) {
		return Qnil;
	} else {
		gnome_vfs_file_info_unref(info);
		r_result = GVFSRESULT2RVAL(result);
		return r_result;
	}
}

static VALUE
directory_foreach(self, uri)
	VALUE self, uri;
{
	VALUE dir;

	dir = rb_funcall(g_gvfs_dir, rb_intern("open"), 1, uri);
	rb_ensure(directory_each, dir, directory_close, dir);
	return Qnil;
}

static gboolean
directory_visit_callback(rel_path, info, recursing_will_loop, data, recurse)
	const gchar *rel_path;
	GnomeVFSFileInfo *info;
	gboolean recursing_will_loop;
	gpointer data;
	gboolean *recurse;
{
	*recurse = RTEST(rb_funcall((VALUE)data,
				    g_id_call,
				    GVFSFILEINFO2RVAL(info),
				    recursing_will_loop));
	/* XXX: or what? */
	return TRUE;
}

static VALUE
directory_visit(argc, argv, self)
	int argc;
	VALUE *argv;
	VALUE self;
{
	VALUE uri, info_options, visit_options;
	VALUE func;
	GnomeVFSResult result;

	argc = rb_scan_args(argc, argv, "12&", &uri, &info_options,
			     &visit_options, &func);
	if (argc < 4) {
		visit_options = INT2FIX(GNOME_VFS_DIRECTORY_VISIT_DEFAULT);
	}
	if (argc < 3) {
		info_options = INT2FIX(GNOME_VFS_FILE_INFO_DEFAULT);
	}

	if (NIL_P(func)) {
		func = G_BLOCK_PROC();
	}
	G_RELATIVE(self, func);

	if (RTEST(rb_obj_is_kind_of(uri, g_gvfs_uri))) {
		result = gnome_vfs_directory_visit_uri(RVAL2GVFSURI(uri),
			FIX2INT(info_options),
			FIX2INT(visit_options),
			(GnomeVFSDirectoryVisitFunc)directory_visit_callback,
			(gpointer)func);
	} else {
		result = gnome_vfs_directory_visit(RVAL2CSTR(uri),
			FIX2INT(info_options),
			FIX2INT(visit_options),
			(GnomeVFSDirectoryVisitFunc)directory_visit_callback,
			(gpointer)func);
	}

	return GVFSRESULT2RVAL(result);
}

static VALUE
directory_visit_files(argc, argv, self)
	int argc;
	VALUE *argv;
	VALUE self;
{
	VALUE uri, r_list, info_options, visit_options;
	GList *list = NULL;
	int i, n;
	VALUE func;
	GnomeVFSResult result;

	argc = rb_scan_args(argc, argv, "22&", &uri, &r_list, &info_options,
			     &visit_options, &func);
	if (argc < 4) {
		visit_options = INT2FIX(GNOME_VFS_DIRECTORY_VISIT_DEFAULT);
	}
	if (argc < 3) {
		info_options = INT2FIX(GNOME_VFS_FILE_INFO_DEFAULT);
	}

	if (NIL_P(func)) {
		func = G_BLOCK_PROC();
	}
	G_RELATIVE(self, func);

	Check_Type(r_list, T_ARRAY);
	n = RARRAY(r_list)->len;
	for (i = 0; i < n; i++) {
		VALUE s;

		s = rb_ary_entry(r_list, i);
		Check_Type(s, T_STRING);
		list = g_list_append(list, RVAL2CSTR(s));
	}

	if (RTEST(rb_obj_is_kind_of(uri, g_gvfs_uri))) {
		result = gnome_vfs_directory_visit_files_at_uri(
			RVAL2GVFSURI(uri),
			list,
			FIX2INT(info_options),
			FIX2INT(visit_options),
			(GnomeVFSDirectoryVisitFunc)directory_visit_callback,
			(gpointer)func);
	} else {
		result = gnome_vfs_directory_visit_files(
			RVAL2CSTR(uri),
			list,
			FIX2INT(info_options),
			FIX2INT(visit_options),
			(GnomeVFSDirectoryVisitFunc)directory_visit_callback,
			(gpointer)func);
	}

	/* XXX: should we call g_free to unmalloc the strings from above? */
	g_list_foreach(list, (GFunc)g_free, NULL);
	g_list_free(list);

	return GVFSRESULT2RVAL(result);
}

static VALUE
directory_list_load(argc, argv, self)
	int argc;
	VALUE *argv;
	VALUE self;
{
	VALUE uri, r_options;
	GnomeVFSFileInfoOptions options;
	GList *list;
	GnomeVFSResult result;

	if (rb_scan_args(argc, argv, "11", &uri, &r_options) == 2) {
		options = FIX2INT(r_options);
	} else {
		/* XXX: or? */
		options = GNOME_VFS_FILE_INFO_DEFAULT;
	}

	result = gnome_vfs_directory_list_load(&list, RVAL2CSTR(uri), options);
	if (result == GNOME_VFS_OK) {
		return GLIST2ARY(list);
	} else {
		return GVFSRESULT2RVAL(result);
	}
}

void
Init_gnomevfs_directory(m_gvfs)
	VALUE m_gvfs;
{
	g_gvfs_dir = G_DEF_CLASS(GNOMEVFS_TYPE_DIRECTORY, "Directory", m_gvfs);

	rb_define_singleton_method(g_gvfs_dir, "make_directory",
				   make_directory, -1);
	rb_define_singleton_method(g_gvfs_dir, "mkdir", make_directory, -1);
	rb_define_singleton_method(g_gvfs_dir, "remove_directory",
				   remove_directory, 1);
	rb_define_singleton_method(g_gvfs_dir, "rmdir", remove_directory, 1);
	rb_define_singleton_method(g_gvfs_dir, "unlink", remove_directory, 1);
	rb_define_singleton_method(g_gvfs_dir, "delete", remove_directory, 1);
	rb_define_singleton_method(g_gvfs_dir, "open", directory_open, -1);
	rb_define_singleton_method(g_gvfs_dir, "foreach", directory_foreach, 1);
	rb_define_singleton_method(g_gvfs_dir, "visit", directory_visit, -1);
	rb_define_singleton_method(g_gvfs_dir, "visit_files",
				   directory_visit_files, -1);
	rb_define_singleton_method(g_gvfs_dir, "list_load",
				   directory_list_load, -1);
	/*
	rb_define_singleton_method(g_gvfs_dir, "entries", directory_list_load,
				   -1);
				   */

	rb_define_method(g_gvfs_dir, "initialize", directory_initialize, -1);
	rb_define_method(g_gvfs_dir, "close", directory_close, 0);
	rb_define_method(g_gvfs_dir, "each", directory_each, 0);
	rb_define_method(g_gvfs_dir, "read_next", directory_read_next, 0);
	rb_define_alias(g_gvfs_dir, "read", "read_next");

	rb_define_const(g_gvfs_dir,
			"VISIT_DEFAULT",
			INT2FIX(GNOME_VFS_DIRECTORY_VISIT_DEFAULT));
	rb_define_const(g_gvfs_dir,
			"VISIT_SAMEFS",
			INT2FIX(GNOME_VFS_DIRECTORY_VISIT_SAMEFS));
	rb_define_const(g_gvfs_dir,
			"VISIT_LOOPCHECK",
			INT2FIX(GNOME_VFS_DIRECTORY_VISIT_LOOPCHECK));
}

/* vim: set sts=0 sw=8 ts=8: *************************************************/
