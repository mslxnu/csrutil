/*
 * DMAPFS.h — Reverse-engineered DMAPFS class from DiskManagement.framework.
 *
 * DMAPFS is the APFS-specific volume management class.  It handles:
 *   - Making volumes bootable (signing LocalPolicy)
 *   - Loading/saving SIP configurations
 *   - Committing configuration changes
 *   - Managing firmware security levels
 *   - Unlocking encrypted volumes
 *
 * Reverse-engineered from the __TEXT,__objc_methname section of
 * the csrutil binary.
 *
 * Copyright (c) 2025 mSL project — BSD-3-Clause licence.
 */

#ifndef DM_APFS_H
#define DM_APFS_H

#ifdef __OBJC__

#import <Foundation/Foundation.h>
#import <DiskArbitration/DiskArbitration.h>

@class DMManager;

/* ── Firmware security level enum ─────────────────────────────────── */

typedef NS_ENUM(uint32_t, DMFirmwareSecurityLevel) {
    DMFirmwareSecurityLevelFull       = 0,
    DMFirmwareSecurityLevelReduced   = 1,
    DMFirmwareSecurityLevelPermissive = 2,
};

/* ── DMAPFS ─────────────────────────────────────────────────────────
 *
 * Superclass: NSObject
 *
 * DMAPFS represents an APFS volume and provides methods for
 * modifying its boot configuration, SIP flags, and security settings.
 *
 * ivars (from disassembly):
 *   _manager              (DMManager *)
 *   _daSession            (DASessionRef)
 *   _disk                 (DADiskRef)
 *   _volumeGroupUUID      (NSUUID *)
 *   _configuration        (NSMutableDictionary *)
 *   _dataDeviceName       (NSString *)
 *   _prebootDeviceName    (NSString *)
 *   _systemDeviceName     (NSString *)
 *   _fullName             (NSString *)
 *   _name                 (NSString *)
 *   _uuid                 (NSUUID *)
 *   _userName             (NSString *)
 *
 * Properties from __objc_methname (selrefs):
 *   isARVSealingRequired         (BOOL, getter/setter)
 *   isAppleInternalPolicyAllowed (BOOL, getter/setter)
 *   isBootArgFilteringEnabled    (BOOL, getter/setter)
 *   isCTRREnforcementRequired    (BOOL, getter/setter)
 *   isDTraceRestricted           (BOOL, getter/setter)
 *   isDebuggingRestricted        (BOOL, getter/setter)
 *   isFilesystemAccessRestricted  (BOOL, getter/setter)
 *   isKernelDebuggingRestricted  (BOOL, getter/setter)
 *   isKextSigningRequired        (BOOL, getter/setter)
 *   isNVRAMAccessRestricted      (BOOL, getter/setter)
 *   isRecoveryVerificationRequired (BOOL, getter/setter)
 *   isResearchGuestAllowed       (BOOL, getter/setter)
 *   isThirdPartyKextLoadingEnabled (BOOL, getter/setter)
 *   isFileVaultEnabled           (BOOL, readonly)
 *   isLocked                     (BOOL, readonly)
 */
@interface DMAPFS : NSObject

/* ── Properties ───────────────────────────────────────────────────── */

@property (nonatomic, strong, nullable) DMManager *manager;
@property (nonatomic, copy, nullable)   NSString  *dataDeviceName;
@property (nonatomic, copy, nullable)   NSString  *prebootDeviceName;
@property (nonatomic, copy, nullable)   NSString  *systemDeviceName;
@property (nonatomic, copy, nullable)   NSString  *fullName;
@property (nonatomic, copy, nullable)   NSString  *name;
@property (nonatomic, strong, nullable) NSUUID    *uuid;
@property (nonatomic, copy, nullable)   NSString  *userName;
@property (nonatomic, strong, nullable) NSUUID    *volumeGroupUUID;
@property (nonatomic, strong, nullable) DMFirmwareSecurityLevel firmwareSecurityLevel;

/* ── SIP flag properties ─────────────────────────────────────────── */

@property (nonatomic, assign) BOOL isARVSealingRequired;
@property (nonatomic, assign) BOOL isAppleInternalPolicyAllowed;
@property (nonatomic, assign) BOOL isBootArgFilteringEnabled;
@property (nonatomic, assign) BOOL isCTRREnforcementRequired;
@property (nonatomic, assign) BOOL isDTraceRestricted;
@property (nonatomic, assign) BOOL isDebuggingRestricted;
@property (nonatomic, assign) BOOL isFilesystemAccessRestricted;
@property (nonatomic, assign) BOOL isKernelDebuggingRestricted;
@property (nonatomic, assign) BOOL isKextSigningRequired;
@property (nonatomic, assign) BOOL isNVRAMAccessRestricted;
@property (nonatomic, assign) BOOL isRecoveryVerificationRequired;
@property (nonatomic, assign) BOOL isResearchGuestAllowed;
@property (nonatomic, assign) BOOL isThirdPartyKextLoadingEnabled;
@property (nonatomic, readonly) BOOL isFileVaultEnabled;
@property (nonatomic, readonly) BOOL isLocked;

/* ── Initialization ───────────────────────────────────────────────── */

- (instancetype)initWithManager:(DMManager * _Nonnull)manager;

