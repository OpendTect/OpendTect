/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          Sep 2006
 RCS:           $Id: uigdexamacorr.cc,v 1.11 2007-01-30 11:40:08 cvshelene Exp $
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
#include "attribdatacubes.h"
#include "attribdatapack.h"
#include "uiexecutor.h"
#include "arrayndimpl.h"
#include "uimsg.h"
#include "ptrman.h"
#include "uiflatviewgen.h"
#include "colortab.h"

using namespace Attrib;

GapDeconACorrView::GapDeconACorrView( uiParent* p )
    : attribid_( DescID::undef() )
    , datapackexam_( 0 )
    , datapackqc_( 0 )
    , examviewergen_( 0 )
    , qcviewergen_( 0 )
    , dset_( 0 )
{
    examwin_ = new uiMainWin( p, "Auto-correlation viewer (examine)", 0,
	    		      false, true );
    qcwin_ = new uiMainWin( p, "Auto-correlation viewer (check parameters)", 0,
	    		    false, true );
}


GapDeconACorrView::~GapDeconACorrView()
{
    if ( datapackexam_ ) delete datapackexam_;
    if ( datapackqc_ ) delete datapackqc_;
    if ( examviewergen_ ) delete examviewergen_;
    if ( qcviewergen_ ) delete qcviewergen_;
    if ( dset_ ) delete dset_;
    if ( examwin_ ) delete examwin_;
    if ( qcwin_ ) delete qcwin_;
}


#define mCreateDataPack( datapack ) \
{ \
    if ( datapack ) delete datapack; \
    datapack = new CubeDataPack( *output ); \
}\

bool GapDeconACorrView::computeAutocorr( bool isqc )
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
    uiExecutor dlg( examwin_, *proc );
    if ( !dlg.go() )
	return false;

    const Attrib::DataCubes* output = aem->getDataCubesOutput( *proc );
    if ( !output )
	return false;
    
    output->ref();
    if ( isqc )
    	mCreateDataPack(datapackqc_)
    else
	mCreateDataPack(datapackexam_)
    output->unRef();
    return true;
}


EngineMan* GapDeconACorrView::createEngineMan()
{
    EngineMan* aem = new EngineMan;

    TypeSet<SelSpec> attribspecs;
    SelSpec sp( 0, attribid_ );
    attribspecs += sp;

    aem->setAttribSet( dset_ );
    aem->setAttribSpecs( attribspecs );
    aem->setCubeSampling( cs_ );

    return aem;
}


void GapDeconACorrView::createAndDisplay2DViewer( bool isqc )
{
    if ( isqc )
    {
	qcwin_->close();
	if ( qcviewergen_ ) delete qcviewergen_;
	qcviewergen_ = new FlatDisp::uiFlatViewGen( qcwin_, true, true );
    }
    else
    {
	examwin_->close();
	if ( examviewergen_ ) delete examviewergen_;
	examviewergen_ = new FlatDisp::uiFlatViewGen( examwin_, true, true );
    }

    FlatDisp::uiFlatViewGen* viewergen = isqc ? qcviewergen_ : examviewergen_;
    viewergen->setData( isqc ? datapackqc_ : datapackexam_, false );
    FlatDisp::Context& ctxt = viewergen->getContext();
    ctxt.ddpars_.vd_.rg_ = Interval<float>( -0.2, 0.2 );
    ctxt.posdata_.x2rg_ = StepInterval<double>(0,cs_.zrg.stop-cs_.zrg.start,
			                       cs_.zrg.step);
    
    isqc ? qcwin_->show() : examwin_->show();
}


void GapDeconACorrView::setDescSet( Attrib::DescSet* ds )
{
    if ( dset_ )
	delete dset_;

    dset_ = ds;
}
