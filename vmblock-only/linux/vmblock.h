/*********************************************************
 * Copyright (C) 2006-2008 VMware, Inc. All rights reserved.
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

/*
 * vmblock.h --
 *
 *   User-level interface to the vmblock device.
 *
 *   VMBLOCK_DEVICE should be opened with VMBLOCK_DEVICE_MODE mode. Then
 *   VMBLOCK_CONTROL should be called to perform blocking operations.
 *   The links which can be blocked are in the directory VMBLOCK_MOUNT_POINT.
 *
 *   VMBLOCK_CONTROL takes the file descriptor of the VMBLOCK_DEVICE, an
 *   operation, and the path of the target of the file being operated on (if
 *   applicable).
 *
 *   The operation should be one of:
 *   VMBLOCK_ADD_FILEBLOCK
 *   VMBLOCK_DEL_FILEBLOCK
 *   VMBLOCK_LIST_FILEBLOCKS
 *
 *   path should be something in /tmp/VMwareDnD/ rather than in
 *   VMBLOCK_MOUNT_POINT.
 *
 *   VMBLOCK_CONTROL returns 0 on success or returns -1 and sets errno on
 *   failure.
 */

#ifndef _VMBLOCK_H_
#define _VMBLOCK_H_

#if defined(sun) || defined(__FreeBSD__)
# include <sys/ioccom.h>
#endif

#if defined(__FreeBSD__)
# include <sys/param.h>
#endif


/*
 * FUSE definitions. They are supposed to be used by userspace code and
 * therefore not guarded by ARCH defines since FUSE can potentially
 * be used on different operating systems.
 */

#define VMBLOCK_FUSE_ADD_FILEBLOCK     'a'
#define VMBLOCK_FUSE_DEL_FILEBLOCK     'd'
#ifdef VMX86_DEVEL
# define VMBLOCK_FUSE_LIST_FILEBLOCKS  'l'
#endif /* VMX86_DEVEL */

/*
 * If you try 'read'-ing from file descriptor vmblock-fuse is supposed
 * to respond with the following. It is used to check whether we deal
 * with FUSE or in-kernel implementation.
 */
#define VMBLOCK_FUSE_READ_RESPONSE     "I am VMBLOCK-FUSE"

#define VMBLOCK_FUSE_FS_NAME           "fuse.vmware-vmblock"
#define VMBLOCK_FUSE_MOUNT_POINT       "/tmp/vmblock-fuse"
#define VMBLOCK_FUSE_CTRL_MNTPNT       "blockdir"
#define VMBLOCK_FUSE_FS_ROOT           VMBLOCK_FUSE_MOUNT_POINT "/" VMBLOCK_FUSE_CTRL_MNTPNT
#define VMBLOCK_FUSE_DEVICE_NAME       "dev"
#define VMBLOCK_FUSE_DEVICE            VMBLOCK_FUSE_MOUNT_POINT "/" VMBLOCK_FUSE_DEVICE_NAME
#define VMBLOCK_FUSE_DEVICE_MODE       O_RDWR

/* Commands for the control half of vmblock driver */
#if defined(vmblock_fuse)
/* These definitions are for vmblock-fuse module itself */
# include <unistd.h>
# include <limits.h>
# include <string.h>
# include <errno.h>
# include "vm_basic_types.h"
# define VMBLOCK_ADD_FILEBLOCK         VMBLOCK_FUSE_ADD_FILEBLOCK
# define VMBLOCK_DEL_FILEBLOCK         VMBLOCK_FUSE_DEL_FILEBLOCK
# ifdef VMX86_DEVEL
#  define VMBLOCK_LIST_FILEBLOCKS      VMBLOCK_FUSE_LIST_FILEBLOCKS
# endif /* VMX86_DEVEL */
/*
 * Some of the following names don't actually make much sense on their own.
 * They're used for consistency with the other ports. See the file header for
 * explanations of what they're used for.
 */
