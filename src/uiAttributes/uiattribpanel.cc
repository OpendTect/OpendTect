/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattribpanel.h"
#include "attribparam.h"
#include "attribsel.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "attribprocessor.h"
#include "attribprovider.h"
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

#include "hiddenparam.h"
static HiddenParam<uiAttribPanel,RefMan<FlatDataPack>> hp_vddp_(nullptr);



uiAttribPanel::uiAttribPanel( uiParent* p )
    : flatvwin_(nullptr)
    , geomid_(Survey::GM().cUndefGeomID())
    , attribid_( DescID::undef() )
    , dset_(nullptr)
    , parent_(p)
{
    hp_vddp_.setParam( this, nullptr );
}


uiAttribPanel::~uiAttribPanel()
{
    delete dset_;
    delete flatvwin_;
    hp_vddp_.setParam( this, nullptr );
    hp_vddp_.removeParam( this );
}


FlatDataPack* uiAttribPanel::computeAttrib()
{
    RefMan<FlatDataPack> dp = computeAttribute();
    hp_vddp_.setParam( this, dp );
    return dp;
}


RefMan<FlatDataPack> uiAttribPanel::computeAttribute()
{
    if ( !dset_ )
    {
	uiMSG().error( tr("No valid AttributeSet found") );
	return nullptr;
    }

    uiString errmsg;
    RefMan<Attrib::Data2DHolder> d2dh = new Attrib::Data2DHolder();
    PtrMan<EngineMan> aem = createEngineMan();

    const bool is2d = dset_->is2D();
    PtrMan<Processor> proc = is2d ? aem->createScreenOutput2D( errmsg, *d2dh )
				  : aem->createDataPackOutput( errmsg, nullptr);
    if ( !proc )
    {
	uiMSG().error( errmsg );
	return nullptr;
    }

    const bool issingtrc = ( is2d || tkzs_.nrInl()==1 ) && tkzs_.nrCrl()==1;
    if ( issingtrc && proc->getProvider() )
	proc->getProvider()->enableAllOutputs( true );

    proc->setName( getProcName() );
    uiTaskRunner dlg( parent_ );
    if ( !TaskRunner::execute( &dlg, *proc ) )
	return nullptr;

    RefMan<FlatDataPack> fdpack = is2d ? createFDPack( *d2dh )
				       : createFDPack( aem, proc );
    if ( !fdpack )
	return nullptr;

    fdpack->setName( getPackName() );
    return fdpack;
}


EngineMan* uiAttribPanel::createEngineMan()
{
    auto* aem = new EngineMan;

    TypeSet<SelSpec> attribspecs;
    SelSpec sp( nullptr, attribid_ );
    attribspecs += sp;

    aem->setAttribSet( dset_ );
    aem->setAttribSpecs( attribspecs );
    aem->setGeomID( geomid_ );
    aem->setTrcKeyZSampling( tkzs_ );	//should be only 1 trace
    return aem;
}


RefMan<FlatDataPack> uiAttribPanel::createFDPack(const Data2DHolder& d2dh) const
{
    if ( d2dh.dataset_.isEmpty() )
	return nullptr;

    TrcKeyZSampling sampling = d2dh.getTrcKeyZSampling();
    sampling.hsamp_.setGeomID( geomid_ );

    auto rsdp = uiAttribPartServer::createDataPackFor2DRM( d2dh, sampling,
							   SI().zDomain() );
    return rsdp ? new RegularFlatDataPack(*rsdp,-1) : nullptr;
}


RefMan<FlatDataPack> uiAttribPanel::createFDPack( EngineMan* aem,
						  Processor* proc ) const
{
    ConstRefMan<RegularSeisDataPack> output = aem->getDataPackOutput( *proc );
    return output ? new RegularFlatDataPack(*output,-1) : nullptr;
}


void uiAttribPanel::createAndDisplay2DViewer( FlatDataPack* fdpack )
{
    if ( !fdpack )
	return;

    RefMan<FlatDataPack> fdp = hp_vddp_.getParam(this);
    if ( flatvwin_ )
	flatvwin_->viewer().setPack( FlatView::Viewer::VD, fdp );
    else
    {
	flatvwin_ = new uiFlatViewMainWin( parent_,
			uiFlatViewMainWin::Setup(toUiString(getPanelName())));
	uiFlatViewer& vwr = flatvwin_->viewer();
	vwr.setInitialSize( uiSize(400,600) );
	FlatView::Appearance& app = vwr.appearance();
	app.annot_.setAxesAnnot( true );
	app.annot_.x1_.sampling_ = fdpack->posData().range(true);
	app.annot_.x2_.sampling_ = fdpack->posData().range(false);
	app.setDarkBG( false );
	app.setGeoDefaults( true );
	app.ddpars_.show( false, true );
	vwr.setPack( FlatView::Viewer::VD, fdp );
	flatvwin_->addControl( new uiFlatViewStdControl(vwr,
		uiFlatViewStdControl::Setup(nullptr).isvertical(true)) );
	flatvwin_->setDeleteOnClose( false );
	flatvwin_->showAlwaysOnTop();
    }

    flatvwin_->show();
}


void uiAttribPanel::compAndDispAttrib( DescSet* dset, const DescID& mpid,
				       const TrcKeyZSampling& cs,
				       const Pos::GeomID& geomid )
{
    attribid_ = mpid;
    tkzs_ = cs;
    geomid_ = geomid;

    delete dset_;
    dset_ = dset;

    RefMan<FlatDataPack> fdpack = computeAttribute();
    hp_vddp_.setParam( this, fdpack );
    if ( fdpack )
	createAndDisplay2DViewer( fdpack );
    else
	{ pErrMsg("Error during attribute computation"); }
}
