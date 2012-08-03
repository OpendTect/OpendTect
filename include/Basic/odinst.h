#ifndef odinst_h
#define odinst_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2012
 RCS:           $Id: odinst.h,v 1.8 2012-08-03 13:00:13 cvskris Exp $
________________________________________________________________________

-*/

#include "basicmod.h"
#include "enums.h"
class Settings;
class BufferStringSet;


namespace ODInst
{

    mGlobal(Basic) BufferString	GetInstallerDir();
    mGlobal(Basic) bool		canInstall();
    mGlobal(Basic) void		startInstManagement();
    mGlobal(Basic) bool		updatesAvailable();
    mGlobal(Basic) const char*		getPkgVersion(const char* file_pkg_basenm);
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
    mGlobal(Basic) bool		autoInstTypeIsFixed();
    mGlobal(Basic) AutoInstType	getAutoInstType();
    mGlobal(Basic) void		setAutoInstType(AutoInstType);

    mGlobal(Basic) const BufferStringSet& autoInstTypeUserMsgs();
    mGlobal(Basic) const char*		sKeyAutoInst();
    mGlobal(Basic) Settings&		userSettings();
    
    mGlobal(Basic) RelType		getRelType();

} // namespace

#endif

