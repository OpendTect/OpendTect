/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne and Kristofer
 Date:          December 2007
________________________________________________________________________

-*/
static const char* mUnusedVar rcsID = "$Id: inituiodmain.cc,v 1.2 2012-05-02 11:53:47 cvskris Exp $";

#include "moddepmgr.h"

#include "odsession.h"

mDefModInitFn(uiODMain)
{
    mIfNotFirstTime( return );
    
    ODSessionTranslatorGroup::initClass();
    dgbODSessionTranslator::initClass();

}
