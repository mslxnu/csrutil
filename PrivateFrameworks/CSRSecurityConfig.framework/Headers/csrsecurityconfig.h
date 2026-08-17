/*
 * csrsecurityconfig.h — Reverse-engineered CSRSecurityConfig ObjC class.
 *
 * CSRSecurityConfig is Apple's main ObjC class for reading and writing
 * SIP state.  It lives inside the csrutil binary itself (not a framework).
 *
 * The class has 37 instance methods and 3 class methods.  It wraps the
 * libbootpolicy and ACM calls into an object-oriented interface.
 *
 * All signatures below were recovered from the binary's ObjC metadata
 * via `otool -oV /usr/bin/csrutil`.
 *
 * Copyright (c) 2025 mSL project — BSD-3-Clause licence.
 */

#ifndef CSRUTIL_CSRSECURITYCONFIG_H
#define CSRUTIL_CSRSECURITYCONFIG_H

/*
 * ── CSRSecurityConfig ──────────────────────────────────────────────
 *
 * Superclass: NSObject
 * Instance size: 32 bytes
 *
 * Properties (ObjC runtime names):
 *   rawFlags                    — uint32_t  (CSR config bitmask, "sip0")
 *   DTraceRestricted            — BOOL      (tag "sip1")
 *   KernelDebuggingRestricted   — BOOL      (tag "sip2")
 *   KextSigningRequired         — BOOL      (tag "sip3")
 *   FilesystemAccessRestricted  — BOOL      (tag "smb0")
 *   NVRAMAccessRestricted       — BOOL      (tag "smb1")
 *   DebuggingRestricted         — BOOL      (tag "smb2")
 *   ARVSealingRequired          — BOOL
 *   CTRREnforcementRequired     — BOOL
 *   BootArgFilteringEnabled     — BOOL
 *   ThirdPartyKextLoadingEnabled — BOOL
 *   AppleInternalPolicyAllowed  — BOOL
 *   ResearchGuestAllowed        — BOOL
 *   RecoveryVerificationRequired — BOOL
 *   FirmwareSecurityLevel       — int
 *   HighestCompatibleFirmwareSecurityLevel — int
 *
 * Tag names (used by bootpolicy_get_local_policy_boolean_tag):
 *   sip0  → rawFlags (CSR config bitmask)
 *   sip1  → DTraceRestricted
 *   sip2  → KernelDebuggingRestricted
 *   sip3  → KextSigningRequired
 *   smb0  → FilesystemAccessRestricted
 *   smb1  → NVRAMAccessRestricted
 *   smb2  → DebuggingRestricted
 *
 * Instance methods (37 total):
 *
 *   Getters (is*):
 *     isKextSigningRequired
 *     isFilesystemAccessRestricted
 *     isDebuggingRestricted
 *     isDTraceRestricted
 *     isKernelDebuggingRestricted
 *     isNVRAMAccessRestricted
 *     isARVSealingRequired
 *     isCTRREnforcementRequired
 *     isBootArgFilteringEnabled
 *     isThirdPartyKextLoadingEnabled
 *     isAppleInternalPolicyAllowed
 *     isResearchGuestAllowed
 *     isRecoveryVerificationRequired
 *     isCompatibleWithFirmwareSecurityLevel:
 *     isFileVaultEnabled          (readonly, from CSROSInstall)
 *     isLocked                    (readonly)
 *
 *   Setters (set*):
 *     setKextSigningRequired:
 *     setFilesystemAccessRestricted:
 *     setDebuggingRestricted:
 *     setDTraceRestricted:
 *     setKernelDebuggingRestricted:
 *     setNVRAMAccessRestricted:
 *     setARVSealingRequired:
 *     setCTRREnforcementRequired:
 *     setBootArgFilteringEnabled:
 *     setThirdPartyKextLoadingEnabled:
 *     setAppleInternalPolicyAllowed:
 *     setResearchGuestAllowed:
 *     setRecoveryVerificationRequired:
 *     setFirmwareSecurityLevel:
 *
 *   SIP flag helpers:
 *     _getSIPflag:                — internal, reads a CSR bit
 *     _setSIPflag:enabled:        — internal, sets a CSR bit
 *
 *   Lifecycle:
 *     clearConfigurationForInstallation:usingAuthenticationContext:error:
 *     commitConfigurationToInstallation:usingAuthenticationContext:error:
 *     updateLogLevelFromKext
 *
 * Class methods (3):
 *     +csrsecurityConfigWithRecoveryOS:error:
 *     +csrsecurityConfigForVolume:error:
 *     +csrsecurityConfigForRunningSystem
 */

