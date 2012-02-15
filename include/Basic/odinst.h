#ifndef odinst_h
#define odinst_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2012
 RCS:           $Id: odinst.h,v 1.2 2012-02-15 16:22:05 cvsbert Exp $
________________________________________________________________________

-*/

#include "enums.h"
class Settings;
class BufferStringSet;


namespace ODInst
{

    enum AutoInstType		{ UseManager, InformOnly, FullAuto, NoAuto  };
				DeclareNameSpaceEnumUtils(AutoInstType)

    mGlobal bool		canInstall();
    mGlobal void		startInstManagement();
    mGlobal bool		updatesAvailable();

    mGlobal AutoInstType	getAutoInstType();	//!< from user settings
    mGlobal void		setAutoInstType(AutoInstType); //!< and store

    mGlobal const BufferStringSet& autoInstTypeUserMsgs();
    mGlobal const char*		sKeyAutoInst();
    mGlobal Settings&		userSettings();

} // namespace

#endif
