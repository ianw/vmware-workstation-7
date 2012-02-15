/*********************************************************
 * Copyright (C) 2002 VMware, Inc. All rights reserved.
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

#ifndef __COMPAT_NETDEVICE_H__
#   define __COMPAT_NETDEVICE_H__


#include <linux/skbuff.h>
#include <linux/rtnetlink.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>

/*
 * The enet_statistics structure moved from linux/if_ether.h to
 * linux/netdevice.h and is renamed net_device_stats in 2.1.25 --hpreg
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 1, 25)
#   include <linux/if_ether.h>

#   define net_device_stats enet_statistics
#endif


/* The netif_rx_ni() API appeared in 2.4.8 --hpreg */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 8)
#   define netif_rx_ni netif_rx
#endif


/* The device struct was renamed net_device in 2.3.14 --hpreg */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 3, 14)
#   define net_device device
#endif

/* it looks like these have been removed from the kernel 3.1
 * probably because the "transition" is considered complete.
 * so to keep this source compatible we just redefine them like they were
 * previously
 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 41, 0) && LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0)) || LINUX_VERSION_CODE >= KERNEL_VERSION(3, 1, 0)
#define HAVE_ALLOC_NETDEV		/* feature macro: alloc_xxxdev
					   functions are available. */
#define HAVE_FREE_NETDEV		/* free_netdev() */
#define HAVE_NETDEV_PRIV		/* netdev_priv() */
#define HAVE_NETIF_QUEUE
#define HAVE_NET_DEVICE_OPS
#endif

/*
 * SET_MODULE_OWNER appeared sometime during 2.3.x. It was setting
 * dev->owner = THIS_MODULE until 2.5.70, where netdevice refcounting
 * was completely changed.  SET_MODULE_OWNER was nop for whole
 * 2.6.x series, and finally disappeared in 2.6.24.
 *
 * MOD_xxx_USE_COUNT wrappers are here, as they must be mutually
 * exclusive with SET_MODULE_OWNER call.
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 0)
#   define COMPAT_SET_MODULE_OWNER(dev) do {} while (0)
#   define COMPAT_NETDEV_MOD_INC_USE_COUNT MOD_INC_USE_COUNT
#   define COMPAT_NETDEV_MOD_DEC_USE_COUNT MOD_DEC_USE_COUNT
#else
#   if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
#      define COMPAT_SET_MODULE_OWNER(dev) SET_MODULE_OWNER(dev)
#   else
#      define COMPAT_SET_MODULE_OWNER(dev) do {} while (0)
#   endif
#   define COMPAT_NETDEV_MOD_INC_USE_COUNT do {} while (0)
#   define COMPAT_NETDEV_MOD_DEC_USE_COUNT do {} while (0)
#endif

/*
 * SET_NETDEV_DEV appeared sometime during 2.5.x, and later was
 * crossported to various 2.4.x kernels (as dummy macro).
 */
#ifdef SET_NETDEV_DEV
#   define COMPAT_SET_NETDEV_DEV(dev, pdev) SET_NETDEV_DEV(dev, pdev)
#else
#   define COMPAT_SET_NETDEV_DEV(dev, pdev) do {} while (0)
#endif

/*
 * Build alloc_etherdev API on the top of init_etherdev.  For 2.0.x kernels
 * we must provide dummy init method, otherwise register_netdev does
 * nothing.
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 3)

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 2, 0)
int
vmware_dummy_init(struct net_device *dev)
{
   return 0;
}
#endif


static inline struct net_device*
compat_alloc_etherdev(int priv_size)
{
   struct net_device* dev;
   int size = sizeof *dev + priv_size;

   /*
    * The name is dynamically allocated before 2.4.0, but 
    * is an embedded array in later kernels.
    */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 0)
   size += sizeof("ethXXXXXXX");
#endif
   dev = kmalloc(size, GFP_KERNEL);
   if (dev) {
      memset(dev, 0, size);
      if (priv_size) {
         dev->priv = dev + 1;
      }
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 0)
      dev->name = (char *)(dev + 1) + priv_size;
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 2, 0)
      dev->init = vmware_dummy_init;
#endif
      if (init_etherdev(dev, 0) != dev) {
         kfree(dev);
         dev = NULL;
      }
   }
   return dev;
}
#else
#define compat_alloc_etherdev(sz)   alloc_etherdev(sz)
#endif


/*
 * alloc_netdev and free_netdev are there since 2.4.23.  Their use is mandatory
 * since 2.6.24.
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 23)
static inline struct net_device *
compat_alloc_netdev(int priv_size,
                    const char *mask,
                    void (*setup)(struct net_device *))
{
   struct net_device *dev;
   int netdev_size = sizeof *dev;
   int alloc_size;

#   if LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 0)
   netdev_size += IFNAMSIZ;
#   endif

   alloc_size = netdev_size + priv_size;
   dev = kmalloc(alloc_size, GFP_KERNEL);
   if (dev) {
      memset(dev, 0, alloc_size);
      dev->priv = (char*)dev + netdev_size;
      setup(dev);
#   if LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 0)
      dev->name = (char*)(dev + 1);
#   endif
      strcpy(dev->name, mask);   
   }
   return dev;
}
#   define compat_free_netdev(dev)     kfree(dev)
#else
#   define compat_alloc_netdev(size, mask, setup) alloc_netdev(size, mask, setup)
#   define compat_free_netdev(dev)                free_netdev(dev)
#endif

/* netdev_priv() appeared in 2.6.3 */
#if  LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 3)
#   define compat_netdev_priv(netdev)   (netdev)->priv
#else
#   define compat_netdev_priv(netdev)   netdev_priv(netdev)
#endif