/*
 * ── CSROSInstall ───────────────────────────────────────────────────
 *
 * Represents a macOS installation on an APFS volume.
 *
 * Instance size: 56 bytes
 *
 * Properties:
 *   name              — NSString *  (readonly)
 *   volumeGroupUUID   — NSUUID *    (readonly)
 *   dataDeviceName    — NSString *  (readonly)
 *   fileVaultEnabled  — BOOL        (readonly)
 *   locked            — BOOL        (readonly)
 *   recoveryUsers     — NSSet *     (readonly)
 *
 * Instance variables:
 *   _dataDeviceName
 *   _prebootDeviceName
 *   _systemDeviceName
 *   _fileVaultEnabled
 *   _name
 *   _volumeGroupUUID
 *
 * Instance methods (11):
 *     initWithDASession:volumeName:error:
 *     initWithGroupUUID:error:
 *     initWithURL:error:
 *     volume
 *     dataVolume
 *     systemVolume
 *     prebootVolume
 *     isEncryptedVolumeLocked:locked:
 *     isAPFSVolumeDisk:error:
 *     clearConfigurationForInstallation:usingAuthenticationContext:error:
 *     commitConfigurationToInstallation:usingAuthenticationContext:error:
 *
 * Class methods (2):
 *     +currentInstallations
 *     +currentInstallationForVolume:error:
 */

/*
 * ── CSRRecoveryUser ────────────────────────────────────────────────
 *
 * Represents a user authorized for Recovery OS operations.
 *
 * Instance size: 32 bytes
 *
 * Properties:
 *   userName — NSString * (readonly)
 *   fullName — NSString * (readonly)
 *   uuid     — NSUUID *   (readonly)
 *
 * Instance variables:
 *   _userName
 *
 * Instance methods (5):
 *     initWithFullName:userName:uuid:
 *     userName
 *     fullName
 *     uuid
 *     dealloc
 *
 * Class method (1):
 *     +recoveryUsers
 */

/*
 * ── FirmwareSecurityLevel ──────────────────────────────────────────
 *
 * Enum values (from the strings and method signatures):
 *   0 = Full Security
 *   1 = Reduced Security
 *   2 = Permissive Security (unknown)
 */

/* Security level constants. */
#define CSR_FIRMWARE_SECURITY_LEVEL_FULL           0
#define CSR_FIRMWARE_SECURITY_LEVEL_REDUCED        1
#define CSR_FIRMWARE_SECURITY_LEVEL_PERMISSIVE     2

/* ACM policy strings (from the csrutil binary). */
#define CSR_ACM_POLICY_AUTHENTICATE_LOCAL_USER     "authenticate_local_user"
#define CSR_ACM_POLICY_RECOVERY_USER_FOR_INSTALL   "authenticate_recovery_user_for_install"

/* Error domains (from the ObjC metadata). */
#define CSR_SECURITY_CONFIG_ERROR_DOMAIN           "CSRSecurityConfigErrorDomain"
#define CSR_ROS_INSTALL_ERROR_DOMAIN               "CSROSInstallErrorDomain"

#endif /* CSRUTIL_CSRSECURITYCONFIG_H */
