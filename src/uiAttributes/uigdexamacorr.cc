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


GapDeconACorrView::GapDeconACorrView( uiParent* p )
    : attribid_( DescID::undef() )
    , geomid_(Survey::GM().cUndefGeomID())
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
    aem->setGeomID( geomid_ );

    TrcKeyZSampling cs = tkzs_;
    if ( !SI().zRange(0).includes( tkzs_.zsamp_.start, false ) ||
	 !SI().zRange(0).includes( tkzs_.zsamp_.stop, false ) )
    {
	//'fake' a 'normal' cubesampling for the attribute engine
	cs.zsamp_.start = SI().sampling(0).zsamp_.start;
	cs.zsamp_.stop = cs.zsamp_.start + tkzs_.zsamp_.width();
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

    if ( ( !SI().zRange(0).includes(tkzs_.zsamp_.start, false )
	|| !SI().zRange(0).includes(tkzs_.zsamp_.stop, false ) )
	 && correctd2dh.ptr()->trcinfoset_.size() )
    {
	//we previously 'faked' a 'normal' cubesampling for the attribute engine
	//now we have to go back to the user specified sampling
	float zstep = correctd2dh.ptr()->trcinfoset_[0]->sampling.step;
	for ( int idx=0; idx<correctd2dh.ptr()->dataset_.size(); idx++ )
	    correctd2dh.ptr()->dataset_[idx]->z0_
			= mNINT32(tkzs_.zsamp_.start/zstep);
    }

    TrcKeyZSampling sampling = d2dh.getTrcKeyZSampling();
    sampling.hsamp_.setGeomID( geomid_ );

    BufferStringSet cnames;
    cnames.add( "autocorrelation" );
    const DataPackID outputid = uiAttribPartServer::createDataPackFor2D(
					d2dh, sampling, SI().zDomain(),&cnames);
    auto regsdp = DPM(DataPackMgr::SeisID()).get<RegularSeisDataPack>(outputid);
    if ( !regsdp ) return;

    FlatDataPack*& fdp = isqc ? fddatapackqc_ : fddatapackexam_;
    fdp = new RegularFlatDataPack( *regsdp, 0 );
    DPM(DataPackMgr::FlatID()).add( fdp );
}


void GapDeconACorrView::createFD3DDataPack( bool isqc, EngineMan* aem,
					    Processor* proc )
{
    RefMan<RegularSeisDataPack> output = aem->getDataPackOutput( *proc );
    if ( !output ) return;

    bool csmatchessurv = SI().zRange(0).includes(tkzs_.zsamp_.start, false )
			&& SI().zRange(0).includes(tkzs_.zsamp_.stop, false );
    //if we previously 'faked' a 'normal' cubesampling for the attribute engine
    //we now have to go back to the user specified sampling
    if ( !csmatchessurv ) output->setSampling( tkzs_ );

    FlatDataPack*& fdp = isqc ? fddatapackqc_ : fddatapackexam_;
    fdp = new RegularFlatDataPack( *output, 0 );
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
	fvwin->viewer().setPack( FlatView::Viewer::VD, dp->id(), false );
    else
    {
	fvwin = new uiFlatViewMainWin( 0,
		uiFlatViewMainWin::Setup(isqc?qctitle_:examtitle_,false) );
	uiFlatViewer& vwr = fvwin->viewer();
	vwr.setPack( FlatView::Viewer::VD, dp->id(), true );
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
