#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "enums.h"

class Settings;
class BufferStringSet;

/*!\brief OpendTect Installation*/

namespace OS { class MachineCommand; }

namespace ODInst
{
    enum class ActionType;

    mGlobal(Basic) const char*	sKeyHasUpdate();
    mGlobal(Basic) const char*	sKeyHasNoUpdate();

    mGlobal(Basic) BufferString GetRelInfoDir();
    mGlobal(Basic) bool		HasInstaller();
    mGlobal(Basic) bool		canInstall(const char* dirnm);
    mGlobal(Basic) void		startInstManagement(ActionType);
    mGlobal(Basic) void		startUpdateCheck(CallBack);
    mGlobal(Basic) bool		updatesAvailable(int inited=-1);
    mDeprecated("Use updatesAvailable")
    mGlobal(Basic) bool		runInstMgrForUpdt();
    mGlobal(Basic) const char*	getPkgVersion(const char* file_pkg_basenm);
				//!< For example, the Base pkg has "base"
    inline bool			isErrPkgVersion( const char* s )
				{ return !s || !*s || *s == '['; }

    enum AutoInstType		{ UseManager, InformOnly, FullAuto, NoAuto  };
				mDeclareNameSpaceEnumUtils(Basic,AutoInstType)
    enum RelType		{ Stable, Development, PreStable,
				  PreDevelopment, Ancient, OtherRelease };
				mDeclareNameSpaceEnumUtils(Basic,RelType)
    enum class ActionType { Standard, Install, Manage, Uninstall, Update,
			    UpdateCheck };
    /*!
	Policy: Externally, policy can be fixed using OD_INSTALLER_POLICY
	Without OD_INSTALLER_POLICY, read/write user settings
	OD_INSTALLER_POLICY must be the enum string (i.e. the keys in settings)
      */
    mGlobal(Basic) bool		autoInstTypeIsFixed();
    mGlobal(Basic) AutoInstType getAutoInstType();
    mGlobal(Basic) void		setAutoInstType(AutoInstType);

    mGlobal(Basic) const BufferStringSet& autoInstTypeUserMsgs();
    mGlobal(Basic) const char*		sKeyAutoInst();
    mGlobal(Basic) Settings&		userSettings();

    mGlobal(Basic) RelType		getRelType();

    mDeprecated("Use HasInstaller")
    mGlobal(Basic) BufferString GetInstallerDir();
    mDeprecatedObs
    mGlobal(Basic) BufferString getInstallerPlfDir();
    mDeprecated("Provide ActionType")
    mGlobal(Basic) void		startInstManagement();
    mDeprecated("Use startInstManagement")
    mGlobal(Basic) void		startInstManagementWithRelDir(const char*);
    mDeprecated("Use getUpdateMC")
    mGlobal(Basic) void		getMachComm(const char*, OS::MachineCommand&);

} // namespace ODInst

// Deprecated macro, will be removed
#ifdef __mac__
#define mInstallerDirNm "OpendTect Installer.app"
#else
#define mInstallerDirNm "Installer"
#endif
