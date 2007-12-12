/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          Sep 2006
 RCS:           $Id: uigdexamacorr.cc,v 1.22 2007-12-12 15:44:40 cvsbert Exp $
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
#include "attribdataholder.h"
#include "attribdatapack.h"
#include "arrayndimpl.h"
#include "colortab.h"
#include "ptrman.h"
#include "survinfo.h"
#include "uiexecutor.h"
#include "uiflatviewer.h"
#include "uiflatviewmainwin.h"
#include "uiflatviewstdcontrol.h"
#include "uimsg.h"

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


bool GapDeconACorrView::computeAutocorr( bool isqc )
{
    BufferString errmsg;
    RefMan<Attrib::Data2DHolder> d2dh = new Attrib::Data2DHolder();
    PtrMan<EngineMan> aem = createEngineMan();
	
    PtrMan<Processor> proc = dset_->is2D() ? 
			    aem->createScreenOutput2D( errmsg, *d2dh ) 
			    : aem->createDataCubesOutput( errmsg, 0  );
    if ( !proc )
    {
	uiMSG().error( errmsg );
	return false;
    }

    proc->setName( "Compute autocorrelation values" );
    uiExecutor dlg( examwin_, *proc );
    if ( !dlg.go() )
	return false;

    dset_->is2D() ? createFD2DDataPack( isqc, *d2dh )
		  : createFD3DDataPack( isqc, aem, proc );
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
    aem->setLineKey( lk_ );

    CubeSampling cs = cs_;
    if ( !SI().zRange(0).includes( cs_.zrg.start) ||
	 !SI().zRange(0).includes( cs_.zrg.stop) )
    {
	//'fake' a 'normal' cubesampling for the attribute engine
	cs.zrg.start = SI().sampling(0).zrg.start;	
	cs.zrg.stop = cs.zrg.start + cs_.zrg.width();	
    }
    aem->setCubeSampling( cs );

    return aem;
}


#define mCreateFD2DDataPack(fddatapack) \
{ \
    fddatapack = new Attrib::Flat2DDHDataPack( attribid_, *correctd2dh ); \
    fddatapack->setName( "autocorrelation" ); \
}


void GapDeconACorrView::createFD2DDataPack( bool isqc, const Data2DHolder& d2dh)
{
    RefMan<Attrib::Data2DHolder> correctd2dh = new Attrib::Data2DHolder();
    for ( int idx=0; idx<d2dh.dataset_.size(); idx++ )
	correctd2dh->dataset_ += d2dh.dataset_[idx]->clone();
    for ( int idx=0; idx<d2dh.trcinfoset_.size(); idx++ )
    {
	SeisTrcInfo* info = new SeisTrcInfo(*d2dh.trcinfoset_[idx]);
	correctd2dh->trcinfoset_ += info;
    }
    
    if ( ( !SI().zRange(0).includes(cs_.zrg.start)
	|| !SI().zRange(0).includes(cs_.zrg.stop) )
	 && correctd2dh.ptr()->trcinfoset_.size() )
    {
	//we previously 'faked' a 'normal' cubesampling for the attribute engine
	//now we have to go back to the user specified sampling
	float zstep = correctd2dh.ptr()->trcinfoset_[0]->sampling.step;
	for ( int idx=0; idx<correctd2dh.ptr()->dataset_.size(); idx++ )
	    correctd2dh.ptr()->dataset_[idx]->z0_ = mNINT(cs_.zrg.start/zstep);
    }

    if ( isqc )
    	mCreateFD2DDataPack(fddatapackqc_)
    else
	mCreateFD2DDataPack(fddatapackexam_)
}


#define mCreateFD3DDataPack(fddatapack) \
{ \
    fddatapack = new Attrib::Flat3DDataPack( attribid_, *correctoutput, 0 ); \
    fddatapack->setName( "autocorrelation" ); \
}

void GapDeconACorrView::createFD3DDataPack( bool isqc, EngineMan* aem,
					    Processor* proc )
{
    const Attrib::DataCubes* output = aem->getDataCubesOutput( *proc );
    if ( !output )
	return;
    
    output->ref();
    bool csmatchessurv = SI().zRange(0).includes(cs_.zrg.start)
			&& SI().zRange(0).includes(cs_.zrg.stop);
    //if we previously 'faked' a 'normal' cubesampling for the attribute engine
    //we now have to go back to the user specified sampling
    CubeSampling cs = csmatchessurv ? output->cubeSampling() : cs_;
    Attrib::DataCubes* correctoutput = new Attrib::DataCubes();
    correctoutput->ref();
    correctoutput->setSizeAndPos( cs );
    while ( correctoutput->nrCubes() < output->nrCubes() )
	correctoutput->addCube();

    for ( int idx=0; idx<output->nrCubes(); idx++ )
	correctoutput->setCube(idx, output->getCube(idx) );
    
    if ( isqc )
    	mCreateFD3DDataPack(fddatapackqc_)
    else
	mCreateFD3DDataPack(fddatapackexam_)
	    
    output->unRef();
    correctoutput->unRef();
}

    
void GapDeconACorrView::createAndDisplay2DViewer( bool isqc )
{
    if ( isqc )
	qcwin_->close();
    else
	examwin_->close();

    uiFlatViewer& vwr = isqc ? qcwin_->viewer() : examwin_->viewer();
    vwr.setDarkBG( false );
    FlatDataPack* dp = isqc ? fddatapackqc_ : fddatapackexam_;
    vwr.setPack( false, dp->id(), true );
    FlatView::Appearance& app = vwr.appearance();
    app.ddpars_.vd_.rg_ = Interval<float>( -0.2, 0.2 );
    StepInterval<double> newrg( 0, cs_.zrg.stop-cs_.zrg.start, cs_.zrg.step );
    dp->posData().setRange( false, newrg );
    
    isqc ? qcwin_->show() : examwin_->show();
}


void GapDeconACorrView::setDescSet( Attrib::DescSet* ds )
{
    if ( dset_ )
	delete dset_;

    dset_ = ds;
}
