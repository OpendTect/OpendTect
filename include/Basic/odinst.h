#ifndef odinst_h
#define odinst_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2012
 RCS:           $Id: odinst.h,v 1.3 2012-02-21 09:31:28 cvsbert Exp $
________________________________________________________________________

-*/

#include "enums.h"
class Settings;
class BufferStringSet;


namespace ODInst
{

    mGlobal bool		canInstall();
    mGlobal void		startInstManagement();
    mGlobal bool		updatesAvailable();


    enum AutoInstType		{ UseManager, InformOnly, FullAuto, NoAuto  };
				DeclareNameSpaceEnumUtils(AutoInstType)
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

} // namespace

#endif
