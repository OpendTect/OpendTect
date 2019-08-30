/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Huck
 Date:          Sep 2006
________________________________________________________________________

-*/

#include "uigdexamacorr.h"
#include "uigapdeconattrib.h"
#include "gapdeconattrib.h"

#include "attribsel.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "attribprocessor.h"
#include "attribfactory.h"
#include "attribdataholder.h"
#include "flatposdata.h"
#include "seisdatapack.h"
#include "survinfo.h"
#include "uiattribpartserv.h"
#include "uitaskrunner.h"
#include "uiflatviewer.h"
#include "uiflatviewmainwin.h"
#include "uiflatviewstdcontrol.h"
#include "uimsg.h"

using namespace Attrib;

GapDeconACorrView::GapDeconACorrView( uiParent* p )
    : geomid_(mUdfGeomID)
    , dset_( 0 )
    , examwin_(0)
    , qcwin_(0)
    , parent_(p)
{
    const uiString basestr = tr("Auto-correlation viewer ");
    examtitle_ = tr("%1 (examine)").arg(basestr);
    qctitle_ = tr("%1 (check parameters)").arg(basestr);
}


GapDeconACorrView::~GapDeconACorrView()
{
    delete dset_;
    delete examwin_;
    delete qcwin_;
}


bool GapDeconACorrView::computeAutocorr( bool isqc )
{
    uiRetVal uirv;
    RefMan<Data2DHolder> d2dh = new Data2DHolder();
    PtrMan<EngineMan> aem = createEngineMan();

    PtrMan<Processor> proc = dset_->is2D() ?
			    aem->createScreenOutput2D( uirv, *d2dh ) :
			    aem->createDataPackOutput( uirv, 0  );
    if ( !proc )
	{ gUiMsg(parent_).error( uirv ); return false; }

    proc->setName( "Compute autocorrelation values" );
    uiTaskRunner dlg( parent_ );
    if ( !TaskRunner::execute( &dlg, *proc ) )
	return false;

    dset_->is2D() ? createFD2DDataPack( isqc, *d2dh )
		  : createFD3DDataPack( isqc, aem, proc );
    return true;
}


EngineMan* GapDeconACorrView::createEngineMan()
{
    EngineMan* aem = new EngineMan;

    SelSpecList attribspecs;
    SelSpec sp( 0, attribid_ );
    attribspecs += sp;

    aem->setAttribSet( dset_ );
    aem->setAttribSpecs( attribspecs );
    aem->setGeomID( geomid_ );

    TrcKeyZSampling cs = tkzs_;
    if ( !SI().zRange().includes( tkzs_.zsamp_.start, false ) ||
	 !SI().zRange().includes( tkzs_.zsamp_.stop, false ) )
    {
	//'fake' a 'normal' cubesampling for the attribute engine
	cs.zsamp_.start = SI().zRange().start;
	cs.zsamp_.stop = SI().zRange().stop;
    }
    aem->setSubSel( Survey::FullSubSel(cs) );

    return aem;
}


