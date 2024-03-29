/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisegydirectinserter.h"

#include "ioman.h"
#include "segydirecttr.h"
#include "seis2dlineio.h"
#include "uibutton.h"
#include "uimainwin.h"
#include "uimsg.h"
#include "uisegyreadstarter.h"
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
						    tr("Import a SEG-Y file"),
		mCB(self,uiSEGYDirectVolInserter,startScan),
		uiStrings::sSEGY() );
    return ret;
}


void uiSEGYDirectVolInserter::startScan( CallBacker* cb )
{
    mDynamicCastGet(uiButton*,but,cb)
    uiParent* par = nullptr;
    if ( but )
	par = but->mainwin();

    if ( !par )
    {
	pErrMsg( BufferString("Unexpected null: ",but?"but":"par") );
    }

    SEGY::ImpType imptype( Seis::Vol );
    uiSEGYReadStarter dlg( par, false, &imptype );
    dlg.setModal( true );
    dlg.go();

    const MultiID outky = dlg.getOutputKey();
    if ( outky.isUdf() )
	return;

    objInserterd.trigger( outky );
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
	"segydirect_ins", tr("Import a SEG-Y file"),
	mCB(self,uiSEGYDirect2DInserter,startScan), uiStrings::sSEGY() );
    return ret;
}


void uiSEGYDirect2DInserter::startScan( CallBacker* cb )
{
    mDynamicCastGet(uiButton*,but,cb)
    uiParent* par = nullptr;
    if ( but )
	par = but->mainwin();

    if ( !par )
    {
	pErrMsg( BufferString("Unexpected null: ",but?"but":"par") );
    }

    SEGY::ImpType imptype( Seis::Line );
    uiSEGYReadStarter dlg( par, false, &imptype );
    dlg.setModal( true );
    dlg.go();

    const MultiID outky = dlg.getOutputKey();
    if ( outky.isUdf() )
	return;

    objInserterd.trigger(outky);
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
	"segydirect_ins", tr("Import a SEG-Y file"),
	mCB(self,uiSEGYDirectPS3DInserter,startScan), uiStrings::sSEGY() );
    return ret;
}


void uiSEGYDirectPS3DInserter::startScan( CallBacker* cb )
{
    mDynamicCastGet(uiButton*,but,cb)
    uiParent* par = nullptr;
    if ( but )
	par = but->mainwin();

    if ( !par )
    {
	pErrMsg( BufferString("Unexpected null: ",but?"but":"par") );
    }

    SEGY::ImpType imptype( Seis::VolPS );
    uiSEGYReadStarter dlg( par, false, &imptype );
    dlg.setModal( true );
    dlg.go();

    const MultiID outky = dlg.getOutputKey();
    if ( outky.isUdf() )
	return;

    objInserterd.trigger(outky);
}


void uiSEGYDirectPS3DInserter::initClass()
{
    const Translator& transl = mSEGYDirectPS3DTranslInstance;
    factory().addCreator( create, transl.getDisplayName() );
}
