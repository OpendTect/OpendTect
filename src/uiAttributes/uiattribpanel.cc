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
#include "attribdatacubes.h"
#include "attribdataholder.h"
#include "attribdatapack.h"
#include "arrayndimpl.h"
#include "flatposdata.h"
#include "survinfo.h"
#include "uitaskrunner.h"
#include "uiflatviewer.h"
#include "uiflatviewmainwin.h"
#include "uiflatviewstdcontrol.h"
#include "uimsg.h"

using namespace Attrib;


uiAttribPanel::uiAttribPanel( uiParent* p )
    : attribid_( DescID::undef() )
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
    BufferString errmsg;
    RefMan<Attrib::Data2DHolder> d2dh = new Attrib::Data2DHolder();
    PtrMan<EngineMan> aem = createEngineMan();

    const bool is2d = dset_->is2D();
    PtrMan<Processor> proc = is2d ? aem->createScreenOutput2D( errmsg, *d2dh )
				  : aem->createDataCubesOutput( errmsg, 0  );
    if ( !proc )
    {
	uiMSG().error( errmsg );
	return 0;
    }

    const bool issingtrc = ( is2d || cs_.nrInl()==1 ) && cs_.nrCrl()==1;
    if ( issingtrc && proc->getProvider() )
	proc->getProvider()->enableAllOutputs( true );

    proc->setName( getProcName() );
    uiTaskRunner dlg( parent_ );
    if ( !dlg.execute(*proc) )
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
    aem->setLineKey( lk_ );
    aem->setCubeSampling( cs_ );	//should be only 1 trace
    return aem;
}


FlatDataPack* uiAttribPanel::createFDPack( 
					    const Data2DHolder& d2dh ) const
{
    return new Attrib::Flat2DDHDataPack( attribid_, d2dh, true );
}


FlatDataPack* uiAttribPanel::createFDPack( EngineMan* aem,
						     Processor* proc ) const
{
    const Attrib::DataCubes* output = aem->getDataCubesOutput( *proc );
    if ( !output ) return 0;
    
    output->ref();
    FlatDataPack* fdpack = new Attrib::Flat3DDataPack( attribid_, *output, -1 );
	    
    output->unRef();
    return fdpack;
}


void uiAttribPanel::createAndDisplay2DViewer( FlatDataPack* fdpack )
{
    if ( flatvwin_ )
	flatvwin_->viewer().setPack( false, fdpack->id(), false );
    else
    {
	flatvwin_ =
	    new uiFlatViewMainWin( 0, uiFlatViewMainWin::Setup(getPanelName()));
	uiFlatViewer& vwr = flatvwin_->viewer();
	vwr.setInitialSize( uiSize(400,600) );
	FlatView::Appearance& app = vwr.appearance();
	app.annot_.setAxesAnnot( true );
	app.annot_.x1_.sampling_ = fdpack->posData().range(true);
	app.annot_.x2_.sampling_ = fdpack->posData().range(false);
	app.setDarkBG( false );
	app.setGeoDefaults( true );
	app.ddpars_.show( false, true );
	vwr.setPack( false, fdpack->id(), false );
	flatvwin_->addControl( new uiFlatViewStdControl( flatvwin_->viewer(),
			       uiFlatViewStdControl::Setup(0) ) );
	flatvwin_->setDeleteOnClose( false );
    }
    
    flatvwin_->show();
}


void uiAttribPanel::compAndDispAttrib( DescSet* dset, const DescID& mpid,
				       const CubeSampling& cs,
				       const LineKey& lk )
{
    attribid_ = mpid;
    cs_ = cs;
    lk_ = lk;
    
    if ( dset_ ) delete dset_;
    dset_ = dset;

    FlatDataPack* fdpack = computeAttrib();
    if ( fdpack )
	createAndDisplay2DViewer( fdpack );
    else
	pErrMsg( "Error during attribute computation" );
}
