/*********************************************************
 * Copyright (C) 2006 VMware, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation version 2 and no later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 *********************************************************/

#ifndef __COMPAT_NAMEI_H__
#   define __COMPAT_NAMEI_H__

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 18)
#include <linux/namei.h>
#endif

/*
 * In 2.6.25-rc2, dentry and mount objects were removed from the nameidata
 * struct. They were both replaced with a struct path.
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 25)
#define compat_vmw_nd_to_dentry(nd) (nd).path.dentry
#else
#define compat_vmw_nd_to_dentry(nd) (nd).dentry
#endif

/* In 2.6.25-rc2, path_release(&nd) was replaced with path_put(&nd.path). */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 25)
#define compat_path_release(nd) path_put(&(nd)->path)
#else
#define compat_path_release(nd) path_release(nd)
#endif

/* path_lookup was exported in 2.4.25 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 25)
#define compat_path_lookup(path, flags, nd)     path_lookup(path, flags, nd)
#else
#define compat_path_lookup(path, flags, nd)     \
         ({                                     \
            int ret = 0;                        \
            if (path_init(path, flags, nd)) {   \
               ret = path_walk(path, nd);       \
            }                                   \
            ret;                                \
         })
#endif

#endif /* __COMPAT_NAMEI_H__ */
