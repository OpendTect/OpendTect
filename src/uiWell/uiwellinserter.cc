/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		August 2020
________________________________________________________________________

-*/


#include "uiwellinserter.h"

#include "welltransl.h"

#include "uibulkwellimp.h"
#include "uimainwin.h"
#include "uistrings.h"
#include "uitoolbutton.h"

#define mWellTranslInstance mTranslTemplInstance(Well,od)


uiWellInserter::uiWellInserter()
    : uiIOObjInserter(mWellTranslInstance)
{
}


uiWellInserter::~uiWellInserter()
{
    detachAllNotifiers();
}


uiToolButtonSetup* uiWellInserter::getButtonSetup() const
{
    uiWellInserter* self = const_cast<uiWellInserter*>(this);
    uiToolButtonSetup* ret = new uiToolButtonSetup( "welllog",
						    tr("Import LAS file"),
		mCB(self,uiWellInserter,startRead),
		uiStrings::sWellLog() );
    return ret;
}


void uiWellInserter::startRead( CallBacker* cb )
{
    mDynamicCastGet(uiButton*,but,cb)
    uiParent* par = but ? but->mainwin() : nullptr;
    if ( !par )
	return;

    uiBulkLogImport impdlg( par );
    impdlg.setModal( true );
    impdlg.go();
}


void uiWellInserter::initClass()
{
    const Translator& transl = mWellTranslInstance;
    factory().addCreator( create, transl.getDisplayName() );
}

