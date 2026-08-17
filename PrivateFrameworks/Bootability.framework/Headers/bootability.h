/*
 * bootability.h — Reverse-engineered Bootability.framework interface.
 *
 * Bootability.framework is a private Apple framework that manages
 * LocalPolicy signing via an XPC service (BootabilityService.xpc).
 * Apple's csrutil links it for the Recovery OS "commit" path.
 *
 * The framework lives in the dyld shared cache.  No public header exists.
 * Signatures below were recovered from the csrutil binary's ObjC metadata
 * and from the BootabilityService.xpc Mach-O.
 *
 * Copyright (c) 2025 mSL project — BSD-3-Clause licence.
 */

#ifndef CSRUTIL_BOOTABILITY_H
#define CSRUTIL_BOOTABILITY_H

#include <stdbool.h>
#include <stdint.h>

/* ── BYService XPC Interface ──────────────────────────────────────── */

/*
 * Bootability.framework exposes an XPC service via:
 *   com.apple.bootability.service
 *
 * The service class is BYService (NSXPCListener delegate).
 * The client class is BYServiceClient.
 *
 * Protocols:
 *   BYServiceProtocol       — methods the service exposes
 *   BYServiceClientProtocol — methods the client exposes (callbacks)
 *
 * Key XPC methods on BYServiceProtocol:
 *
 *   - (void)makeVolumeBootableWithGroupUUID:(NSUUID *)groupUUID
 *                                   options:(NSDictionary *)options
 *                                     reply:(void (^)(NSDictionary *, NSError *))reply;
 *
 *   - (void)prepareVolumeForMediumSecurityUpdateWithGroupUUID:(NSUUID *)groupUUID
 *                                                      options:(NSDictionary *)options
 *                                                        reply:(void (^)(NSDictionary *, NSError *))reply;
 *
 *   - (void)obtainAuthenticationContextWithReply:(void (^)(NSXPCListener *, NSError *))reply;
 *
 * BYService instance variables:
 *   _client               — BYServiceClient *
 *   _connection           — NSXPCConnection *
 *   _frameworkHandle      — void * (dlopen handle for the per-volume framework)
 *   _mountedPrebootVolume — NSString * (path to mounted Preboot mount point)
 *   _path                 — NSString *
 *   _pid                  — int
 *   _uid                  — uid_t
 *
 * BYServiceClient instance variables:
 *   _client               — id
 *   _connection           — NSXPCConnection *
 *   _frameworkHandle      — void *
 *   _mountedPrebootVolume — NSString *
 *   _path                 — NSString *
 *   _stagingDirectory     — NSString *
 *
 * Key private methods:
 *   - (BOOL)_boolForDeviceTreeKey:(NSString *)key path:(NSString *)path;
 *   - (id)_dataForDeviceTreeKey:(NSString *)key path:(NSString *)path;
 *   - (id)_stringForDeviceTreeKey:(NSString *)key path:(NSString *)path;
 *   - (id)_brainForVolume:(NSString *)volume options:(NSDictionary *)options error:(NSError **)error;
 *   - (id)_loadFrameworkForVolume:(NSString *)volume options:(NSDictionary *)options error:(NSError **)error;
 *   - (id)_systemVolumeWithGroupUUID:(NSUUID *)uuid mountedOnly:(BOOL)mountedOnly;
 *   - (BOOL)_shouldLoadFrameworkFromPrebootForVolume:(NSString *)volume
 *                                      frameworkPath:(NSString **)frameworkPath
 *                                      trustcachePath:(NSString **)trustcachePath
 *                                               error:(NSError **)error;
 *   - (NSString *)_restoreVersionFromPath:(NSString *)path;
 */

/* ── XPC Connection Helpers ───────────────────────────────────────── */

/* Service name for NSXPCConnection. */
#define BOOTABILITY_SERVICE_NAME "com.apple.bootability.service"

/* Protocol identifiers (from the ObjC metadata). */
#define BOOTABILITY_PROTOCOL_VERSION 1

/* ── Options dictionary keys ──────────────────────────────────────── */

/*
 * The options NSDictionary passed to makeVolumeBootableWithGroupUUID
 * and prepareVolumeForMediumSecurityUpdateWithGroupUUID contains:
 *
 *   @"groupUUID"         — NSUUID, the volume group UUID
 *   @"stagingDirectory"  — NSString, path to write the new policy
 *   @"force"             — NSNumber<BOOL>, force even if paired
 */

#endif /* CSRUTIL_BOOTABILITY_H */
