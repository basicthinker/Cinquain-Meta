/*
 * Copyright (c) 2012 Jinglei Ren <jinglei.ren@stanzax.org>
 * All rights reserved.
 */

//
//  cinq_meta.c
//  cinquain-meta
//
//  Created by Jinglei Ren <jinglei.ren@gmail.com> on 3/18/12.
//

#include "cinq_meta.h"

const struct inode_operations cinq_dir_inode_operations = {
	.create		= cinq_create,
	.lookup		= cinq_lookup,
	.link     = cinq_link,
	.unlink		= cinq_unlink,
	.symlink	= cinq_symlink,
	.mkdir		= cinq_mkdir,
	.rmdir		= cinq_rmdir,
	.rename		= cinq_rename,
	.setattr	= cinq_setattr
};
