/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		July 2014
________________________________________________________________________

-*/

#include "uisegydirectinserter.h"

#include "segydirecttr.h"
#include "seis2dlineio.h"
#include "uibutton.h"
#include "uimainwin.h"
#include "uimsg.h"
#include "uisegyread.h"
#include "uistrings.h"
#include "uitoolbutton.h"

#define mSEGYDirectVolTranslInstance mTranslTemplInstance(SeisTrc,SEGYDirect)



uiSEGYDirectVolInserter::uiSEGYDirectVolInserter()
    : uiIOObjInserter(mSEGYDirectVolTranslInstance)
{
}


uiSEGYDirectVolInserter::~uiSEGYDirectVolInserter()
{
    detachAllNotifiers();
}


uiToolButtonSetup* uiSEGYDirectVolInserter::getButtonSetup() const
{
    uiSEGYDirectVolInserter* self = const_cast<uiSEGYDirectVolInserter*>(this);
    uiToolButtonSetup* ret = new uiToolButtonSetup( "segydirect_ins",
						    tr("Scan a SEG-Y file"),
		mCB(self,uiSEGYDirectVolInserter,startScan),
		uiStrings::sSEGY() );
    return ret;
}


void uiSEGYDirectVolInserter::startScan( CallBacker* cb )
{
    mDynamicCastGet(uiButton*,but,cb)
    uiParent* par = 0;
    if ( but ) par = but->mainwin();
    if ( !par )
    { pErrMsg(BufferString("Unexpected null: ",but?"but":"par")); }

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
    const Translator& transl = mSEGYDirectVolTranslInstance;
    factory().addCreator( create, transl.getDisplayName() );
}



#define mSEGYDirect2DTranslInstance mTranslTemplInstance(SeisTrc2D,SEGYDirect)


uiSEGYDirect2DInserter::uiSEGYDirect2DInserter()
    : uiIOObjInserter(mSEGYDirect2DTranslInstance)
{
}


uiSEGYDirect2DInserter::~uiSEGYDirect2DInserter()
{
    detachAllNotifiers();
}


uiToolButtonSetup* uiSEGYDirect2DInserter::getButtonSetup() const
{
    uiSEGYDirect2DInserter* self = const_cast<uiSEGYDirect2DInserter*>(this);
    uiToolButtonSetup* ret = new uiToolButtonSetup(
	"segydirect_ins", tr("Scan a SEG-Y file"),
	mCB(self,uiSEGYDirect2DInserter,startScan), uiStrings::sSEGY() );
    return ret;
}


void uiSEGYDirect2DInserter::startScan( CallBacker* cb )
{
    mDynamicCastGet(uiButton*,but,cb)
    uiParent* par = 0;
    if ( but ) par = but->mainwin();
    if ( !par )
    { pErrMsg(BufferString("Unexpected null: ",but?"but":"par")); }

    uiSEGYRead::Setup srsu( uiSEGYRead::DirectDef );
    srsu.geoms_.erase(); srsu.geoms_ += Seis::Line;
    segyread_ = new uiSEGYRead( but->parent(), srsu );
    mAttachCB( segyread_->processEnded, uiSEGYDirect2DInserter::scanComplete );
}


void uiSEGYDirect2DInserter::scanComplete( CallBacker* )
{
    if ( segyread_->state() != uiVarWizard::cFinished() )
	return;

    const MultiID outky( segyread_->outputID() );
    CBCapsule<MultiID> caps( outky, this );
    objectInserted.trigger( &caps );
}


void uiSEGYDirect2DInserter::initClass()
{
    const Translator& transl = mSEGYDirect2DTranslInstance;
    factory().addCreator( create, transl.getDisplayName() );
}

//Prestack 3D

#define mSEGYDirectPS3DTranslInstance mTranslTemplInstance(SeisPS3D,SEGYDirect)

uiSEGYDirectPS3DInserter::uiSEGYDirectPS3DInserter()
    : uiIOObjInserter(mSEGYDirectPS3DTranslInstance)
{
}


uiSEGYDirectPS3DInserter::~uiSEGYDirectPS3DInserter()
{
    detachAllNotifiers();
}


uiToolButtonSetup* uiSEGYDirectPS3DInserter::getButtonSetup() const
{
    uiSEGYDirectPS3DInserter* self =
				   const_cast<uiSEGYDirectPS3DInserter*>(this);
    uiToolButtonSetup* ret = new uiToolButtonSetup(
	"segydirect_ins", tr("Scan a SEG-Y file"),
	mCB(self,uiSEGYDirectPS3DInserter,startScan), uiStrings::sSEGY() );
    return ret;
}


void uiSEGYDirectPS3DInserter::startScan( CallBacker* cb )
{
    mDynamicCastGet(uiButton*,but,cb)
    uiParent* par = 0;
    if ( but ) par = but->mainwin();
    if ( !par )
    { pErrMsg(BufferString("Unexpected null: ",but?"but":"par")); }

    uiSEGYRead::Setup srsu( uiSEGYRead::DirectDef );
    srsu.geoms_.erase(); srsu.geoms_ += Seis::VolPS;
    segyread_ = new uiSEGYRead( but->parent(), srsu );
    mAttachCB( segyread_->processEnded,
				    uiSEGYDirectPS3DInserter::scanComplete );
}


void uiSEGYDirectPS3DInserter::scanComplete( CallBacker* )
{
    if ( segyread_->state() != uiVarWizard::cFinished() )
	return;

    const MultiID outky( segyread_->outputID() );
    CBCapsule<MultiID> caps( outky, this );
    objectInserted.trigger( &caps );
}


void uiSEGYDirectPS3DInserter::initClass()
{
    const Translator& transl = mSEGYDirectPS3DTranslInstance;
    factory().addCreator( create, transl.getDisplayName() );
}
