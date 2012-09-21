/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne and Kristofer
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "moddepmgr.h"

#include "odsession.h"

mDefModInitFn(uiODMain)
{
    mIfNotFirstTime( return );
    
    ODSessionTranslatorGroup::initClass();
    dgbODSessionTranslator::initClass();

}