/* ── Volume bootability ─────────────────────────────────────────────
 *
 * These are the primary methods used by csrutil to make volumes
 * bootable.
 */

/*
 * -[DMAPFS makeVolumeBootable:options:error:]
 *   Makes this volume bootable.
 *   options: dictionary with optional keys like @"groupUUID", @"stagingDirectory".
 */
- (BOOL)makeVolumeBootable:(DADiskRef _Nonnull)disk
                   options:(NSDictionary * _Nullable)options
                      error:(NSError * _Nullable __autoreleasing * _Nullable)error;

/*
 * -[DMAPFS makeBootableForSecurityLevel:usingAuthenticationContext:error:]
 *   Makes this volume bootable for a specific firmware security level.
 *   securityLevel: 0 = Full, 1 = Reduced, 2 = Permissive.
 *   authContext: authentication context for policy signing.
 */
- (BOOL)makeBootableForSecurityLevel:(DMFirmwareSecurityLevel)level
               usingAuthenticationContext:(id _Nonnull)context
                                    error:(NSError * _Nullable __autoreleasing * _Nullable)error;

/* ── Configuration management ─────────────────────────────────────── */

/*
 * -[DMAPFS loadConfigurationForInstallation:error:]
 *   Loads the SIP configuration for an OS installation.
 *   Reads the LocalPolicy and populates the object's properties.
 */
- (BOOL)loadConfigurationForInstallation:(id _Nonnull)installation
                                   error:(NSError * _Nullable __autoreleasing * _Nullable)error;

/*
 * -[DMAPFS loadConfigurationForRunningSystem:]
 *   Loads the SIP configuration for the currently running system.
 *   Used to read current SIP state.
 */
- (BOOL)loadConfigurationForRunningSystem:(NSError * _Nullable __autoreleasing * _Nullable)error;

/*
 * -[DMAPFS clearConfigurationForInstallation:usingAuthenticationContext:error:]
 *   Clears/resets the SIP configuration for an installation.
 */
- (BOOL)clearConfigurationForInstallation:(id _Nonnull)installation
                 usingAuthenticationContext:(id _Nonnull)context
                                     error:(NSError * _Nullable __autoreleasing * _Nullable)error;

/*
 * -[DMAPFS commitConfigurationToInstallation:usingAuthenticationContext:error:]
 *   Commits (writes) the SIP configuration to an installation.
 *   This is the final step that signs and writes the LocalPolicy.
 */
- (BOOL)commitConfigurationToInstallation:(id _Nonnull)installation
                  usingAuthenticationContext:(id _Nonnull)context
                                      error:(NSError * _Nullable __autoreleasing * _Nullable)error;

/* ── Firmware security level ──────────────────────────────────────── */

/*
 * -[DMAPFS firmwareSecurityLevel]
 *   Returns the current firmware security level.
 */

/*
 * -[DMAPFS setFirmwareSecurityLevel:]
 *   Sets the firmware security level.
 */

/*
 * -[DMAPFS highestCompatibleFirmwareSecurityLevel]
 *   Returns the highest compatible firmware security level.
 */

/*
 * -[DMAPFS isCompatibleWithFirmwareSecurityLevel:]
 *   Checks compatibility with a given firmware security level.
 */

/* ── Encrypted volume support ─────────────────────────────────────── */

/*
 * -[DMAPFS isEncryptedVolumeLocked:locked:]
 *   Checks if an encrypted volume is locked.
 */

/*
 * -[DMAPFS unlockEncryptedVolume:diskUser:anyUser:user:options:details:]
 *   Unlocks an encrypted volume.
 */

/* ── SIP flag operations ──────────────────────────────────────────── */

/*
 * -[DMAPFS _getSIPflag:]
 *   Gets a SIP flag by tag name (e.g. "sip0", "sip1", "smb0").
 */

/*
 * -[DMAPFS _setSIPflag:enabled:]
 *   Sets a SIP flag by tag name.
 */

/* ── Raw flags ────────────────────────────────────────────────────── */

/*
 * -[DMAPFS rawFlags]
 *   Returns the raw SIP flags bitmask as an unsigned integer.
 */

/* ── Package support ──────────────────────────────────────────────── */

/*
 * -[DMAPFS launchAndReturnError:]
 *   Launches a package installer.
 */

@end

/* ── NSString category (from objc_methname) ─────────────────────────
 *
 * The csrutil binary contains NSString categories used by
 * DiskManagement.framework:
 *
 *   -[NSString dataUsingEncoding:]        — UTF8 string conversion
 *   -[NSString fileSystemRepresentation]  — C string for POSIX calls
 *   -[NSString UTF8String]                — raw C string
 *   -[NSString UUIDString]                — UUID string representation
 */

/* ── Configuration dictionary keys ──────────────────────────────────
 *
 * These keys are used in the options dictionaries:
 *
 *   @"groupUUID"          — NSUUID, volume group UUID
 *   @"stagingDirectory"   — NSString, path for staging
 *   @"userName"           — NSString, user for authentication
 *   @"fullName"           — NSString, user's full name
 */

#else
#error "DMAPFS.h requires Objective-C (--objc or .m files)"
#endif /* __OBJC__ */

#endif /* DM_APFS_H */
