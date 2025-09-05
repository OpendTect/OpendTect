/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
    {
	mDynamicCastGet(uiParent*,directpar,cb)
	par = directpar ? directpar : uiMainWin::activeWindow();
    }

    if ( !par )
    {
	pErrMsg("Should have at least a ActiveWindow as parent");
	return;
    }


    uiBulkLogImport impdlg( par );
    impdlg.setModal( true );
    impdlg.go();
    insertFinalized().trigger();
}


void uiWellInserter::initClass()
{
    const Translator& transl = mWellTranslInstance;
    factory().addCreator( create, transl.getDisplayName() );
}
