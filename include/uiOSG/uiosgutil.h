#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		April 2021
________________________________________________________________________

-*/

#include "uiosgmod.h"

class NotifierAccess;

void mGlobal(uiOSG) setOSGTimerCallbacks( const NotifierAccess&,
					  const NotifierAccess&  );