# define VMBLOCK_FS_NAME               VMBLOCK_FUSE_FS_NAME
# define VMBLOCK_DEVICE_NAME           VMBLOCK_FUSE_DEVICE_NAME
# define VMBLOCK_CONTROL_MOUNTPOINT    VMBLOCK_FUSE_CTRL_MNTPNT
# define VMBLOCK_DEVICE                VMBLOCK_FUSE_DEVICE
# define VMBLOCK_DEVICE_MODE           VMBLOCK_FUSE_DEVICE_MODE
# define VMBLOCK_MOUNT_POINT           VMBLOCK_FUSE_MOUNT_POINT

#elif defined(linux)
# define VMBLOCK_ADD_FILEBLOCK         98
# define VMBLOCK_DEL_FILEBLOCK         99
# ifdef VMX86_DEVEL
#  define VMBLOCK_LIST_FILEBLOCKS      100
# endif
# define VMBLOCK_FS_NAME               "vmblock"
# define VMBLOCK_CONTROL_DIRNAME       VMBLOCK_FS_NAME
# define VMBLOCK_CONTROL_DEVNAME       "dev"
# define VMBLOCK_CONTROL_MOUNTPOINT    "mountPoint"
# define VMBLOCK_CONTROL_PROC_DIRNAME  "fs/" VMBLOCK_CONTROL_DIRNAME

# define VMBLOCK_MOUNT_POINT            "/proc/" VMBLOCK_CONTROL_PROC_DIRNAME   \
                                       "/" VMBLOCK_CONTROL_MOUNTPOINT
# define VMBLOCK_FS_ROOT                VMBLOCK_MOUNT_POINT
# define VMBLOCK_DEVICE                 "/proc/" VMBLOCK_CONTROL_PROC_DIRNAME   \
                                       "/" VMBLOCK_CONTROL_DEVNAME
# define VMBLOCK_DEVICE_MODE            O_WRONLY

#elif defined(sun) || defined(__FreeBSD__)
# define VMBLOCK_FS_NAME                "vmblock"
# define VMBLOCK_MOUNT_POINT            "/var/run/" VMBLOCK_FS_NAME
# define VMBLOCK_FS_ROOT                VMBLOCK_MOUNT_POINT
# define VMBLOCK_DEVICE                 VMBLOCK_MOUNT_POINT
# define VMBLOCK_DEVICE_MODE            O_RDONLY
# if defined(sun)                       /* if (sun) { */
   /*
    * Construct ioctl(2) commands for blocks.  _IO() is a helper macro to
    * construct unique command values more easily.  I chose 'v' because I
    * didn't see it being used elsewhere, and the command numbers begin at one.
    */
#  define VMBLOCK_ADD_FILEBLOCK          _IO('v', 1)
#  define VMBLOCK_DEL_FILEBLOCK          _IO('v', 2)
#  ifdef VMX86_DEVEL
#   define VMBLOCK_LIST_FILEBLOCKS       _IO('v', 3)
#  endif

# elif defined(__FreeBSD__)              /* } else if (FreeBSD) { */
   /*
    * Similar to Solaris, construct ioctl(2) commands for block operations.
    * Since the FreeBSD implementation does not change the user's passed-in
    * data (pathname), we use the _IOW macro to define commands which write
    * to the kernel.  (As opposed to _IOR or _IOWR.)  Groups 'v' and 'V'
    * are taken by terminal drivers, so I opted for group 'Z'.
    */
#  define VMBLOCK_ADD_FILEBLOCK          _IOW('Z', 1, char[MAXPATHLEN] )
#  define VMBLOCK_DEL_FILEBLOCK          _IOW('Z', 2, char[MAXPATHLEN] )
#  ifdef VMX86_DEVEL
#   define VMBLOCK_LIST_FILEBLOCKS       _IO('Z', 3)
#   define VMBLOCK_PURGE_FILEBLOCKS      _IO('Z', 4)
#  endif

# endif                                 /* } */
#else
# error "Unknown platform for vmblock."
#endif

#endif /* _VMBLOCK_H_ */
