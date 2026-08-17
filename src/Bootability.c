/*
 * Bootability.c — Wrapper for Bootability.framework.
 *
 * This module provides a high-level interface for working with
 * Bootability.framework, which manages volume bootability on
 * Apple Silicon.
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

/* ── Bootability types ─────────────────────────────────────────────── */

typedef void *BYManagerRef;
typedef void *DADiskRef;

/* ── Bootability class wrapper ───────────────────────────────────────
 *
 * We use ObjC runtime to interact with Bootability.framework without
 * linking against it directly.
 */

static Class BYManager_class = NULL;

/* ── Load Bootability.framework ────────────────────────────────────── */

static int load_bootability(void) {
    if (BYManager_class) {
        return 0;
    }

    /* Load the framework */
    void *framework = dlopen(
        "/System/Library/PrivateFrameworks/Bootability.framework/Bootability",
        RTLD_NOW);

    if (!framework) {
        LOG_ERROR("Failed to load Bootability.framework: %s", dlerror());
        return -1;
    }

    BYManager_class = objc_getClass("BYManager");
    if (!BYManager_class) {
        LOG_ERROR("Failed to get BYManager class");
        dlclose(framework);
        return -1;
    }

    LOG_DEBUG("Loaded Bootability.framework");
    return 0;
}

/* ── BYManager operations ──────────────────────────────────────────── */

BYManagerRef by_manager_create(void) {
    if (load_bootability() != 0) {
        return NULL;
    }

    /* alloc init via objc_msgSend */
    id alloc = ((id (*)(id, SEL))objc_msgSend)((id)BYManager_class, sel_registerName("alloc"));
    if (!alloc) {
        LOG_ERROR("Failed to alloc BYManager");
        return NULL;
    }

    id manager = ((id (*)(id, SEL))objc_msgSend)(alloc, sel_registerName("init"));
    if (!manager) {
        LOG_ERROR("Failed to init BYManager");
        return NULL;
    }

    LOG_DEBUG("Created BYManager");
    return (BYManagerRef)manager;
}

void by_manager_free(BYManagerRef manager) {
    if (!manager) return;

    SEL sel_release = sel_registerName("release");
    if (sel_release) {
        ((void (*)(id, SEL))objc_msgSend)((id)manager, sel_release);
    }

    LOG_DEBUG("Freed BYManager");
}

int by_manager_make_volume_bootable(BYManagerRef manager,
                                    const char *device_name,
                                    const char *mount_point __attribute__((unused))) {
    if (!manager || !device_name) {
        LOG_ERROR("Invalid arguments");
        return -1;
    }

    LOG_INFO("Making volume bootable: %s", device_name);

    /*
     * The real implementation would:
     * 1. Create a DADiskRef from the device name
     * 2. Call [manager makeVolumeBootable:options:error:]
     * 3. Handle the error
     *
     * For now, we just log the operation.
     */

    LOG_WARN("makeVolumeBootable not fully implemented");
    return -1;
}

int by_manager_make_volume_bootable_with_uuid(BYManagerRef manager,
                                              const char *group_uuid) {
    if (!manager || !group_uuid) {
        LOG_ERROR("Invalid arguments");
        return -1;
    }

    LOG_INFO("Making volume bootable with UUID: %s", group_uuid);

    /*
     * The real implementation would:
     * 1. Create an NSUUID from the string
     * 2. Call [manager makeVolumeBootableWithGroupUUID:options:error:]
     * 3. Handle the error
     */

    LOG_WARN("makeVolumeBootableWithGroupUUID not fully implemented");
    return -1;
}

int by_manager_get_authentication_context(BYManagerRef manager,
                                          void **context) {
    if (!manager || !context) {
        LOG_ERROR("Invalid arguments");
        return -1;
    }

    LOG_INFO("Getting authentication context");

    /*
     * The real implementation would:
     * 1. Call [manager obtainAuthenticationContextWithReply:]
     * 2. Return the XPCListener
     */

    LOG_WARN("obtainAuthenticationContext not fully implemented");
    return -1;
}

int by_manager_verify_manifest(BYManagerRef manager,
                                const char *manifest_path,
                                bool personalized) {
    if (!manager || !manifest_path) {
        LOG_ERROR("Invalid arguments");
        return -1;
    }

    LOG_INFO("Verifying manifest: %s (personalized: %s)",
             manifest_path, personalized ? "yes" : "no");

    /*
     * The real implementation would:
     * 1. Call [manager verifyManifest:personalized:restoreBundle:error:]
     * 2. Return the result
     */

    LOG_WARN("verifyManifest not fully implemented");
    return -1;
}
