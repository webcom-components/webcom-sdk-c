/*
 * webcom-sdk-c
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

#ifndef TOOLS_WCSH_VT100_H_
#define TOOLS_WCSH_VT100_H_

enum {
	disabled = 0,
	/* 0 */ vt_reset,
	/* 1 */ vt_bright,
	/* 2 */ vt_dim,
	/* 4 */ vt_underscore,
	/* 5 */ vt_blink,
	/* 7 */ vt_reverse,
	/* 8 */ vt_hidden,
	/* foreground colours */
	/* 30 */ vt_fg_black ,
	/* 31 */ vt_fg_red ,
	/* 32 */ vt_fg_green ,
	/* 33 */ vt_fg_yellow ,
	/* 34 */ vt_fg_blue ,
	/* 35 */ vt_fg_magenta ,
	/* 36 */ vt_fg_cyan ,
	/* 37 */ vt_fg_white ,
	/*	background colours */
	/* 40 */ vt_bg_black ,
	/* 41 */ vt_bg_red ,
	/* 42 */ vt_bg_green ,
	/* 43 */ vt_bg_yellow ,
	/* 44 */ vt_bg_blue ,
	/* 45 */ vt_bg_magenta ,
	/* 46 */ vt_bg_cyan ,
	/* 47 */ vt_bg_white ,
};

extern int is_a_tty;
extern char *vt100codes[];

#define VT(_prop) vt100codes[is_a_tty ? vt_ ## _prop : 0]


#endif /* TOOLS_WCSH_VT100_H_ */