/*
 * All compat_* business is good but when we can we should just provide
 * missing implementation to ease upstreaming task.
 */
#ifndef HAVE_ALLOC_NETDEV
#define alloc_netdev(sz, name, setup)  compat_alloc_netdev(sz, name, setup)
#define alloc_etherdev(sz)             compat_alloc_etherdev(sz)
#endif

#ifndef HAVE_FREE_NETDEV
#define free_netdev(dev)               kfree(dev)
#endif

#ifndef HAVE_NETDEV_PRIV
#define netdev_priv(dev)               ((dev)->priv)
#endif

#if defined(NETDEV_TX_OK)
#   define COMPAT_NETDEV_TX_OK    NETDEV_TX_OK
#   define COMPAT_NETDEV_TX_BUSY  NETDEV_TX_BUSY
#else
#   define COMPAT_NETDEV_TX_OK    0
#   define COMPAT_NETDEV_TX_BUSY  1
#endif

#ifndef HAVE_NETIF_QUEUE
static inline void
netif_start_queue(struct device *dev)
{
   clear_bit(0, &dev->tbusy);
}

static inline void
netif_stop_queue(struct device *dev)
{
   set_bit(0, &dev->tbusy);
}

static inline int
netif_queue_stopped(struct device *dev)
{
   return test_bit(0, &dev->tbusy);
}

static inline void
netif_wake_queue(struct device *dev)
{
   clear_bit(0, &dev->tbusy);
   mark_bh(NET_BH);
}

static inline int
netif_running(struct device *dev)
{
   return dev->start == 0;
}

static inline int
netif_carrier_ok(struct device *dev)
{
   return 1;
}

static inline void
netif_carrier_on(struct device *dev)
{
}

static inline void
netif_carrier_off(struct device *dev)
{
}
#endif

/* Keep compat_* defines for now */
#define compat_netif_start_queue(dev)   netif_start_queue(dev)
#define compat_netif_stop_queue(dev)    netif_stop_queue(dev)
#define compat_netif_queue_stopped(dev) netif_queue_stopped(dev)
#define compat_netif_wake_queue(dev)    netif_wake_queue(dev)
#define compat_netif_running(dev)       netif_running(dev)
#define compat_netif_carrier_ok(dev)    netif_carrier_ok(dev)
#define compat_netif_carrier_on(dev)    netif_carrier_on(dev)
#define compat_netif_carrier_off(dev)   netif_carrier_off(dev)

/* unregister_netdevice_notifier was not safe prior to 2.6.17 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 17) && \
    !defined(ATOMIC_NOTIFIER_INIT)
/* pre 2.6.17 and not patched */
static inline int compat_unregister_netdevice_notifier(struct notifier_block *nb) {
   int err;

   rtnl_lock();
   err = unregister_netdevice_notifier(nb);
   rtnl_unlock();
   return err;
}
#else
/* post 2.6.17 or patched */
#define compat_unregister_netdevice_notifier(_nb) \
        unregister_netdevice_notifier(_nb);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 24)

#   define compat_netif_napi_add(dev, napi, poll, quota) \
      netif_napi_add(dev, napi, poll, quota)

#   if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 30) || \
       defined VMW_NETIF_SINGLE_NAPI_PARM
#      define compat_napi_complete(dev, napi) napi_complete(napi)
#      define compat_napi_schedule(dev, napi) napi_schedule(napi)
#   else
#      define compat_napi_complete(dev, napi) netif_rx_complete(dev, napi)
#      define compat_napi_schedule(dev, napi) netif_rx_schedule(dev, napi)
#   endif

#   define compat_napi_enable(dev, napi)  napi_enable(napi)
#   define compat_napi_disable(dev, napi) napi_disable(napi)

#else

#   define compat_netif_napi_add(dev, napi, pollcb, quota) \
   do {                        \
      (dev)->poll = (pollcb);    \
      (dev)->weight = (quota);\
   } while (0)
#   define compat_napi_schedule(dev, napi) netif_rx_schedule(dev)
#   define compat_napi_enable(dev, napi)   netif_poll_enable(dev)
#   define compat_napi_disable(dev, napi)  netif_poll_disable(dev)

/* RedHat ported GRO to 2.6.18 bringing new napi_struct with it */
#   if defined NETIF_F_GRO
#      define compat_napi_complete(dev, napi) napi_complete(napi)
#   else
#      define compat_napi_complete(dev, napi) netif_rx_complete(dev)
       struct napi_struct {
          int dummy;
       };
#   endif

#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)
#   define COMPAT_VLAN_GROUP_ARRAY_LEN VLAN_N_VID
#else
#   define COMPAT_VLAN_GROUP_ARRAY_LEN VLAN_GROUP_ARRAY_LEN
#endif

#endif /* __COMPAT_NETDEVICE_H__ */
