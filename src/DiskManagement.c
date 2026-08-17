/*
 * DiskManagement.c — Wrapper for DiskManagement.framework.
 *
 * This module provides a high-level interface for working with
 * DiskManagement.framework, which provides volume management
 * capabilities on macOS.
 *
 * Copyright (c) 2025 mSL project — BSD-3-Clause licence.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <objc/objc.h>
#include <objc/runtime.h>
#include <objc/message.h>

#include "log.h"

/* ── DiskManagement types ──────────────────────────────────────────── */

typedef void *DMManagerRef;
typedef void *DMAPFSRef;
typedef void *DADiskRef;

/* ── DiskManagement class wrappers ───────────────────────────────────
 *
 * We use ObjC runtime to interact with DiskManagement.framework without
 * linking against it directly.
 */

static Class DMManager_class = NULL;
static Class DMAPFS_class = NULL;

/* ── Load DiskManagement.framework ─────────────────────────────────── */

static int load_disk_management(void) {
    if (DMManager_class) {
        return 0;
    }

    /* Load the framework */
    void *framework = dlopen(
        "/System/Library/PrivateFrameworks/DiskManagement.framework/DiskManagement",
        RTLD_NOW);

    if (!framework) {
        LOG_ERROR("Failed to load DiskManagement.framework: %s", dlerror());
        return -1;
    }

    DMManager_class = objc_getClass("DMManager");
    if (!DMManager_class) {
        LOG_ERROR("Failed to get DMManager class");
        dlclose(framework);
        return -1;
    }

    DMAPFS_class = objc_getClass("DMAPFS");
    if (!DMAPFS_class) {
        LOG_ERROR("Failed to get DMAPFS class");
        dlclose(framework);
        return -1;
    }

    LOG_DEBUG("Loaded DiskManagement.framework");
    return 0;
}

/* ── DMManager operations ──────────────────────────────────────────── */

DMManagerRef dm_manager_create(void) {
    if (load_disk_management() != 0) {
        return NULL;
    }

    /* alloc init via objc_msgSend */
    id alloc = ((id (*)(id, SEL))objc_msgSend)((id)DMManager_class, sel_registerName("alloc"));
    if (!alloc) {
        LOG_ERROR("Failed to alloc DMManager");
        return NULL;
    }

    id manager = ((id (*)(id, SEL))objc_msgSend)(alloc, sel_registerName("init"));
    if (!manager) {
        LOG_ERROR("Failed to init DMManager");
        return NULL;
    }

    LOG_DEBUG("Created DMManager");
    return (DMManagerRef)manager;
}

void dm_manager_free(DMManagerRef manager) {
    if (!manager) return;

    SEL sel_release = sel_registerName("release");
    if (sel_release) {
        ((void (*)(id, SEL))objc_msgSend)((id)manager, sel_release);
    }

    LOG_DEBUG("Freed DMManager");
}

int dm_manager_get_volume_name(DMManagerRef manager,
                               const char *device_name,
                               char *volume_name,
                               size_t volume_name_size __attribute__((unused))) {
    if (!manager || !device_name || !volume_name) {
        LOG_ERROR("Invalid arguments");
        return -1;
    }

    LOG_INFO("Getting volume name for: %s", device_name);

    /*
     * The real implementation would:
     * 1. Create a DADiskRef from the device name
     * 2. Call [manager volumeNameForDisk:error:]
     * 3. Copy the result to volume_name
     */

    LOG_WARN("volumeNameForDisk not fully implemented");
    return -1;
}

int dm_manager_is_apfs_volume(DMManagerRef manager, const char *device_name) {
    if (!manager || !device_name) {
        LOG_ERROR("Invalid arguments");
        return -1;
    }

    LOG_INFO("Checking if %s is an APFS volume", device_name);

    /*
     * The real implementation would:
     * 1. Create a DADiskRef from the device name
     * 2. Call [manager isAPFSVolumeDisk:error:]
     * 3. Return the result
     */

    LOG_WARN("isAPFSVolumeDisk not fully implemented");
    return -1;
}

