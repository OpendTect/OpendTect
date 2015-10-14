/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Huck
 Date:          Jun 2010
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


uiAttribPanel::uiAttribPanel( uiParent* p )
    : attribid_( DescID::undef() )
    , geomid_(Survey::GM().cUndefGeomID())
    , dset_( 0 )
    , flatvwin_( 0 )
    , parent_( p )
{
}


uiAttribPanel::~uiAttribPanel()
{
    if ( dset_ ) delete dset_;
    if ( flatvwin_ ) delete flatvwin_;
}


FlatDataPack* uiAttribPanel::computeAttrib()
{
    uiString errmsg;
    RefMan<Attrib::Data2DHolder> d2dh = new Attrib::Data2DHolder();
    PtrMan<EngineMan> aem = createEngineMan();

    const bool is2d = dset_->is2D();
    PtrMan<Processor> proc = is2d ? aem->createScreenOutput2D( errmsg, *d2dh )
				  : aem->createDataPackOutput( errmsg, 0  );
    if ( !proc )
    {
	uiMSG().error( errmsg );
	return 0;
    }

    const bool issingtrc = ( is2d || tkzs_.nrInl()==1 ) && tkzs_.nrCrl()==1;
    if ( issingtrc && proc->getProvider() )
	proc->getProvider()->enableAllOutputs( true );

    proc->setName( getProcName() );
    uiTaskRunner dlg( parent_ );
    if ( !TaskRunner::execute( &dlg, *proc ) )
	return 0;

    FlatDataPack* fdpack = is2d ? createFDPack( *d2dh )
				: createFDPack( aem, proc );
    if ( !fdpack ) return 0;
    
    fdpack->setName( getPackName() );
    DPM(DataPackMgr::FlatID()).add( fdpack );
    return fdpack;
}


EngineMan* uiAttribPanel::createEngineMan()
{
    EngineMan* aem = new EngineMan;

    TypeSet<SelSpec> attribspecs;
    SelSpec sp( 0, attribid_ );
    attribspecs += sp;

    aem->setAttribSet( dset_ );
    aem->setAttribSpecs( attribspecs );
    aem->setGeomID( geomid_ );
    aem->setTrcKeyZSampling( tkzs_ );	//should be only 1 trace
    return aem;
}


FlatDataPack* uiAttribPanel::createFDPack( const Data2DHolder& d2dh ) const
{
    if ( d2dh.dataset_.isEmpty() ) return 0;
    TrcKeyZSampling sampling = d2dh.getTrcKeyZSampling();
    sampling.hsamp_.start_.inl() = sampling.hsamp_.stop_.inl() = geomid_;

    const DataPack::ID outputid = uiAttribPartServer::createDataPackFor2D(
						d2dh, sampling, SI().zDomain());
    ConstDataPackRef<RegularSeisDataPack> regsdp =
		DPM(DataPackMgr::SeisID()).obtain( outputid );
    return regsdp ? new RegularFlatDataPack(*regsdp,-1) : 0;
}


FlatDataPack* uiAttribPanel::createFDPack( EngineMan* aem,
						     Processor* proc ) const
{
    const RegularSeisDataPack* output = aem->getDataPackOutput( *proc );
    return output ? new RegularFlatDataPack(*output,-1) : 0;
}


void uiAttribPanel::createAndDisplay2DViewer( FlatDataPack* fdpack )
{
    if ( flatvwin_ )
	flatvwin_->viewer().setPack( false, fdpack->id() );
    else
    {
	flatvwin_ =
	    new uiFlatViewMainWin( 0, uiFlatViewMainWin::Setup(
						   toUiString(getPanelName())));
	uiFlatViewer& vwr = flatvwin_->viewer();
	vwr.setInitialSize( uiSize(400,600) );
	FlatView::Appearance& app = vwr.appearance();
	app.annot_.setAxesAnnot( true );
	app.annot_.x1_.sampling_ = fdpack->posData().range(true);
	app.annot_.x2_.sampling_ = fdpack->posData().range(false);
	app.setDarkBG( false );
	app.setGeoDefaults( true );
	app.ddpars_.show( false, true );
	vwr.setPack( false, fdpack->id() );
	flatvwin_->addControl( new uiFlatViewStdControl(vwr,
			uiFlatViewStdControl::Setup(0).isvertical(true)) );
	flatvwin_->setDeleteOnClose( false );
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
    
    if ( dset_ ) delete dset_;
    dset_ = dset;

    FlatDataPack* fdpack = computeAttrib();
    if ( fdpack )
	createAndDisplay2DViewer( fdpack );
    else
	{ pErrMsg("Error during attribute computation"); }
}
