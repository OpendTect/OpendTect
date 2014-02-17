/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Feb 2014
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uitexttranslator.h"

#include "ptrman.h"



mGlobal( uiBase )  TextTranslateMgr& TrMgr()
{
    mDefineStaticLocalObject( PtrMan<TextTranslateMgr>, trmgr, = 0 );
    if ( !trmgr ) trmgr = new TextTranslateMgr();
    return *trmgr;
}


