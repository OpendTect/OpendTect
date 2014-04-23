/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Huck
 Date:          July 2010
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uitrcpositiondlg.h"

#include "bufstringset.h"
#include "ioobj.h"
#include "flatposdata.h"
#include "pickretriever.h"
#include "posinfo2d.h"
#include "position.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "posinfo2dsurv.h"

#include "uicombobox.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uiseisioobjinfo.h"
#include "uispinbox.h"
#include "uislider.h"
#include "uitoolbutton.h"
#include "od_helpids.h"

uiFlatDPPosSel::uiFlatDPPosSel( uiParent* p, const DataPack::FullID& dpfid )
    : uiGroup(p)
    , altdimnmflds_( 0 )
    , possldr_( 0 )
    , posvalfld_( 0 )
{
    DataPack* dp = DPM( dpfid.ID(0) ).obtain( dpfid.ID(1) );
    mDynamicCast(FlatDataPack*,fdp_,dp);
    if ( !fdp_ )
    {
	pErrMsg( "Have no flatdatapack, Cannot construct the class" );
	return;
    }

    BufferStringSet altdimnms;
    fdp_->getAltDim0Keys( altdimnms );
    altdimnmflds_ = new uiComboBox( this, altdimnms, "" );
    altdimnmflds_->selectionChanged.notify(
	    mCB(this,uiFlatDPPosSel,sldrPosChangedCB) );
    possldr_ = new uiSlider( this, uiSlider::Setup() );
    possldr_->valueChanged.notify( mCB(this,uiFlatDPPosSel,sldrPosChangedCB) );
    StepInterval<double> posdatarg = fdp_->posData().range( true );
    StepInterval<float> floatrg( mCast(float,posdatarg.start),
				 mCast(float,posdatarg.stop),
				 mCast(float,posdatarg.step) );
    possldr_->setInterval( floatrg );
    possldr_->attach( rightOf, altdimnmflds_ );
    posvalfld_ = new uiGenInput( this, "", DoubleInpSpec() );
    posvalfld_->attach( rightOf, possldr_ );
    sldrPosChangedCB( 0 );
}


uiFlatDPPosSel::~uiFlatDPPosSel()
{
    DPM( DataPackMgr::FlatID() ).release( fdp_ );
}


double uiFlatDPPosSel::getPos() const
{
    if ( !posvalfld_ ) return -1.0;
    StepInterval<double> x1rg = fdp_->posData().range( true );
    const int idx = x1rg.getIndex( possldr_->getValue() );
    const double posval = fdp_->getAltDim0Value( 0, idx );
    return posval;
}

void uiFlatDPPosSel::sldrPosChangedCB( CallBacker* )
{
    if ( !posvalfld_ ) return;
    StepInterval<double> x1rg = fdp_->posData().range( true );
    const int idx = x1rg.getIndex( possldr_->getValue() );
    const double posval =
	fdp_->getAltDim0Value( altdimnmflds_->currentItem(), idx );
    posvalfld_->setValue( posval );
}



uiTrcPositionDlg::uiTrcPositionDlg( uiParent* p, const DataPack::FullID& dpfid )
    : uiDialog( p, uiDialog::Setup("Attribute trace position",0,
                                    mODHelpKey(mTrcPositionDlgHelpID) )
			     .modal(false) )
    , linesfld_( 0 )
    , trcnrfld_( 0 )
    , inlfld_( 0 )
    , crlfld_( 0 )
    , pickretriever_( 0 )
{
    const int dpmid = dpfid.ID( 0 );
    if ( dpmid!=DataPackMgr::FlatID() && dpmid!=DataPackMgr::CubeID() )
    {
	pErrMsg( "Only Flat & Cube DataPacks supported" );
	return;
    }

    DataPack* dp = DPM( dpmid ).obtain( dpfid.ID(1) );
    if ( dpmid == DataPackMgr::FlatID() )
    {
	fdpposfld_ = new uiFlatDPPosSel( this, dpfid );
	mDynamicCastGet(FlatDataPack*,fdp,dp);
	if ( !fdp )
	{
	    pErrMsg( "Could not find Flat DataPack" );
	    DPM( dpmid ).release( dpfid.ID(1) );
	    return;
	}

	StepInterval<double> x2rg = fdp->posData().range( false );
	StepInterval<float> fzrg( mCast(float,x2rg.start),
				  mCast(float,x2rg.stop),
				  mCast(float,x2rg.step) );
	zrg_ = fzrg;
    }
    else if ( dpmid == DataPackMgr::CubeID() )
    {
	mDynamicCastGet(CubeDataPack*,cdp,dp);
	if ( !cdp )
	{
	    pErrMsg( "Could not find Cube DataPack" );
	    DPM( dpmid ).release( dpfid.ID(1) );
	    return;
	}

	CubeSampling cs = cdp->sampling();
	BufferString str = "Compute attribute at position:";
	inlfld_ = new uiLabeledSpinBox( this, str );
	crlfld_ = new uiSpinBox( this );
	crlfld_->attach( rightTo, inlfld_ );
	inlfld_->box()->setInterval( cs.hrg.inlRange() );
	inlfld_->box()->setValue( cs.hrg.inlRange().snappedCenter() );
	crlfld_->setInterval( cs.hrg.crlRange() );
	crlfld_->setValue( cs.hrg.crlRange().snappedCenter() );
	DPM( DataPackMgr::CubeID() ).release( dpfid.ID(1) );
	zrg_ = cs.zrg;
    }

    DPM( dpmid ).release( dpfid.ID(1) );
}


