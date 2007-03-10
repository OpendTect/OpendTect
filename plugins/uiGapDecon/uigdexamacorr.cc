/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          Sep 2006
 RCS:           $Id: uigdexamacorr.cc,v 1.17 2007-03-10 12:13:46 cvsbert Exp $
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
#include "uiflatviewmainwin.h"
#include "uiflatviewstdcontrol.h"

using namespace Attrib;

#define mCreateFViewWin(nm) \
    nm##win_ = new uiFlatViewMainWin( p, uiFlatViewMainWin::Setup(nm##str)); \
    FlatView::Appearance& app##nm = nm##win_->viewer().appearance(); \
    app##nm.annot_.setAxesAnnot(true); \
    app##nm.setGeoDefaults(true); \
    app##nm.ddpars_.show(false,true); \
    nm##win_->viewer().setDarkBG( true ); \
    nm##win_->addControl( new uiFlatViewStdControl( nm##win_->viewer(), \
			  uiFlatViewStdControl::Setup(p) ) );

GapDeconACorrView::GapDeconACorrView( uiParent* p )
    : attribid_( DescID::undef() )
    , dset_( 0 )
{
    BufferString basestr = " Auto-correlation viewer ";
    BufferString examstr = basestr; examstr +="(examine)";
    BufferString qcstr = basestr; qcstr +="(check parameters)";

    mCreateFViewWin(exam)
    mCreateFViewWin(qc)
}


GapDeconACorrView::~GapDeconACorrView()
{
    if ( dset_ ) delete dset_;
    if ( examwin_ ) delete examwin_;
    if ( qcwin_ ) delete qcwin_;
}


#define mCreateFDDataPack(fddatapack) \
{ \
    fddatapack = new Attrib::Flat3DDataPack( attribid_, *output, 0 ); \
}

bool GapDeconACorrView::computeAutocorr( bool isqc )
{
    //TODO : allow 2D!
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
    	mCreateFDDataPack(fddatapackqc_)
    else
	mCreateFDDataPack(fddatapackexam_)
	    
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
	qcwin_->close();
    else
	examwin_->close();

    uiFlatViewer& vwr = isqc ? qcwin_->viewer() : examwin_->viewer();
    vwr.setPack( false, isqc ? fddatapackqc_ : fddatapackexam_ );
    FlatView::Appearance& app = vwr.appearance();
    app.ddpars_.vd_.rg_ = Interval<float>( -0.2, 0.2 );
    StepInterval<double> newrg( 0, cs_.zrg.stop-cs_.zrg.start, cs_.zrg.step );
    vwr.data().vd_.pos_.setRange( false, newrg );
    
    isqc ? qcwin_->show() : examwin_->show();
}


void GapDeconACorrView::setDescSet( Attrib::DescSet* ds )
{
    if ( dset_ )
	delete dset_;

    dset_ = ds;
}
