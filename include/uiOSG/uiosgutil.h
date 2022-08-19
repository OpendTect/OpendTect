#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiosgmod.h"

class NotifierAccess;

void mGlobal(uiOSG) setOSGTimerCallbacks( const NotifierAccess&,
					  const NotifierAccess&  );