uiTrcPositionDlg::uiTrcPositionDlg( uiParent* p, const CubeSampling& cs,
				    bool is2d, const MultiID& mid )
    : uiDialog( p, uiDialog::Setup("Attribute trace position", 0,
                                   mODHelpKey(mTrcPositionDlgHelpID) )
			     .modal(false) )
    , linesfld_( 0 )
    , trcnrfld_( 0 )
    , inlfld_( 0 )
    , crlfld_( 0 )
    , fdpposfld_( 0 )
    , mid_( mid )
{
    if ( is2d )
    {
	BufferStringSet linenames;
	uiSeisIOObjInfo objinfo( mid );
	objinfo.ioObjInfo().getLineNames( linenames );
	BufferString str = "Compute attribute on line:";
	linesfld_ = new uiLabeledComboBox( this, str );
	for ( int idx=0; idx<linenames.size(); idx++ )
	    linesfld_->box()->addItem( linenames.get(idx) );

	linesfld_->box()->selectionChanged.notify(
					mCB(this,uiTrcPositionDlg,lineSel) );
	trcnrfld_ = new uiLabeledSpinBox( this, "at trace nr:" );
	trcnrfld_->attach( alignedBelow, linesfld_ );
	lineSel(0);
    }
    else
    {
	BufferString str = "Compute attribute at position:";
	inlfld_ = new uiLabeledSpinBox( this, str );
	crlfld_ = new uiSpinBox( this );
	crlfld_->attach( rightTo, inlfld_ );
	inlfld_->box()->setInterval( cs.hrg.inlRange() );
	inlfld_->box()->setValue( cs.hrg.inlRange().snappedCenter() );
	crlfld_->setInterval( cs.hrg.crlRange() );
	crlfld_->setValue( cs.hrg.crlRange().snappedCenter() );
    }

    getposbut_ = new uiToolButton( this, "pick", "Point in 3D scene",
				   mCB(this,uiTrcPositionDlg,getPosCB) );
    if ( trcnrfld_ )
	getposbut_->attach( rightOf, trcnrfld_ );
    else
	getposbut_->attach( rightOf, crlfld_ );

    pickretriever_ = PickRetriever::getInstance();
    pickretriever_->finished()->notify(
				mCB(this,uiTrcPositionDlg,pickRetrievedCB) );
    zrg_.setFrom( cs.zrg );
}


uiTrcPositionDlg::~uiTrcPositionDlg()
{
    if ( pickretriever_ )
	pickretriever_->finished()->remove(
				mCB(this,uiTrcPositionDlg,pickRetrievedCB) );
}


void uiTrcPositionDlg::getPosCB( CallBacker* )
{
    pickretriever_->enable( 0 );
    getposbut_->setSensitive( false );
}


bool uiTrcPositionDlg::getSelLineGeom( PosInfo::Line2DData& l2ddata )
{
    uiSeisIOObjInfo objinfo( mid_ );
    const IOObj* obj = objinfo.ioObj();
    if ( !obj ) return false;

    S2DPOS().setCurLineSet( obj->name() );
    const char* sellnm = linesfld_->box()->text();
    l2ddata.setLineName( sellnm );
    return S2DPOS().getGeometry( l2ddata ) ? true : false;
}


void uiTrcPositionDlg::pickRetrievedCB( CallBacker* )
{
    getposbut_->setSensitive( true );
    const Coord3 crd = pickretriever_->getPos();
    if ( !pickretriever_->success() )
	return;

    if ( trcnrfld_ )
    {
	PosInfo::Line2DData line2d;
	if ( !getSelLineGeom( line2d ) )
	    return;

	PosInfo::Line2DPos l2dpos;
	if ( !line2d.getPos( crd, l2dpos, SI().crlDistance() ) )
	{
	    BufferString msg( "Please pick trace on line:",
			      linesfld_->box()->text() );
	    uiMSG().message( msg );
	    return;
	}

	trcnrfld_->box()->setValue( l2dpos.nr_ );
    }
    else
    {
	const BinID bid = SI().transform( crd );
	inlfld_->box()->setValue( bid.inl() );
	crlfld_->setValue( bid.crl() );
    }
}


LineKey uiTrcPositionDlg::getLineKey() const
{
    LineKey lk;
    if ( !linesfld_ ) return lk;

    lk.setLineName( linesfld_->box()->text() );
    return lk;
}


CubeSampling uiTrcPositionDlg::getCubeSampling() const
{
    CubeSampling cs;
    if ( trcnrfld_ || fdpposfld_ )
    {
	int trcnr = fdpposfld_ ? mCast(int,fdpposfld_->getPos() )
			       : trcnrfld_->box()->getValue();
	cs.hrg.set( cs.hrg.inlRange(), StepInterval<int>( trcnr, trcnr, 1 ) );
    }
    else
    {
	int inlnr = inlfld_->box()->getValue();
	int crlnr = crlfld_->getValue();
	cs.hrg.set( StepInterval<int>( inlnr, inlnr, 1 ),
		    StepInterval<int>( crlnr, crlnr, 1 ) );
    }

    cs.zrg = zrg_;
    return cs;
}


void uiTrcPositionDlg::lineSel( CallBacker* cb )
{
    PosInfo::Line2DData line2d;
    if ( !getSelLineGeom( line2d ) )
	return;

    trcnrfld_->box()->setInterval( line2d.trcNrRange() );
    trcnrfld_->box()->setValue( line2d.trcNrRange().snappedCenter() );
}
