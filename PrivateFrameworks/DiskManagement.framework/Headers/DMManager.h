/*
 * DMManager.h — Reverse-engineered DMManager class from DiskManagement.framework.
 *
 * DMManager is the primary class for interacting with disk volumes
 * on macOS.  It manages DASession lifecycle, provides volume lookup,
 * and coordinates with DMAPFS for APFS-specific operations.
 *
 * Reverse-engineered from method names extracted via objdump of the
 * __TEXT,__objc_methname section of /usr/bin/csrutil.
 *
 * Copyright (c) 2025 mSL project — BSD-3-Clause licence.
 */

#ifndef DM_MANAGER_H
#define DM_MANAGER_H

#ifdef __OBJC__

#import <Foundation/Foundation.h>
#import <DiskArbitration/DiskArbitration.h>

@class DMAPFS;

/* ── DMManager ──────────────────────────────────────────────────────
 *
 * Superclass: NSObject
 *
 * DMManager wraps a DASession and provides high-level methods for
 * working with volumes.  It is the primary entry point for
 * DiskManagement.framework.
 *
 * ivars (from disassembly):
 *   _daSession           (DASessionRef)
 *   _defaultDASession    (DASessionRef)
 *   _username            (NSString *)
 *   _fullName            (NSString *)
 *   _uuid                (NSUUID *)
 *   _volumeGroupUUID     (NSUUID *)
 *   _dataDeviceName      (NSString *)
 *   _userName            (NSString *)
 */
@interface DMManager : NSObject

/* ── Properties ───────────────────────────────────────────────────── */

@property (nonatomic, copy, nullable)   NSString *username;
@property (nonatomic, copy, nullable)   NSString *fullName;
@property (nonatomic, strong, nullable) NSUUID   *uuid;
@property (nonatomic, strong, nullable) NSUUID   *volumeGroupUUID;
@property (nonatomic, copy, nullable)   NSString *dataDeviceName;
@property (nonatomic, copy, nullable)   NSString *userName;

/* ── Initialization ───────────────────────────────────────────────── */

/*
 * -[DMManager init]
 *   Default initializer.
 */
- (instancetype)init;

/*
 * -[DMManager initWithDiskArbitrationSession:manager:volumeEntry:volumeGroupUUID:]
 *   Initializes with a DA session, another manager, a volume entry,
 *   and a volume group UUID.
 */
- (instancetype)initWithDiskArbitrationSession:(DASessionRef)session
                                       manager:(DMManager * _Nullable)manager
                                    volumeEntry:(id _Nullable)entry
                               volumeGroupUUID:(NSUUID * _Nullable)groupUUID;

/* ── Volume operations ────────────────────────────────────────────── */

/*
 * -[DMManager copyDiskForArgumentName:timeout:complete:]
 *   Resolves a disk argument name to a DADiskRef.
 *   Timeout in seconds.
 */
- (DADiskRef _Nullable)copyDiskForArgumentName:(NSString * _Nonnull)name
                                       timeout:(NSTimeInterval)timeout
                                      complete:(BOOL * _Nullable)complete;

/*
 * -[DMManager volumeNameForDisk:error:]
 *   Gets the APFS volume name for a given disk.
 */
- (NSString * _Nullable)volumeNameForDisk:(DADiskRef _Nonnull)disk
                                    error:(NSError * _Nullable __autoreleasing * _Nullable)error;

/*
 * -[DMManager isAPFSVolumeDisk:error:]
 *   Checks if a disk is an APFS volume.
 */
- (BOOL)isAPFSVolumeDisk:(DADiskRef _Nonnull)disk
                   error:(NSError * _Nullable __autoreleasing * _Nullable)error;

/*
 * -[DMManager mountDataVolumeAndPerformBlock:]
 *   Mounts the data volume and executes a block.
 *   Pass a block that receives an optional error.
 */
- (void)mountDataVolumeAndPerformBlock:(void (^ _Nonnull)(NSError * _Nullable))block;

/* ── OS installations ─────────────────────────────────────────────── */

/*
 * -[DMManager allEligibleOSInstallations]
 *   Returns an NSSet of all eligible OS installation objects.
 */
- (NSSet * _Nullable)allEligibleOSInstallations;

/*
 * -[DMManager currentlyBootedInstall]
 *   Returns the currently booted OS installation object.
 */
- (id _Nullable)currentlyBootedInstall;

/* ── DA session management ────────────────────────────────────────── */

/*
 * -[DMManager setDefaultDASession:]
 *   Sets the default Disk Arbitration session.
 */
- (void)setDefaultDASession:(DASessionRef)session;

/* ── SIP flag operations ────────────────────────────────────────────
 *
 * These methods are called by csrutil to get/set SIP flags
 * on a per-volume basis.
 *
 * -[DMManager _getSIPflag:]
 *   Returns the value of a named SIP flag.
 *
 * -[DMManager _setSIPflag:enabled:]
 *   Sets a named SIP flag to the given value.
 */

/*
 * -[DMManager _prebootDeviceName]
 *   Returns the preboot device name.
 */

/*
 * -[DMManager _systemDeviceName]
 *   Returns the system device name.
 */

/* ── Credential operations ────────────────────────────────────────── */

/*
 * -[DMManager authenticationContextWithPassword:]
 *   Creates an authentication context from a password.
 *   Used for SIP/authentication flows.
 */

/*
 * -[DMManager setCredential:type:error:]
 *   Sets a credential of a given type.
 */

/*
 * -[DMManager unlockEncryptedVolume:diskUser:anyUser:user:options:details:]
 *   Unlocks an encrypted volume.
 */

/* ── Convenience methods (from NSString category) ─────────────────── */

/*
 * -[DMManager copyDiskForArgumentName:timeout:complete:]
 *   Resolves a disk argument name (e.g. "boot", "data") to a DADiskRef.
 */

@end

/* ── String constants (from __objc_methname) ────────────────────────
 *
 * These are the SIP flag tag names used by DiskManagement.framework:
 *   "sip0" — main SIP flags bitmask
 *   "sip1" — DTrace restricted
 *   "sip2" — kernel debugging restricted
 *   "sip3" — kext signing required
 *   "smb0" — filesystem access restricted
 *   "smb1" — NVRAM access restricted
 *   "smb2" — debugging restricted
 */

/* ── Notification constants ─────────────────────────────────────────
 *
 * DiskManagement posts notifications when volumes are
 * mounted/unmounted or when SIP flags change.
 */

#else
#error "DMManager.h requires Objective-C (--objc or .m files)"
#endif /* __OBJC__ */

#endif /* DM_MANAGER_H */
