/*
 * BYManager.h — Reverse-engineered BYManager class from Bootability.framework.
 *
 * BYManager is the primary client-side class for managing volume
 * bootability on Apple Silicon.  It wraps the Bootability XPC service
 * and provides high-level methods for:
 *   - Making volumes bootable (signing LocalPolicy)
 *   - Preparing volumes for medium-security software updates
 *   - Obtaining authentication contexts for policy operations
 *   - Verifying manifests and firmware security levels
 *
 * Reverse-engineered from the live ObjC runtime via
 * class_copyMethodList / class_copyIvarList on macOS 26.
 * The framework lives in the dyld shared cache at:
 *   /System/Library/PrivateFrameworks/Bootability.framework
 *
 * Copyright (c) 2025 mSL project — BSD-3-Clause licence.
 */

#ifndef BY_MANAGER_H
#define BY_MANAGER_H

#ifdef __OBJC__

#import <Foundation/Foundation.h>
#import <DiskArbitration/DiskArbitration.h>

/* ── Forward declarations ─────────────────────────────────────────── */

@class BYAuthenticationContext;

/* ── Security mode enum ─────────────────────────────────────────────
 *
 * Extracted from the _OBJC_ENUM_VALUES in the csrutil binary.
 */
typedef NS_ENUM(uint32_t, BYBootOptionSecurityMode) {
    BYBootOptionSecurityModeFull       = 0,
    BYBootOptionSecurityModeReduced   = 1,
    BYBootOptionSecurityModePermissive = 2,
};

/* ── BYManager ──────────────────────────────────────────────────────
 *
 * Superclass: NSObject
 * Instance size: 40 bytes
 *
 * ivars:
 *   _username                (NSString *)   offset=8
 *   _password                (NSString *)   offset=16
 *   _localAuthenticationContext (NSData *)  offset=24
 *   _authenticationContext   (BYAuthenticationContext *)  offset=32
 */
@interface BYManager : NSObject

/* ── Properties ───────────────────────────────────────────────────── */

@property (nonatomic, copy, nullable)   NSString *username;
@property (nonatomic, copy, nullable)   NSString *password;
@property (nonatomic, strong, nullable) NSData *localAuthenticationContext;
@property (nonatomic, strong, nullable) BYAuthenticationContext *authenticationContext;

@property (nonatomic, readonly, nullable) NSString *firstSystemVolumeMountPoint;

/* ── Instance methods ─────────────────────────────────────────────── */

/*
 * -[BYManager makeVolumeBootable:options:error:]
 *   Makes the given volume bootable by signing a new LocalPolicy.
 *
 *   volume  — DADisk* for the volume to make bootable.
 *   options — NSDictionary of options (groupUUID, stagingDirectory, etc.).
 *   error   — on failure, receives NSError*.
 *   Returns BOOL (YES on success).
 */
- (BOOL)makeVolumeBootable:(DADiskRef _Nonnull)volume
                   options:(NSDictionary * _Nullable)options
                      error:(NSError * _Nullable __autoreleasing * _Nullable)error;

/*
 * -[BYManager makeVolumeBootableWithGroupUUID:options:error:]
 *   Same as above, but identifies the volume group by UUID.
 */
- (BOOL)makeVolumeBootableWithGroupUUID:(NSUUID * _Nonnull)groupUUID
                                options:(NSDictionary * _Nullable)options
                                   error:(NSError * _Nullable __autoreleasing * _Nullable)error;

/*
 * -[BYManager prepareVolumeForMediumSecuritySoftwareUpdate:options:error:]
 *   Prepares a volume for a medium-security software update.
 *   Returns BOOL.
 */
- (BOOL)prepareVolumeForMediumSecuritySoftwareUpdate:(DADiskRef _Nonnull)volume
                                              options:(NSDictionary * _Nullable)options
                                                 error:(NSError * _Nullable __autoreleasing * _Nullable)error;

/*
 * -[BYManager prepareVolumeForMediumSecurityUpdateWithGroupUUID:options:error:]
 *   Same as above, identified by group UUID.
 */
- (BOOL)prepareVolumeForMediumSecurityUpdateWithGroupUUID:(NSUUID * _Nonnull)groupUUID
                                                  options:(NSDictionary * _Nullable)options
                                                     error:(NSError * _Nullable __autoreleasing * _Nullable)error;

/*
 * -[BYManager obtainAuthenticationContextWithReply:]
 *   Obtains an XPC listener for the authentication context.
 *   The reply block receives the NSXPCListener and an optional error.
 */
- (void)obtainAuthenticationContextWithReply:(void (^ _Nonnull)(NSXPCListener * _Nullable,
                                                                NSError * _Nullable))reply;

/*
 * -[BYManager verifyManifest:personalized:restoreBundle:error:]
 *   Verifies a manifest for a personalized restore bundle.
 *   Returns BOOL.
 */
- (BOOL)verifyManifest:(NSString * _Nonnull)manifest
            personalized:(BOOL)personalized
            restoreBundle:(NSString * _Nullable)restoreBundle
                   error:(NSError * _Nullable __autoreleasing * _Nullable)error;

/* ── Private methods (from ObjC metadata) ─────────────────────────── */

- (NSString * _Nullable)localAuthenticationContext;
- (void)setLocalAuthenticationContext:(NSString * _Nullable)context;

- (BYAuthenticationContext * _Nullable)authenticationContext;
- (void)setAuthenticationContext:(BYAuthenticationContext * _Nullable)ctx;

- (NSXPCConnection * _Nullable)_serviceConnectionWithError:(NSError * _Nullable __autoreleasing * _Nullable)error;

- (NSData * _Nullable)_authenticationContextWithVolume:(NSString * _Nonnull)volume
                                               options:(NSDictionary * _Nullable)options;

@end

/* ── Key constants ──────────────────────────────────────────────────
 *
 * Options dictionary keys (from the csrutil disassembly):
 *   @"groupUUID"         — NSUUID, the volume group UUID
 *   @"stagingDirectory"  — NSString, path to write the new policy
 *   @"force"             — NSNumber<BOOL>, force even if paired
 */

#else
#error "BYManager.h requires Objective-C (--objc or .m files)"
#endif /* __OBJC__ */

#endif /* BY_MANAGER_H */
