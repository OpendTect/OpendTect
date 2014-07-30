/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		July 2014
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uisegydirectinserter.h"
#include "segydirecttr.h"
#include "uitoolbutton.h"
#include "uisegyread.h"
#include "uibutton.h"
#include "uimainwin.h"
#include "uimsg.h"

#define mSEGYDirectVolTranslInstance mTranslTemplInstance(SeisTrc,SEGYDirect)



uiSEGYDirectVolInserter::uiSEGYDirectVolInserter()
    : uiIOObjInserter(mSEGYDirectVolTranslInstance)
{
}


uiToolButtonSetup* uiSEGYDirectVolInserter::getButtonSetup() const
{
    uiSEGYDirectVolInserter* self = const_cast<uiSEGYDirectVolInserter*>(this);
    uiToolButtonSetup* ret = new uiToolButtonSetup( "segydirect_ins",
	    uiString( "Scan a SEG-Y file" ),
	    mCB(self,uiSEGYDirectVolInserter,startScan), "SEG-Y" );
    return ret;
}


void uiSEGYDirectVolInserter::startScan( CallBacker* cb )
{
    mDynamicCastGet(uiButton*,but,cb)
    uiParent* par = 0;
    if ( but ) par = but->mainwin();
    if ( !par )
	pErrMsg(BufferString("Unexpected null: ",but?"but":"par"));

    uiSEGYRead::Setup srsu( uiSEGYRead::DirectDef );
    srsu.geoms_.erase(); srsu.geoms_ += Seis::Vol;
    segyread_ = new uiSEGYRead( but->parent(), srsu );
    mAttachCB( segyread_->processEnded, uiSEGYDirectVolInserter::scanComplete );
}


void uiSEGYDirectVolInserter::scanComplete( CallBacker* )
{
    if ( segyread_->state() != uiVarWizard::cFinished() )
	return;

    const MultiID outky( segyread_->outputID() );
    CBCapsule<MultiID> caps( outky, this );
    objectInserted.trigger( &caps );
}


void uiSEGYDirectVolInserter::initClass()
{
    factory().addCreator( create,
			  mSEGYDirectVolTranslInstance.getDisplayName() );
}
