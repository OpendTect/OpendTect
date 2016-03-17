/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2007
________________________________________________________________________

-*/


#include "legal.h"

static PtrMan< ::Factory<uiString> > inst;

::Factory<uiString>& legalInformation()
{
    if ( !inst )
    {
	::Factory<uiString>* newfact = new ::Factory<uiString>;
	inst.setIfNull(newfact,true);
    }

    return *inst;
}

