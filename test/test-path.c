/*
 * ht
 *
 * Copyright 2018 Orange
 * <camille.oudot@orange.com>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include "stfu.h"
#include "../lib/datasync/path.h"

int main(void) {
	wc_ds_path_t *path1, *path2, *path3, *path4, *path5, *path6, *path7, *path8;

	path1 = wc_datasync_path_new("/////ababa/foo/////");
	path2 = wc_datasync_path_new("");
	path3 = wc_datasync_path_new("/////");
	path4 = wc_datasync_path_new("/////ababa/foo/qsdqsd/retert");
	path5 = wc_datasync_path_new("ababa/foo/azerty/oooo/");
	path6 = wc_datasync_path_new("/a/z/e/r/t/y/u/i/o/p/q/s/d/f/g/h/j/k/l/m/w/x/c/v/b/n/0/1/2/3/4/5/BOOM/7/8/9/");
	path7 = wc_datasync_path_new("/ababa/fop");
	path8 = wc_datasync_path_new("/ababa/fop/fup");


	STFU_TRUE("'/////ababa/foo/////' has 2 parts",
			wc_datasync_path_get_part_count(path1) == 2);

	STFU_TRUE("'' has 0 parts",
			wc_datasync_path_get_part_count(path2) == 0);

	STFU_TRUE("'/////' has 0 parts",
			wc_datasync_path_get_part_count(path3) == 0);

	STFU_TRUE("'/////ababa/foo/qsdqsd/retert' has 4 parts",
			wc_datasync_path_get_part_count(path4) == 4);

	STFU_TRUE("'ababa/foo/azerty/oooo/' has 4 parts",
			wc_datasync_path_get_part_count(path5) == 4);

	STFU_TRUE("'/a/z/e/r/t/y/u/i/o/p/q/s/d/f/g/h/j/k/l/m/w/x/c/v/b/n/0/1/2/3/4/5/BOOM/7/8/9/' has 32 parts (max depth)",
			wc_datasync_path_get_part_count(path6) == 32);



	STFU_STR_EQ("'ababa/foo/azerty/oooo/' first part is 'ababa'",
			wc_datasync_path_get_part(path5, 0), "ababa");

	STFU_STR_EQ("'ababa/foo/azerty/oooo/' second part is 'foo'",
			wc_datasync_path_get_part(path5, 1), "foo");

	STFU_STR_EQ("'ababa/foo/azerty/oooo/' third part is 'azerty'",
			wc_datasync_path_get_part(path5, 2), "azerty");

	STFU_STR_EQ("'ababa/foo/azerty/oooo/' fourth part is 'oooo'",
			wc_datasync_path_get_part(path5, 3), "oooo");

	STFU_TRUE("Empty paths are equal",
			wc_datasync_path_cmp(path2, path3) == 0);

	STFU_TRUE("/ababa/foo/qsdqsd/retert > /ababa/foo/azerty/oooo",
			wc_datasync_path_cmp(path4, path5) > 0);

	STFU_TRUE("/ababa/foo/azerty/oooo < /ababa/foo/qsdqsd/retert",
			wc_datasync_path_cmp(path5, path4) < 0);

	STFU_TRUE("/ababa/fop > /ababa/foo",
			wc_datasync_path_cmp(path7, path1) > 0);

	STFU_TRUE("/ababa/foo < /ababa/fop",
			wc_datasync_path_cmp(path1, path7) < 0);

	STFU_TRUE("/ababa/fop > /ababa/fop/fup",
				wc_datasync_path_cmp(path7, path8) > 0);

		STFU_TRUE("/ababa/fop/fup < /ababa/fop",
			wc_datasync_path_cmp(path8, path7) < 0);

	wc_datasync_path_destroy(path1);
	wc_datasync_path_destroy(path2);
	wc_datasync_path_destroy(path3);
	wc_datasync_path_destroy(path4);
	wc_datasync_path_destroy(path5);
	wc_datasync_path_destroy(path6);
	wc_datasync_path_destroy(path7);
	wc_datasync_path_destroy(path8);

	STFU_SUMMARY();

	return STFU_NUMBER_FAILED;
}
