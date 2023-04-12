/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "genc.h"
#include "ioman.h"
#include "moddepmgr.h"
#include "uiodstratlayermodelmgr.h"
#include "uiwelllogattrib.h"

void initUISLM( CallBacker* )
{
    uiStratLayerModelManager::initClass();
}

mDefModInitFn(uiWellAttrib)
{
    mIfNotFirstTime( return );

    if ( NeedDataBase() )
    {
	if ( IOMan::isOK() )
	    initUISLM( nullptr );
	else
	    IOMan::iomReady().notify( mSCB(initUISLM) );
    }

    uiWellLogAttrib::initClass();
}
