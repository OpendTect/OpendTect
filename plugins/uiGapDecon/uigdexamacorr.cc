/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          Sep 2006
 RCS:           $Id: uigdexamacorr.cc,v 1.15 2007-02-23 09:35:33 cvsbert Exp $
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
#include "colortab.h"
#include "uiflatviewer.h"

using namespace Attrib;

GapDeconACorrView::GapDeconACorrView( uiParent* p )
    : attribid_( DescID::undef() )
    , dset_( 0 )
{
    examwin_ = new uiMainWin( p, "Auto-correlation viewer (examine)", 0,
	    		      false, true );
    qcwin_ = new uiMainWin( p, "Auto-correlation viewer (check parameters)", 0,
	    		    false, true );
}


GapDeconACorrView::~GapDeconACorrView()
{
    if ( dset_ ) delete dset_;
    if ( examwin_ ) delete examwin_;
    if ( qcwin_ ) delete qcwin_;
}


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
    /*
    if ( isqc )
    	mCreateFDDataPack(fddatapackqc_)
    else
	mCreateFDDataPack(fddatapackexam_)
	*/
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
    /*
    if ( isqc )
    {
	qcwin_->close();
	if ( qcdpview_ ) delete qcdpview_;
	qcdpview_ = new FlatView::uiViewFDDataPack( qcwin_, true,
					FlatView::uiViewFDDataPack::Setup() );
    }
    else
    {
	examwin_->close();
	if ( examdpview_ ) delete examdpview_;
	examdpview_ = new FlatView::uiViewFDDataPack( examwin_, true,
					FlatView::uiViewFDDataPack::Setup() );
    }

    FlatView::uiViewFDDataPack* fddpview = isqc ? qcdpview_ : examdpview_;
    fddpview->setData( isqc ? fddatapackqc_ : fddatapackexam_, false );
    FlatView::Context& ctxt = fddpview->getContext();
    ctxt.ddpars_.vd_.rg_ = Interval<float>( -0.2, 0.2 );
    ctxt.posdata_.x2rg_ = StepInterval<double>(0,cs_.zrg.stop-cs_.zrg.start,
			                       cs_.zrg.step);
    
    isqc ? qcwin_->show() : examwin_->show();
    */
}


void GapDeconACorrView::setDescSet( Attrib::DescSet* ds )
{
    if ( dset_ )
	delete dset_;

    dset_ = ds;
}
