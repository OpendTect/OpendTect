#ifndef odinst_h
#define odinst_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2012
 RCS:           $Id: odinst.h,v 1.7 2012-07-09 12:10:03 cvsraman Exp $
________________________________________________________________________

-*/

#include "enums.h"
class Settings;
class BufferStringSet;


namespace ODInst
{

    mGlobal BufferString	GetInstallerDir();
    mGlobal bool		canInstall();
    mGlobal void		startInstManagement();
    mGlobal bool		updatesAvailable();
    mGlobal const char*		getPkgVersion(const char* file_pkg_basenm);
    				//!< For example, the Base pkg has "base" 
    inline bool			isErrPkgVersion( const char* s )
				{ return !s || !*s || *s == '['; }


    enum AutoInstType		{ UseManager, InformOnly, FullAuto, NoAuto  };
				DeclareNameSpaceEnumUtils(AutoInstType)
    enum RelType		{ Stable, Development, PreStable,
				  PreDevelopment, Ancient, Other };
				DeclareNameSpaceEnumUtils(RelType)
    /*!
	Policy: Externally, policy can be fixed using OD_INSTALLER_POLICY
	Without OD_INSTALLER_POLICY, read/write user settings
	OD_INSTALLER_POLICY must be the enum string (i.e. the keys in settings)
      */
    mGlobal bool		autoInstTypeIsFixed();
    mGlobal AutoInstType	getAutoInstType();
    mGlobal void		setAutoInstType(AutoInstType);

    mGlobal const BufferStringSet& autoInstTypeUserMsgs();
    mGlobal const char*		sKeyAutoInst();
    mGlobal Settings&		userSettings();
    
    mGlobal RelType		getRelType();

} // namespace

#endif
