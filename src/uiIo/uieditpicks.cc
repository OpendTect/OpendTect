/*+
________________________________________________________________________

Copyright:	(C) 1995-2022 dGB Beheer B.V.
License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uieditpicks.h"

#include "ioman.h"
#include "picklocation.h"
#include "pickset.h"
#include "picksettr.h"
#include "string2.h"

#include "uibuttongroup.h"
#include "uigeninput.h"
#include "uigroup.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uitable.h"
#include "uitextedit.h"
#include "uitoolbutton.h"
#include "uizrangeinput.h"

constexpr int cXCol		= 0;
constexpr int cYCol		= 1;
constexpr int cInlCol		= 2;
constexpr int cCrlCol		= 3;
constexpr int cZCol		= 4;


class PickSetInfo
{
public:
PickSetInfo( const Pick::Set& ps )
    : ps_(ps)
{
    ps_.getBoundingBox( const_cast<TrcKeyZSampling&>(pstkzs_) );
    zrg_ = pstkzs_.zsamp_;
    inlrg_ = pstkzs_.hsamp_.lineRange();
    crlrg_ = pstkzs_.hsamp_.trcRange();
    const Coord start = pstkzs_.hsamp_.toCoord( pstkzs_.hsamp_.start_ );
    const Coord stop = pstkzs_.hsamp_.toCoord( pstkzs_.hsamp_.stop_ );
    if ( !start.isUdf() && !stop.isUdf() )
    {
	xrg_.start_ = start.x_;
	xrg_.stop_ = stop.x_;
	yrg_.start_ = start.y_;
	yrg_.stop_ = stop.y_;
    }

    if ( !ps_.isPolygon() )
    {
	const TypeSet<Pick::Location>& locs = ps_.locations();
	for ( const auto& loc : locs )
	{
	    if ( !loc.hasDir() )
		continue;

	    const Sphere& dir = loc.dir();
	    if ( !mIsUdf(dir.radius) )
		radvalrg_.include( dir.radius );
	    if ( !mIsUdf(dir.theta) )
		dipvalrg_.include( dir.theta );
	    if ( !mIsUdf(dir.phi) )
		azimvalrg_.include( dir.phi );
	}
    }
}

~PickSetInfo()
{}

#define mRgToString( rg ) \
    BufferString rg##str; \
    if ( !rg##_.isUdf() ) \
	rg##str.add("[").add(rg##_.start_).add(",").add(rg##_.stop_).add("]");

void toText( BufferString& text ) const
{
    text.add( sKey::Name() ).add( " : " ).add( ps_.name() ).addNewLine()
	.add( sKey::Size() ).add( " : " ).add( ps_.size() ).addNewLine();

    mRgToString( xrg )
    mRgToString( yrg )
    text.add( sKey::XRange() ).add( " : " ).add( xrgstr ).addNewLine()
	.add( sKey::YRange() ).add( " : " ).add( yrgstr ).addNewLine();

    if ( ps_.is2D() )
    {
	mRgToString( crlrg )
	text.add( sKey::TrcRange() ).add( " : " ).add( crlrgstr ).addNewLine();
    }
    else
    {
	mRgToString( inlrg )
	mRgToString( crlrg )
	text.add( sKey::InlRange() ).add( " : " ).add( inlrgstr ).addNewLine()
	    .add( sKey::CrlRange() ).add( " : " ).add( crlrgstr ).addNewLine();
    }

    mRgToString( zrg )
    text.add( sKey::ZRange() ).add( " : " ).add( zrgstr ).addNewLine();
    if ( !ps_.isPolygon() )
    {
	mRgToString( radvalrg )
	text.add( "Length range: " ).add( radvalrgstr ).addNewLine();
	mRgToString( dipvalrg )
	text.add( "Dip range: " ).add( radvalrgstr ).addNewLine();
	mRgToString( azimvalrg )
	text.add( "Azimuth range: " ).add( radvalrgstr ).addNewLine();
    }
}

const Interval<float>&	zRange() const		{ return zrg_; }
const Interval<double>& xRange() const		{ return xrg_; }
const Interval<double>& yRange() const		{ return yrg_; }
const Interval<int>&	inlRange() const	{ return inlrg_; }
const Interval<int>&	crlRange() const	{ return crlrg_; }
const Interval<float>&	radValRange() const	{ return radvalrg_; }
const Interval<float>&	dipValRange() const	{ return dipvalrg_; }
const Interval<float>&	azimValRange() const	{ return azimvalrg_; }

bool validRadiusVals() const
{
    return !radvalrg_.isUdf() && radvalrg_.width()>0;
}

bool validDipVals() const
{
    return !dipvalrg_.isUdf() && dipvalrg_.width()>0;
}

bool validAzimuthVals() const
{
    return !azimvalrg_.isUdf() && azimvalrg_.width()>0;
}

private:

    const Pick::Set&		ps_;
    const TrcKeyZSampling	pstkzs_;

    Interval<float>		zrg_		= Interval<float>::udf();
    Interval<double>		xrg_		= Interval<double>::udf();
    Interval<double>		yrg_		= Interval<double>::udf();
    Interval<int>		inlrg_		= Interval<int>::udf();
    Interval<int>		crlrg_		= Interval<int>::udf();

    Interval<float>		radvalrg_	= Interval<float>::udf();
    Interval<float>		dipvalrg_	= Interval<float>::udf();
    Interval<float>		azimvalrg_	= Interval<float>::udf();

};


uiEditPolygonGroup::uiEditPolygonGroup( uiParent* p, Pick::Set& ps )
    : uiGroup( p, "Edit polygon group" )
    , ps_(ps)
{
    if ( !ps.isPolygon() )
    {
	pErrMsg( "This group is for editing a polygon." );
	return;
    }

    createTable();
    fillTable();
    mAttachCB( tbl_->valueChanged, uiEditPolygonGroup::valChgCB );
    tblbutgrp_ = new uiButtonGroup( this, "Table button group",
				    OD::Vertical );
    tblbutgrp_->attach( rightOf, tbl_ );
    uiToolButton::getStd( tblbutgrp_, OD::Remove,
			  mCB(this,uiEditPolygonGroup,removeLocCB),
			  tr("Remove selected locations") );

    mAttachCB( Pick::Mgr().locationChanged,
	       uiEditPolygonGroup::pickLocationChangedCB );
    mAttachCB( Pick::Mgr().bulkLocationChanged,
	       uiEditPolygonGroup::pickLocationChangedCB );
}


uiEditPolygonGroup::~uiEditPolygonGroup()
{}


uiButtonGroup* uiEditPolygonGroup::tblButGrp()
{
    return tblbutgrp_;
}


void uiEditPolygonGroup::createTable()
{
    tbl_ = new uiTable( this, uiTable::Setup().rowdesc("Location")
					      .rowgrow(true).defrowlbl("")
					      .selmode(uiTable::Multi),
			"Pick Location Table" );
    uiStringSet lbls;
    const uiString xyunstr = SI().getUiXYUnitString();
    const uiString zunstr = ps_.zDomain().uiUnitStr(true);
    lbls.add( ::toUiString("%1 %2").arg(uiStrings::sX()).arg(xyunstr) )
	.add( ::toUiString("%1 %2").arg(uiStrings::sY()).arg(xyunstr) )
	.add( uiStrings::sInline() ).add( uiStrings::sCrossline() )
	.add( ::toUiString("%1 %2").arg(uiStrings::sZ()).arg(zunstr) );

    tbl_->setColumnLabels( lbls );
    tbl_->setColumnResizeMode( uiTable::ResizeToContents );
    tbl_->setMinimumWidth(500);
}


void uiEditPolygonGroup::fillTable()
{
    NotifyStopper ns( tbl_->valueChanged );
    const int nrrows = ps_.size();
    tbl_->setNrRows( nrrows );
    for ( int row=0; row<nrrows; row++ )
	fillRow( row );
}


void uiEditPolygonGroup::fillRow( int row )
{
    if ( row>ps_.size() || row>tbl_->nrRows() )
	return;

    const Pick::Location& loc = ps_.get( row );
    const double x = loc.pos().x_;
    if ( !mIsUdf(x) )
	tbl_->setText( RowCol(row,cXCol), ::toUiString(x,0,'f',2) );

    const double y = loc.pos().y_;
    if ( !mIsUdf(y) )
	tbl_->setText( RowCol(row,cYCol), ::toUiString(y,0,'f',2) );

    const TrcKey& tk = loc.trcKey();
    if ( !tk.isUdf() )
    {
	tbl_->setText( RowCol(row,cInlCol), ::toUiString(tk.inl()) );
	tbl_->setText( RowCol(row,cCrlCol), ::toUiString(tk.crl()) );
    }

    const float z = loc.z()*ps_.zDomain().userFactor();
    if ( !mIsUdf(z) )
	tbl_->setText( RowCol(row,cZCol), ::toUiString(z,0,'f',2) );

}


void uiEditPolygonGroup::valChgCB( CallBacker* )
{
    const RowCol& rc = tbl_->notifiedCell();
    if ( rc.row() >= ps_.size() )
    {
	pErrMsg( "nrrows should be equal to size of Pick::Set" );
	return;
    }

    const float newval = tbl_->getFValue( rc );
    if ( mIsUdf(newval) )
    {
	uiMSG().error( tr("Please enter a valid value.") );
	return;
    }

    const int col = rc.col();
    if ( col == cZCol )
	ps_.setZ( col, newval );
    else if ( col==cXCol || col==cYCol )
    {
	const Coord& currcoord = ps_.getPos( rc.row() );
	const Coord newcoord( col==cXCol ? newval : currcoord.x_,
			      col==cYCol ? newval : currcoord.y_ );
	ps_.setPos( rc.row(), newcoord );
    }
    else if ( col==cInlCol || col==cCrlCol )
    {
	const int newposval = tbl_->getIntValue( rc );
	const Pick::Location& currloc = ps_.get( rc.row() );
	const BinID& currbid = currloc.binID();
	const BinID newbid( col==cInlCol ? newposval : currbid.inl(),
			    col==cCrlCol ? newposval : currbid.crl() );
	const TrcKey newtk( newbid );
	ps_.setPos( rc.row(), newtk.getCoord() );
    }
    else
	return;

    NotifyStopper ns( tbl_->valueChanged );
    NotifyStopper pslocns( Pick::Mgr().locationChanged, this );
    fillRow( rc.row() );
    Pick::Mgr().reportChange( nullptr, ps_ );
}


void uiEditPolygonGroup::removeLocCB( CallBacker* )
{
    TypeSet<int> selrows;
    tbl_->getSelectedRows( selrows );
    if ( selrows.isEmpty() )
	return;

    TypeSet<Pick::Location> locs;
    for ( const auto& row: selrows )
	locs += ps_.get( row ) ;

    ps_.bulkRemoveWithUndo( locs, selrows );
    tbl_->clearTable();
    fillTable();
    NotifyStopper pslocns( Pick::Mgr().locationChanged, this );
    Pick::Mgr().reportChange( nullptr, ps_ );
}


void uiEditPolygonGroup::pickLocationChangedCB( CallBacker* )
{
    tbl_->clearTable();
    fillTable();
}


uiFilterPicksGrp::uiFilterPicksGrp( uiParent* p, Pick::Set& ps )
    : uiGroup( p, "Filter picks group" )
    , ps_(ps)
    , psinfo_(*new PickSetInfo(ps))
{
    auto* leftgrp = new uiGroup( this, "Left group" );
    usexycb_ = new uiGenInput( leftgrp, tr("Use"),
			       BoolInpSpec(true,tr("X/Y"),tr("Inl/Crl")) );
    mAttachCB( usexycb_->valueChanged, uiFilterPicksGrp::useXYCB );
    keepdiscardfld_ = new uiGenInput( leftgrp, tr("Keep picks"),
			       BoolInpSpec(true,tr("Inside specified ranges"),
					   tr("Outside specified ranges")) );
    keepdiscardfld_->attach( alignedBelow, usexycb_ );

    auto* sep = new uiSeparator( this, "Vertical separator", OD::Vertical );
    sep->attach( stretchedRightTo, leftgrp );

    auto* rightgrp = new uiGroup( this, "Right group" );
    rightgrp->attach( ensureRightOf, sep );
    crdgrp_ = new uiGroup( rightgrp, "Cordinate range group" );
    xintvfld_ = new uiGenInput( crdgrp_, uiStrings::sXRange(),
				DoubleInpIntervalSpec(psinfo_.xRange()) );
    crdgrp_->setHAlignObj( xintvfld_ );
    yintvfld_ = new uiGenInput( crdgrp_, uiStrings::sYRange(),
				DoubleInpIntervalSpec(psinfo_.yRange()) );
    yintvfld_->attach( alignedBelow, xintvfld_ );

    inlcrlgrp_ = new uiGroup( rightgrp, "Inl/Crl group" );
    inlintvfld_ = new uiGenInput( inlcrlgrp_, uiStrings::sInlineRange(),
				IntInpIntervalSpec(psinfo_.inlRange()) );
    inlcrlgrp_->setHAlignObj( inlintvfld_ );
    crlintvfld_ = new uiGenInput( inlcrlgrp_, uiStrings::sCrosslineRange(),
				IntInpIntervalSpec(psinfo_.crlRange()) );
    crlintvfld_->attach( alignedBelow, inlintvfld_ );
    inlcrlgrp_->display( false );

    zintvfld_ = new uiZRangeInput( rightgrp, ps_.zDomain().isDepth(), false );
    zintvfld_->setTitleText( uiStrings::sZRange() );
    zintvfld_->setZRange( psinfo_.zRange() );
    zintvfld_->attach( alignedBelow, crdgrp_ );
    zintvfld_->attach( alignedBelow, inlcrlgrp_ );
    uiObject* attachobj = zintvfld_->attachObj();
    if ( !ps.isPolygon() )
    {
	if ( psinfo_.validRadiusVals() )
	{
	    radiusfld_ = new uiGenInput( rightgrp, tr("Radius value range"),
				 FloatInpIntervalSpec(psinfo_.radValRange()) );
	    radiusfld_->attach( alignedBelow, zintvfld_ );
	    attachobj = radiusfld_->attachObj();
	}

	if ( psinfo_.validDipVals() )
	{
	    dipfld_ = new uiGenInput( rightgrp, tr("Dip value range"),
				 FloatInpIntervalSpec(psinfo_.dipValRange()) );
	    dipfld_->attach( alignedBelow, attachobj );
	    attachobj = dipfld_->attachObj();
	}

	if ( psinfo_.validAzimuthVals() )
	{
	    azimfld_ = new uiGenInput( rightgrp, tr("Azimuth value range"),
				 FloatInpIntervalSpec(psinfo_.azimValRange()) );
	    azimfld_->attach( alignedBelow, attachobj );
	}
    }

    mAttachCB( postFinalize(), uiFilterPicksGrp::finalizeCB );
}


uiFilterPicksGrp::~uiFilterPicksGrp()
{}


void uiFilterPicksGrp::finalizeCB( CallBacker* )
{
    xintvfld_->setNrDecimals( 2, 0 );
    xintvfld_->setNrDecimals( 2, 1 );
    yintvfld_->setNrDecimals( 2, 0 );
    yintvfld_->setNrDecimals( 2, 1 );
}


void uiFilterPicksGrp::useXYCB( CallBacker* )
{
    const bool usexy = usexycb_->getBoolValue();
    crdgrp_->display( usexy );
    inlcrlgrp_->display( !usexy );
}


void uiFilterPicksGrp::getInfo( BufferString& text ) const
{
    psinfo_.toText( text );
}


bool uiFilterPicksGrp::applyFilter()
{
    const bool keepinside = keepdiscardfld_->getBoolValue();
    const bool ret = applyLocFilter(keepinside) &&
		     ( !ps_.isPolygon() && applySphericalFilter(keepinside) );
    return ret;
}


bool uiFilterPicksGrp::applyLocFilter( bool keepinside )
{
    const bool usexy = usexycb_->getBoolValue();
    Interval<double> xintv = usexy ? xintvfld_->getDInterval()
				   : Interval<double>::udf();
    Interval<double> yintv = usexy ? yintvfld_->getDInterval()
				   : Interval<double>::udf();
    if ( !usexy )
    {
	Interval<int> inlrg = inlintvfld_->getIInterval();
	if ( inlrg.isUdf() )
	{
	    uiMSG().error( tr("Please provide a valid Inline Range") );
	    return false;
	}

	Interval<int> crlrg = crlintvfld_->getIInterval();
	if ( crlrg.isUdf() )
	{
	    uiMSG().error( tr("Please provide a valid Crossline Range") );
	    return false;
	}

	inlrg.start_--; inlrg.stop_++;
	crlrg.start_--; crlrg.stop_++;
	const TrcKey starttk( BinID(inlrg.start_,crlrg.start_) );
	const Coord startcrd = starttk.getCoord();
	const TrcKey stoptk( BinID(inlrg.stop_,crlrg.stop_) );
	const Coord stopcrd = stoptk.getCoord();
	xintv.include( startcrd.x_ );
	xintv.include( stopcrd.x_ );
	yintv.include( startcrd.y_ );
	yintv.include( stopcrd.y_ );
    }

    if ( xintv.isUdf() || yintv.isUdf() )
    {
	uiMSG().error( tr("Please provide a valid co-ordinate range") );
	return false;
    }

    const Interval<double> zintv = zintvfld_->getDZRange();
    if ( zintv.isUdf() )
    {
	uiMSG().error( tr("Please provide a valid Z-Range") );
	return false;
    }

    TypeSet<Pick::Location> locs;
    TypeSet<int> idxs;
    for ( int idx=0; idx<ps_.size(); idx++ )
    {
	const Pick::Location& loc = ps_.get( idx );
	const Coord3& locpos = loc.pos();
	const bool isinxintv = xintv.includes( locpos.x_, false );
	const bool isinyintv = yintv.includes( locpos.y_, false );
	const bool isinzintv = zintv.includes( locpos.z_, false );
	const bool isinintvs = isinxintv && isinyintv && isinzintv;
	const bool dokeep = keepinside ? isinintvs : !isinintvs;
	if ( !dokeep )
	{
	    idxs += idx;
	    locs += loc;
	}
    }

    if ( !idxs.isEmpty() )
	ps_.bulkRemoveWithUndo( locs, idxs );

    return true;
}


bool uiFilterPicksGrp::applySphericalFilter( bool keepinside )
{
    const Interval<float> radrg = radiusfld_ ? radiusfld_->getFInterval()
					     : Interval<float>::udf();
    const Interval<float> diprg = dipfld_ ? dipfld_->getFInterval()
					  : Interval<float>::udf();
    const Interval<float> azimrg = azimfld_ ? azimfld_->getFInterval()
					    : Interval<float>::udf();
    TypeSet<Pick::Location> locs;
    TypeSet<int> idxs;
    for ( int idx=0; idx<ps_.size(); idx++ )
    {
	const Pick::Location& loc = ps_.get( idx );
	const Sphere& locdir = loc.dir();
	if ( locdir.isNull() )
	    continue;

	if ( !radrg.isUdf() &&
	     (keepinside && radrg.includes(locdir.radius,false)) )
	{
	    idxs += idx;
	    locs += loc;
	    continue;
	}

	if ( !diprg.isUdf() &&
	     (keepinside && diprg.includes(locdir.theta,false)) )
	{
	    idxs += idx;
	    locs += loc;
	    continue;
	}

	if ( !azimrg.isUdf() &&
	     (keepinside && azimrg.includes(locdir.phi,false)) )
	{
	    idxs += idx;
	    locs += loc;
	    continue;
	}
    }

    if ( !idxs.isEmpty() )
	ps_.bulkRemoveWithUndo( locs, idxs );

    return true;
}

mImplFactory2Param(uiEditPicksDlg,uiParent*,Pick::Set&,uiEditPicksDlg::factory);

uiEditPicksDlg::uiEditPicksDlg( uiParent* p, Pick::Set& ps )
    : uiDialog( p, uiDialog::Setup(tr("Edit pick locations: %1").arg(ps.name()),
				   mTODOHelpKey)
				   .applybutton(!ps.isPolygon()) )
    , ps_(ps)
{
    setOkText( uiStrings::sSave() );
    const bool ispolygon = ps_.isPolygon();
    uiGroup* topgrp = new uiGroup( this, "Top group" );
    if ( ispolygon )
	polygongrp_ = new uiEditPolygonGroup( topgrp, ps );
    else
    {
	infofld_ = new uiTextEdit( topgrp, "Point set information", true );
	infofld_->setPrefHeightInChar( 10 );
    }

    filtergrp_ = new uiFilterPicksGrp( this, ps );
    if ( infofld_ )
    {
	BufferString infotxt;
	filtergrp_->getInfo( infotxt );
	infofld_->setText( infotxt );
    }

    filtergrp_->attach( centeredBelow, topgrp );
    mAttachCB( applyPushed, uiEditPicksDlg::applyCB );
}


uiEditPicksDlg::~uiEditPicksDlg()
{}


void uiEditPicksDlg::initClass()
{
    uiEditPicksDlg::factory().addCreator( create, sKey::PickSet() );
}


bool uiEditPicksDlg::acceptOK( CallBacker* )
{
    const MultiID id = Pick::Mgr().get( ps_ );
    PtrMan<IOObj> obj = IOM().get( id );
    if ( !obj )
    {
	uiMSG().errorWithDetails(
		uiStrings::phrCannotCreateDBEntryFor( uiStrings::sPickSet() ),
		tr("Could not save the changes") );
	return false;
    }

    uiString errmsg;
    const bool ret = PickSetTranslator::store( ps_, obj.ptr(), errmsg);
    if ( !ret )
	uiMSG().errorWithDetails( errmsg, tr("Could not save the changes") );

    return ret;
}


void uiEditPicksDlg::applyCB( CallBacker* )
{
    if ( ps_.isPolygon() )
	return;

    const bool ret = filtergrp_->applyFilter();
    if ( ret )
	Pick::Mgr().reportChange( nullptr, ps_ );
    else
	uiMSG().error( tr("Could not apply your changes to the %1")
			  .arg(sKey::PickSet()) );
}
