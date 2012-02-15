/*********************************************************
 * Copyright (C) 1998-2003 VMware, Inc. All rights reserved.
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
 *
 * vcpuid.h --
 *
 *    Monitor's VCPU ID.
 */

#ifndef _VCPUID_H_
#define _VCPUID_H_

#define INCLUDE_ALLOW_USERLEVEL
#define INCLUDE_ALLOW_VMMEXT
#define INCLUDE_ALLOW_MODULE
#define INCLUDE_ALLOW_VMMON
#define INCLUDE_ALLOW_VMNIXMOD
#define INCLUDE_ALLOW_VMKERNEL
#define INCLUDE_ALLOW_VMK_MODULE
#define INCLUDE_ALLOW_DISTRIBUTE
#define INCLUDE_ALLOW_VMCORE
#include "includeCheck.h"


typedef uint32 Vcpuid;			// VCPU number

#define VCPUID_INVALID	(~0U)

#define BOOT_VCPU_ID		0
#define IS_BOOT_VCPUID(vcpuid)  ((vcpuid) == BOOT_VCPU_ID)
#define IS_BOOT_VCPU()		IS_BOOT_VCPUID(CurVcpuid())

#define MAX_VCPUS	8


#endif // ifndef _VCPUID_H_