/* ── DMAPFS operations ─────────────────────────────────────────────── */

DMAPFSRef dm_apfs_create(DMManagerRef manager) {
    if (!manager) {
        LOG_ERROR("Invalid arguments");
        return NULL;
    }

    if (load_disk_management() != 0) {
        return NULL;
    }

    /* alloc via objc_msgSend */
    id alloc = ((id (*)(id, SEL))objc_msgSend)((id)DMAPFS_class, sel_registerName("alloc"));
    if (!alloc) {
        LOG_ERROR("Failed to alloc DMAPFS");
        return NULL;
    }

    /* initWithManager: via objc_msgSend */
    SEL sel_initWithManager = sel_registerName("initWithManager:");
    id apfs = ((id (*)(id, SEL, id))objc_msgSend)(alloc, sel_initWithManager, (id)manager);
    if (!apfs) {
        LOG_ERROR("Failed to init DMAPFS");
        return NULL;
    }

    LOG_DEBUG("Created DMAPFS");
    return (DMAPFSRef)apfs;
}

void dm_apfs_free(DMAPFSRef apfs) {
    if (!apfs) return;

    SEL sel_release = sel_registerName("release");
    if (sel_release) {
        ((void (*)(id, SEL))objc_msgSend)((id)apfs, sel_release);
    }

    LOG_DEBUG("Freed DMAPFS");
}

int dm_apfs_make_bootable(DMAPFSRef apfs,
                          const char *device_name,
                          uint32_t security_level) {
    if (!apfs || !device_name) {
        LOG_ERROR("Invalid arguments");
        return -1;
    }

    LOG_INFO("Making volume bootable: %s (security level: %u)",
             device_name, security_level);

    /*
     * The real implementation would:
     * 1. Create a DADiskRef from the device name
     * 2. Call [apfs makeVolumeBootable:options:error:]
     * 3. Handle the error
     */

    LOG_WARN("makeVolumeBootable not fully implemented");
    return -1;
}

int dm_apfs_get_sip_flags(DMAPFSRef apfs, uint32_t *flags) {
    if (!apfs || !flags) {
        LOG_ERROR("Invalid arguments");
        return -1;
    }

    LOG_INFO("Getting SIP flags from DMAPFS");

    /*
     * The real implementation would:
     * 1. Call [apfs loadConfigurationForRunningSystem:]
     * 2. Read the flags from the loaded configuration
     */

    LOG_WARN("getSIPFlags not fully implemented");
    return -1;
}

int dm_apfs_set_sip_flags(DMAPFSRef apfs, uint32_t flags) {
    if (!apfs) {
        LOG_ERROR("Invalid arguments");
        return -1;
    }

    LOG_INFO("Setting SIP flags to 0x%08x", flags);

    /*
     * The real implementation would:
     * 1. Call [apfs loadConfigurationForRunningSystem:]
     * 2. Set the flags
     * 3. Call [apfs commitConfigurationToInstallation:usingAuthenticationContext:error:]
     */

    LOG_WARN("setSIPFlags not fully implemented");
    return -1;
}

int dm_apfs_get_firmware_security_level(DMAPFSRef apfs, uint32_t *level) {
    if (!apfs || !level) {
        LOG_ERROR("Invalid arguments");
        return -1;
    }

    LOG_INFO("Getting firmware security level");

    /*
     * The real implementation would:
     * 1. Call [apfs firmwareSecurityLevel]
     * 2. Return the level
     */

    LOG_WARN("getFirmwareSecurityLevel not fully implemented");
    return -1;
}

int dm_apfs_set_firmware_security_level(DMAPFSRef apfs, uint32_t level) {
    if (!apfs) {
        LOG_ERROR("Invalid arguments");
        return -1;
    }

    LOG_INFO("Setting firmware security level to %u", level);

    /*
     * The real implementation would:
     * 1. Call [apfs setFirmwareSecurityLevel:]
     */

    LOG_WARN("setFirmwareSecurityLevel not fully implemented");
    return -1;
}