void GapDeconACorrView::createFD2DDataPack( bool isqc, const Data2DHolder& d2dh)
{
    RefMan<Data2DHolder> correctd2dh = new Data2DHolder();
    for ( int idx=0; idx<d2dh.dataset_.size(); idx++ )
	correctd2dh->dataset_ += d2dh.dataset_[idx]->clone();
    for ( int idx=0; idx<d2dh.trcinfoset_.size(); idx++ )
    {
	SeisTrcInfo* info = new SeisTrcInfo(*d2dh.trcinfoset_[idx]);
	correctd2dh->trcinfoset_ += info;
    }

    if ( ( !SI().zRange().includes(tkzs_.zsamp_.start, false )
	|| !SI().zRange().includes(tkzs_.zsamp_.stop, false ) )
	 && correctd2dh.ptr()->trcinfoset_.size() )
    {
	//we previously 'faked' a 'normal' cubesampling for the attribute engine
	//now we have to go back to the user specified sampling
	float zstep = correctd2dh.ptr()->trcinfoset_[0]->sampling_.step;
	for ( int idx=0; idx<correctd2dh.ptr()->dataset_.size(); idx++ )
	    correctd2dh.ptr()->dataset_[idx]->z0_
			= mNINT32(tkzs_.zsamp_.start/zstep);
    }

    TrcKeyZSampling sampling = d2dh.getTrcKeyZSampling();
    sampling.hsamp_.start_.inl() = sampling.hsamp_.stop_.inl()
	    = geomid_.lineNr();

    BufferStringSet cnames; cnames.add( "autocorrelation" );
    const DataPack::ID outputid = uiAttribPartServer::createDataPackFor2D(
						d2dh, SI().zDomain(), &cnames );
    ConstRefMan<RegularSeisDataPack> regsdp =
		DPM(DataPackMgr::SeisID()).get<RegularSeisDataPack>( outputid );
    if ( !regsdp )
	return;

    FlatDataPack*& fdp = isqc ? fddatapackqc_ : fddatapackexam_;
    fdp = new RegularSeisFlatDataPack( *regsdp, 0 );
    DPM(DataPackMgr::FlatID()).add( fdp );
}


void GapDeconACorrView::createFD3DDataPack( bool isqc, EngineMan* aem,
					    Processor* proc )
{
    RefMan<RegularSeisDataPack> output = aem->getDataPackOutput(*proc);
    if ( !output ) return;

    bool csmatchessurv = SI().zRange().includes(tkzs_.zsamp_.start, false )
			&& SI().zRange().includes(tkzs_.zsamp_.stop, false );
    //if we previously 'faked' a 'normal' cubesampling for the attribute engine
    //we now have to go back to the user specified sampling
    if ( !csmatchessurv ) output->setSampling( tkzs_ );

    FlatDataPack*& fdp = isqc ? fddatapackqc_ : fddatapackexam_;
    fdp = new RegularSeisFlatDataPack( *output, 0 );
    DPM(DataPackMgr::FlatID()).add( fdp );
}


bool GapDeconACorrView::setUpViewWin( bool isqc )
{
    FlatDataPack* dp = isqc ? fddatapackqc_ : fddatapackexam_;
    if ( !dp ) return false;
    const StepInterval<double> newrg(
	    0, tkzs_.zsamp_.stop-tkzs_.zsamp_.start, tkzs_.zsamp_.step );

    dp->posData().setRange( false, newrg );

    uiFlatViewMainWin*& fvwin = isqc ? qcwin_ : examwin_;
    if ( fvwin )
	fvwin->viewer().setPack( false, dp->id(), false );
    else
    {
	fvwin = new uiFlatViewMainWin( 0,
		uiFlatViewMainWin::Setup(isqc?qctitle_:examtitle_,false) );
	uiFlatViewer& vwr = fvwin->viewer();
	vwr.setPack( false, dp->id(), true );
	FlatView::Appearance& app = vwr.appearance();
	app.annot_.setAxesAnnot( true );
	app.setDarkBG( false );
	app.setGeoDefaults( true );
	app.ddpars_.show( false, true );
	vwr.appearance().ddpars_.vd_.mapper_->setup().setFixedRange(
					Interval<float>(-1,1) );
	vwr.setInitialSize( uiSize(600,400) );
	fvwin->addControl( new uiFlatViewStdControl(vwr,
			uiFlatViewStdControl::Setup(0).isvertical(true)) );
    }

    return true;
}


void GapDeconACorrView::createAndDisplay2DViewer( bool isqc )
{
    if ( setUpViewWin( isqc ) )
	isqc ? qcwin_->show() : examwin_->show();
    else
	gUiMsg(parent_).error(
		tr("The window start and stop should be different.\n"
		   "Please correct the window parameters before"
		   " restarting the computation") );
}


void GapDeconACorrView::setDescSet( DescSet* ds )
{
    delete dset_;
    dset_ = ds;
}
