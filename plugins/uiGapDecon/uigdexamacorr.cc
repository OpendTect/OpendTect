/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          Sep  2006
 RCS:           $Id: uigdexamacorr.cc,v 1.2 2006-09-26 15:43:45 cvshelene Exp $
________________________________________________________________________

-*/

#include "uigdexamacorr.h"
#include "uigapdeconattrib.h"
#include "gapdeconattrib.h"

#include "attribparam.h"
#include "attribsel.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "attribprocessor.h"
#include "attribfactory.h"
#include "uiexecutor.h"
#include "uimsg.h"
#include "ptrman.h"

using namespace Attrib;

GapDeconACorrView::GapDeconACorrView( uiParent* p )
    : parent_( p )
    , inpid_( DescID::undef() )
{
}


bool GapDeconACorrView::computeAutocorr()
{
    BufferString errmsg;
    PtrMan<EngineMan> aem = createEngineMan();
    PtrMan<Processor> proc = aem->createDataCubesOutput( errmsg, 0  );
    if ( !proc )
    {
	uiMSG().error( errmsg );
	return false;
    }

    proc->setName( "Compute autocorrelation values" );
    uiExecutor dlg( parent_, *proc );
    if ( !dlg.go() )
	return false;

    extractAndSaveVals();
    return true;
}


EngineMan* GapDeconACorrView::createEngineMan()
{
    EngineMan* aem = new EngineMan;

    TypeSet<SelSpec> attribspecs;
    Desc* inpdesc = dset_->getDesc( inpid_ );
    Desc* newdesc = PF().createDescCopy( GapDecon::attribName() );
    if ( !newdesc || !inpdesc )
	return 0;

    mDynamicCastGet( FloatGateParam*,gateparam,
		     newdesc->getValParam(GapDecon::gateStr()) )
    gateparam->setValue( gate_ );
    mDynamicCastGet( BoolParam*,boolparam,
		     newdesc->getValParam(GapDecon::onlyacorrStr()) )
    boolparam->setValue( true );
    newdesc->updateParams();
    newdesc->selectOutput( 0 );
    newdesc->setInput( 0, inpdesc );
    newdesc->setHidden( true );
    newdesc->setUserRef( "autocorrelation" );
    SelSpec sp( 0, dset_->addDesc( newdesc ) );
    attribspecs += sp;

    aem->setAttribSet( dset_ );
    aem->setAttribSpecs( attribspecs );
    aem->setCubeSampling( cs_ );

    return aem;
}


void GapDeconACorrView::extractAndSaveVals()
{
}
