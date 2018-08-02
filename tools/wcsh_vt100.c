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

#include "wcsh_vt100.h"

char *vt100codes[] = {
		[disabled] = "",
		[vt_reset]       = "\033[0m" ,
		[vt_bright]      = "\033[1m" ,
		[vt_dim]         = "\033[2m" ,
		[vt_underscore]  = "\033[4m" ,
		[vt_blink]       = "\033[5m" ,
		[vt_reverse]     = "\033[7m" ,
		[vt_hidden]      = "\033[8m" ,
		[vt_fg_black]    = "\033[30m",
		[vt_fg_red]      = "\033[31m",
		[vt_fg_green]    = "\033[32m",
		[vt_fg_yellow]   = "\033[33m",
		[vt_fg_blue]     = "\033[34m",
		[vt_fg_magenta]  = "\033[35m",
		[vt_fg_cyan]     = "\033[36m",
		[vt_fg_white]    = "\033[37m",
		[vt_bg_black]    = "\033[40m",
		[vt_bg_red]      = "\033[41m",
		[vt_bg_green]    = "\033[42m",
		[vt_bg_yellow]   = "\033[43m",
		[vt_bg_blue]     = "\033[44m",
		[vt_bg_magenta]  = "\033[45m",
		[vt_bg_cyan]     = "\033[46m",
		[vt_bg_white]    = "\033[47m",
};
