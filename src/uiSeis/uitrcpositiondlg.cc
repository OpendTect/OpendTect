/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitrcpositiondlg.h"

#include "bufstringset.h"
#include "flatposdata.h"
#include "ioobj.h"
#include "od_helpids.h"
#include "posinfo2d.h"
#include "seisdatapack.h"
#include "survgeom2d.h"
#include "survinfo.h"

#include "uicombobox.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uiseisioobjinfo.h"
#include "uislider.h"
#include "uispinbox.h"
#include "uitoolbutton.h"


// uiFlatDPPosSel

uiFlatDPPosSel::uiFlatDPPosSel( uiParent* p, const DataPack::FullID& dpfid )
    : uiGroup(p)
{
    auto fdp = DPM( dpfid.mgrID() ).get<FlatDataPack>( dpfid.packID() );
    if ( !fdp_ )
    {
	pErrMsg( "Have no flatdatapack, Cannot construct the class" );
	return;
    }

    BufferStringSet altdimnms;
    fdp_->getAltDim0Keys( altdimnms );
    altdimnmflds_ = new uiComboBox( this, altdimnms, "" );
    mAttachCB( altdimnmflds_->selectionChanged,
	       uiFlatDPPosSel::sldrPosChangedCB );
    possldr_ = new uiSlider( this, uiSlider::Setup(), "posslider" );
    mAttachCB( possldr_->valueChanged, uiFlatDPPosSel::sldrPosChangedCB );
    StepInterval<double> posdatarg = fdp_->posData().range( true );
    StepInterval<float> floatrg( mCast(float,posdatarg.start_),
				 mCast(float,posdatarg.stop_),
				 mCast(float,posdatarg.step_) );
    possldr_->setInterval( floatrg );
    possldr_->attach( rightOf, altdimnmflds_ );
    posvalfld_ = new uiGenInput( this, uiString::emptyString(),
                                 DoubleInpSpec() );
    posvalfld_->attach( rightOf, possldr_ );
    mAttachCB( postFinalize(), uiFlatDPPosSel::initGrp );
}


uiFlatDPPosSel::~uiFlatDPPosSel()
{
    detachAllNotifiers();
}


void uiFlatDPPosSel::initGrp( CallBacker* )
{
    sldrPosChangedCB( nullptr );
}


double uiFlatDPPosSel::getPos() const
{
    if ( !posvalfld_ )
	return -1.0;

    StepInterval<double> x1rg = fdp_->posData().range( true );
    const int idx = x1rg.getIndex( possldr_->getFValue() );
    const double posval = fdp_->getAltDim0Value( 0, idx );
    return posval;
}

void uiFlatDPPosSel::sldrPosChangedCB( CallBacker* )
{
    if ( !posvalfld_ )
	return;

    StepInterval<double> x1rg = fdp_->posData().range( true );
    const int idx = x1rg.getIndex( possldr_->getFValue() );
    const double posval =
	fdp_->getAltDim0Value( altdimnmflds_->currentItem(), idx );
    posvalfld_->setValue( posval );
}


// uiTrcPositionDlg

uiTrcPositionDlg::uiTrcPositionDlg( uiParent* p, const TrcKeyZSampling& cs,
				    bool is2d, const MultiID& mid )
    : uiDialog( p, uiDialog::Setup(tr("Attribute trace position"),
				   uiStrings::sEmptyString(),
				   mODHelpKey(mTrcPositionDlgHelpID) )
				   .modal(false) )
    , mid_( mid )
{
    if ( is2d )
    {
	BufferStringSet linenames;
	const uiSeisIOObjInfo objinfo( mid );
	objinfo.ioObjInfo().getLineNames( linenames );
	uiString str = tr("Compute attribute on line:");
	linesfld_ = new uiLabeledComboBox( this, str );
	for ( int idx=0; idx<linenames.size(); idx++ )
	    linesfld_->box()->addItem( toUiString(linenames.get(idx)) );

	mAttachCB( linesfld_->box()->selectionChanged,
		   uiTrcPositionDlg::lineSel );
	trcnrfld_ = new uiLabeledSpinBox( this, tr("at trace nr:") );
	trcnrfld_->attach( alignedBelow, linesfld_ );
    }
    else
    {
	uiString str = tr("Compute attribute at position:");
	inlfld_ = new uiLabeledSpinBox( this, str );
	crlfld_ = new uiSpinBox( this );
	crlfld_->attach( rightTo, inlfld_ );
	inlfld_->box()->setInterval( cs.hsamp_.inlRange() );
	inlfld_->box()->setValue( cs.hsamp_.inlRange().snappedCenter() );
	crlfld_->setInterval( cs.hsamp_.crlRange() );
	crlfld_->setValue( cs.hsamp_.crlRange().snappedCenter() );
    }

    getposbut_ = new uiToolButton( this, "pick", tr("Point in 3D scene"),
				   mCB(this,uiTrcPositionDlg,getPosCB) );
    getposbut_->setToggleButton( true );
    if ( trcnrfld_ )
	getposbut_->attach( rightOf, trcnrfld_ );
    else
	getposbut_->attach( rightOf, crlfld_ );

    pickretriever_ = PickRetriever::instance();
    RefMan<PickRetriever> pickretriever = pickretriever_.get();
    if ( pickretriever && pickretriever->finished() )
	mAttachCB( *pickretriever->finished(),
		   uiTrcPositionDlg::pickRetrievedCB );
    zrg_ = cs.zsamp_;

    mAttachCB( postFinalize(), uiTrcPositionDlg::initDlg );
}


