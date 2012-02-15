#ifndef odinst_h
#define odinst_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2012
 RCS:           $Id: odinst.h,v 1.1 2012-02-15 13:44:07 cvsbert Exp $
________________________________________________________________________

-*/

#include "enums.h"
class Settings;


namespace ODInst
{

    enum AutoInstType	{ UseManager, InformOnly, FullAuto, NoAuto  };
			DeclareNameSpaceEnumUtils(AutoInstType)

    bool		canInstall();
    void		startInstManagement();
    bool		updatesAvailable();

    AutoInstType	getAutoInstType();		//!< from user settings
    void		setAutoInstType(AutoInstType);	//!< to user settings

    static const char**	autoInstTypeUserMsgs();
    static const char*	sKeyAutoInst();
    Settings&		userSettings();

} // namespace

#endif
