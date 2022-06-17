#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Feb 2012
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
    mGlobal(Basic) const char*	sKeyHasUpdate();
    mGlobal(Basic) const char*	sKeyHasNoUpdate();

    mGlobal(Basic) BufferString GetInstallerDir();
    mGlobal(Basic) BufferString GetRelInfoDir();
    mGlobal(Basic) bool		canInstall(const char* dirnm);
    mGlobal(Basic) void		startInstManagement();
    mGlobal(Basic) void		startInstManagementWithRelDir(const char*);

    mGlobal(Basic) void		getMachComm(const char*, OS::MachineCommand&);
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
				  PreDevelopment, Ancient, Other };
				mDeclareNameSpaceEnumUtils(Basic,RelType)
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
    mGlobal(Basic) BufferString		getInstallerPlfDir();


} // namespace

#ifdef __mac__
#define mInstallerDirNm "OpendTect Installer.app"
#else
#define mInstallerDirNm "Installer"
#endif

