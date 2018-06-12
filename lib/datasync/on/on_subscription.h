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

#ifndef LIB_DATASYNC_ON_ON_SUBSCRIPTION_H_
#define LIB_DATASYNC_ON_ON_SUBSCRIPTION_H_

#include "webcom-c/webcom.h"


#include "../path.h"
#include "../cache/treenode.h"
#include "../../collection/avl.h"


struct on_cb_list {
	on_callback_f cb;
	struct on_cb_list *next;
};

struct on_sub {
	wc_context_t *ctx;
	struct on_cb_list *cb_list[ON_EVENT_TYPE_COUNT];
	avl_t *children_hashes;
	treenode_hash_t hash;
	struct wc_ds_path path;
};

#define ON_SUB_STRUCT_MAX_SIZE (sizeof(struct on_sub) + PATH_STRUCT_MAX_FLEXIBLE_SIZE)


#endif /* LIB_DATASYNC_ON_ON_SUBSCRIPTION_H_ */