uiTrcPositionDlg::uiTrcPositionDlg( uiParent* p, const DataPack::FullID& dpfid )
    : uiDialog( p, uiDialog::Setup(tr("Attribute trace position"),
				    uiString::empty(),
				    mODHelpKey(mTrcPositionDlgHelpID) )
				    .modal(false) )
{
    const DataPack::MgrID dpmid = dpfid.mgrID();
    if ( dpmid!=DataPackMgr::FlatID() && dpmid!=DataPackMgr::SeisID() )
    {
	pErrMsg( "Only Flat & Cube DataPacks supported" );
	return;
    }

    if ( dpmid == DataPackMgr::FlatID() )
    {
	fdpposfld_ = new uiFlatDPPosSel( this, dpfid );
	auto fdp = DPM( dpmid ).get<FlatDataPack>( dpfid.packID() );
	if ( !fdp )
	{
	    pErrMsg( "Could not find Flat DataPack" );
	    return;
	}

	StepInterval<double> x2rg = fdp->posData().range( false );
	StepInterval<float> fzrg( mCast(float,x2rg.start_),
				  mCast(float,x2rg.stop_),
				  mCast(float,x2rg.step_) );
	zrg_ = fzrg;
    }
    else if ( dpmid == DataPackMgr::SeisID() )
    {
	auto sdp = DPM( dpmid ).get<RegularSeisDataPack>( dpfid.packID() );
	if ( !sdp )
	{
	    pErrMsg( "Could not find Cube DataPack" );
	    return;
	}

	TrcKeyZSampling cs = sdp->sampling();
	uiString str = tr("Compute attribute at position:");
	inlfld_ = new uiLabeledSpinBox( this, str );
	crlfld_ = new uiSpinBox( this );
	crlfld_->attach( rightTo, inlfld_ );
	inlfld_->box()->setInterval( cs.hsamp_.inlRange() );
	inlfld_->box()->setValue( cs.hsamp_.inlRange().snappedCenter() );
	crlfld_->setInterval( cs.hsamp_.crlRange() );
	crlfld_->setValue( cs.hsamp_.crlRange().snappedCenter() );

	zrg_ = cs.zsamp_;
    }

    mAttachCB( postFinalize(), uiTrcPositionDlg::initDlg );
}


uiTrcPositionDlg::~uiTrcPositionDlg()
{
    detachAllNotifiers();
}


void uiTrcPositionDlg::initDlg( CallBacker* )
{
    if ( trcnrfld_ )
	lineSel( nullptr );
}


void uiTrcPositionDlg::getPosCB( CallBacker* )
{
    RefMan<PickRetriever> pickretriever = pickretriever_.get();
    if ( !pickretriever )
	return;

    if ( getposbut_->isOn() )
	pickretriever->enable( nullptr );
    else
	pickretriever->reset();
}


bool uiTrcPositionDlg::getSelLineGeom( PosInfo::Line2DData& l2ddata )
{
    const uiSeisIOObjInfo objinfo( mid_ );
    const IOObj* obj = objinfo.ioObj();
    if ( !obj )
	return false;

    const char* sellnm = linesfld_->box()->text();
    const Survey::Geometry* geom = Survey::GM().getGeometry( sellnm );
    mDynamicCastGet(const Survey::Geometry2D*,geom2d,geom)
    if ( !geom2d )
	return false;

    l2ddata = geom2d->data();
    return true;
}


void uiTrcPositionDlg::pickRetrievedCB( CallBacker* )
{
    if ( !getposbut_->isOn() )
	return;

    getposbut_->setOn( false );
    RefMan<PickRetriever> pickretriever = pickretriever_.get();
    if ( !pickretriever )
	return;

    const Coord3 crd = pickretriever->getPos();
    if ( !pickretriever->success() )
	return;

    if ( trcnrfld_ )
    {
	PosInfo::Line2DData line2d;
	if ( !getSelLineGeom( line2d ) )
	    return;

	PosInfo::Line2DPos l2dpos;
	if ( !line2d.getPos( crd, l2dpos, SI().crlDistance() ) )
	{
	    uiString msg = tr("Please pick trace on line:%1")
			 .arg(linesfld_->box()->text());
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


Pos::GeomID uiTrcPositionDlg::getGeomID() const
{
    if ( fdpposfld_ )
	return Pos::GeomID( OD::GeomSynth );

    if ( !linesfld_ )
	return Pos::GeomID( OD::Geom3D );

    const char* linename = linesfld_->box()->text();
    return Survey::GM().getGeomID( linename );
}


TrcKeyZSampling uiTrcPositionDlg::getTrcKeyZSampling() const
{
    TrcKeyZSampling cs;
    if ( trcnrfld_ || fdpposfld_ )
    {
	const int trcnr = fdpposfld_ ? mCast(int,fdpposfld_->getPos() )
				     : trcnrfld_->box()->getIntValue();
	cs.hsamp_.set( getGeomID(), StepInterval<int>(trcnr,trcnr,1) );
    }
    else
    {
	BinID bid;
	bid.inl() = inlfld_->box()->getIntValue();
	bid.crl() = crlfld_->getIntValue();
	cs.hsamp_.set( TrcKey(bid) );
    }

    cs.zsamp_ = zrg_;
    return cs;
}


void uiTrcPositionDlg::lineSel( CallBacker* )
{
    PosInfo::Line2DData line2d;
    if ( !getSelLineGeom(line2d) )
	return;

    trcnrfld_->box()->setInterval( line2d.trcNrRange() );
    trcnrfld_->box()->setValue( line2d.trcNrRange().snappedCenter() );
}
