/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uigdexamacorr.h"

#include "attribdataholder.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "attribprocessor.h"
#include "attribsel.h"
#include "flatposdata.h"
#include "seisdatapack.h"
#include "survinfo.h"

#include "uiattribpartserv.h"
#include "uiflatviewer.h"
#include "uiflatviewmainwin.h"
#include "uiflatviewstdcontrol.h"
#include "uimsg.h"
#include "uitaskrunner.h"

using namespace Attrib;


GapDeconACorrView::GapDeconACorrView( uiParent* p )
    : parent_(p)
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
    uiString errmsg;
    RefMan<Attrib::Data2DHolder> d2dh = new Attrib::Data2DHolder();
    PtrMan<EngineMan> aem = createEngineMan();

    PtrMan<Processor> proc = dset_->is2D() ?
			    aem->createScreenOutput2D( errmsg, *d2dh ) :
			    aem->createDataPackOutput( errmsg, 0  );
    if ( !proc )
    {
	uiMSG().error( errmsg );
	return false;
    }

    proc->setName( "Compute autocorrelation values" );
    uiTaskRunner dlg( parent_ );
    if ( !TaskRunner::execute( &dlg, *proc ) )
	return false;

    dset_->is2D() ? createFD2DDataPack( isqc, *d2dh )
		  : createFD3DDataPack( isqc, aem.ptr(), proc.ptr() );
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
    aem->setGeomID( geomid_ );

    TrcKeyZSampling cs = tkzs_;
    if ( !SI().zRange(0).includes( tkzs_.zsamp_.start_, false ) ||
	 !SI().zRange(0).includes( tkzs_.zsamp_.stop_, false ) )
    {
	//'fake' a 'normal' cubesampling for the attribute engine
	cs.zsamp_.start_ = SI().sampling(0).zsamp_.start_;
	cs.zsamp_.stop_ = cs.zsamp_.start_ + tkzs_.zsamp_.width();
    }
    aem->setTrcKeyZSampling( cs );

    return aem;
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

    if ( ( !SI().zRange(0).includes(tkzs_.zsamp_.start_, false )
	   || !SI().zRange(0).includes(tkzs_.zsamp_.stop_, false ) )
	 && correctd2dh.ptr()->trcinfoset_.size() )
    {
	//we previously 'faked' a 'normal' cubesampling for the attribute engine
	//now we have to go back to the user specified sampling
	float zstep = correctd2dh.ptr()->trcinfoset_[0]->sampling_.step_;
	for ( int idx=0; idx<correctd2dh.ptr()->dataset_.size(); idx++ )
	    correctd2dh.ptr()->dataset_[idx]->z0_
		    = mNINT32(tkzs_.zsamp_.start_/zstep);
    }

    TrcKeyZSampling sampling = d2dh.getTrcKeyZSampling();
    sampling.hsamp_.setGeomID( geomid_ );

    BufferStringSet cnames;
    cnames.add( "autocorrelation" );
    RefMan<RegularSeisDataPack> regsdp =
	uiAttribPartServer::createDataPackFor2DRM( d2dh, sampling,
					SI().zDomainInfo(), &cnames );
    if ( !regsdp )
	return;

    RefMan<FlatDataPack> fdp = new RegularSeisFlatDataPack( *regsdp, 0 );
    if ( isqc )
	fddatapackqc_ = fdp;
    else
	fddatapackexam_ = fdp;
}


void GapDeconACorrView::createFD3DDataPack( bool isqc, EngineMan* aem,
					    Processor* proc )
{
    RefMan<RegularSeisDataPack> output = aem->getDataPackOutput( *proc );
    if ( !output ) return;

    bool csmatchessurv = SI().zRange(0).includes(tkzs_.zsamp_.start_, false )
			 && SI().zRange(0).includes(tkzs_.zsamp_.stop_, false );
    //if we previously 'faked' a 'normal' cubesampling for the attribute engine
    //we now have to go back to the user specified sampling
    if ( !csmatchessurv )
	output->setSampling( tkzs_ );

    RefMan<FlatDataPack> fdp = new RegularSeisFlatDataPack( *output, 0 );
    if ( isqc )
	fddatapackqc_ = fdp;
    else
	fddatapackexam_ = fdp;
}


bool GapDeconACorrView::setUpViewWin( bool isqc )
{
    RefMan<FlatDataPack> dp = isqc ? fddatapackqc_ : fddatapackexam_;
    if ( !dp ) return false;
    const StepInterval<double> newrg(
		0, tkzs_.zsamp_.stop_-tkzs_.zsamp_.start_, tkzs_.zsamp_.step_ );

    dp->posData().setRange( false, newrg );

    uiFlatViewMainWin*& fvwin = isqc ? qcwin_ : examwin_;
    if ( fvwin )
	fvwin->viewer().setPack( FlatView::Viewer::VD, dp.ptr(), false );
    else
    {
	fvwin = new uiFlatViewMainWin( 0,
		uiFlatViewMainWin::Setup(isqc?qctitle_:examtitle_,false) );
	uiFlatViewer& vwr = fvwin->viewer();
	vwr.setPack( FlatView::Viewer::VD, dp.ptr(), true );
	FlatView::Appearance& app = vwr.appearance();
	app.annot_.setAxesAnnot( true );
	app.setDarkBG( false );
	app.setGeoDefaults( true );
	app.ddpars_.show( false, true );
	vwr.appearance().ddpars_.vd_.mappersetup_.type_ =
	    ColTab::MapperSetup::Fixed;
	vwr.appearance().ddpars_.vd_.mappersetup_.range_ =Interval<float>(-1,1);
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
	uiMSG().error( tr( "The window start and stop should be different;\n"
			   "Please correct the window parameters before"
			   " restarting the computation" ) );
}


void GapDeconACorrView::setDescSet( Attrib::DescSet* ds )
{
    delete dset_;
    dset_ = ds;
}
