/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2007
 RCS:           $Id$
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "legal.h"

static PtrMan< ::Factory<uiString> > inst;

::Factory<uiString>& legalInformation()
{
    if ( !inst )
    {
	::Factory<uiString>* newfact = new ::Factory<uiString>;
	if ( !inst.setIfNull(newfact) )
	    delete newfact;
    }

    return *inst;
}

