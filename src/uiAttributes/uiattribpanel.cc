/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "attribdataholder.h"
#include "attribdescid.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "attribprocessor.h"
#include "attribprovider.h"
#include "attribsel.h"
#include "flatposdata.h"
#include "ptrman.h"
#include "seisdatapack.h"
#include "survinfo.h"

#include "uiattribpanel.h"
#include "uiattribpartserv.h"
#include "uiflatviewer.h"
#include "uiflatviewmainwin.h"
#include "uiflatviewstdcontrol.h"
#include "uimsg.h"
#include "uitaskrunner.h"


using namespace  Attrib;

uiAttribPanel::uiAttribPanel( uiParent* p )
    : flatvwin_(nullptr)
    , geomid_(Survey::GM().cUndefGeomID())
    , attribid_( DescID::undef() )
    , dset_(nullptr)
    , parent_(p)
{
}


uiAttribPanel::~uiAttribPanel()
{
    delete dset_;
    delete flatvwin_;
}


RefMan<FlatDataPack> uiAttribPanel::computeAttrib()
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


void uiAttribPanel::createAndDisplay2DViewer()
{
    if ( flatvwin_ )
	flatvwin_->viewer().setPack( FlatView::Viewer::VD, vddp_ );
    else
    {
	flatvwin_ = new uiFlatViewMainWin( parent_,
			uiFlatViewMainWin::Setup(toUiString(getPanelName())));
	uiFlatViewer& vwr = flatvwin_->viewer();
	vwr.setInitialSize( uiSize(400,600) );
	FlatView::Appearance& app = vwr.appearance();
	app.annot_.setAxesAnnot( true );
	app.annot_.x1_.sampling_ = vddp_->posData().range(true);
	app.annot_.x2_.sampling_ = vddp_->posData().range(false);
	app.setDarkBG( false );
	app.setGeoDefaults( true );
	app.ddpars_.show( false, true );
	vwr.setPack( FlatView::Viewer::VD, vddp_ );
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

    vddp_ = computeAttrib();
    if ( vddp_ )
	createAndDisplay2DViewer();
    else
	{ pErrMsg("Error during attribute computation"); }
}
