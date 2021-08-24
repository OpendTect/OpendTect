/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2008
________________________________________________________________________

-*/

#include "uidirectionalplot.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicscoltab.h"
#include "uifont.h"
#include "uistrings.h"

#include "angles.h"
#include "coltabsequence.h"
#include "coltabmapper.h"
#include "dataclipper.h"
#include "mouseevent.h"


#define mDefMarkerZValue 2
#define mHLMarkerZValue 3


static uiPoint uiPointFromPolar( const uiPoint& c, float r, float angrad )
{
    Geom::Point2D<float> fpt( c.x + r * cos(angrad), c.y - r * sin(angrad) );
    return uiPoint( mNINT32(fpt.x), mNINT32(fpt.y) );
}


uiDirectionalPlot::uiDirectionalPlot( uiParent* p,
				      const uiDirectionalPlot::Setup& su )
    : uiGraphicsView(p,"Function display viewer")
    , setup_(su)
    , selsector_(-1)
    , cursector_(-1)
    , outercircleitm_(0)
    , selsectoritem_(0)
    , sectorlines_(*scene().addItemGrp(new uiGraphicsItemGroup(true)))
    , curveitems_(*scene().addItemGrp(new uiGraphicsItemGroup(true)))
    , markeritems_(*scene().addItemGrp(new uiGraphicsItemGroup(true)))
    , hdrannotitm1_(0)
    , hdrannotitm2_(0)
    , scalelineitm_(0)
    , scalearcitm_(0)
    , scalestartptitem_(0)
    , scaleannotitm_(0)
    , scalestartitm_(0)
    , scalestopitm_(0)
    , coltabitm_(0)
    , sectorPicked(this)
    , colseq_(0)
    , isempty_(true)
{
    disableScrollZoom();
    setPrefWidth( setup_.prefsize_.width() );
    setPrefHeight( setup_.prefsize_.height() );
    setSceneBorder( 20 );
    setStretch( 2, 2 );

    getMouseEventHandler().buttonReleased.notify(
			mCB(this,uiDirectionalPlot,mouseRelease) );

    reSize.notify( mCB(this,uiDirectionalPlot,reSized) );
    setScrollBarPolicy( true, uiGraphicsView::ScrollBarAlwaysOff );
    setScrollBarPolicy( false, uiGraphicsView::ScrollBarAlwaysOff );
    draw();
}


uiDirectionalPlot::~uiDirectionalPlot()
{
    delete outercircleitm_;
    delete selsectoritem_;
    delete &sectorlines_;
    delete &curveitems_;
    delete &markeritems_;
    delete hdrannotitm1_;
    delete hdrannotitm2_;
    delete scalelineitm_;
    delete scalearcitm_;
    delete scalestartptitem_;
    delete scaleannotitm_;
    delete scalestartitm_;
    delete scalestopitm_;
    delete coltabitm_;
}


void uiDirectionalPlot::reSized( CallBacker* )
{
    draw();
}


void uiDirectionalPlot::setData( const float* vals, int sz )
{
    data_.erase();
    highlightidxs_.erase();
    for ( int idx=0; idx<sz; idx++ )
    {
	Stats::SectorData* sd = new Stats::SectorData;
	*sd += Stats::SectorPartData( vals[idx] );
	data_ += sd;
    }

    cursector_ = selsector_ = -1;
    gatherInfo(); draw();
}


void uiDirectionalPlot::setData( const Stats::DirectionalData& dird )
{
    data_ = dird;
    highlightidxs_.erase();

    cursector_ = selsector_ = -1;
    gatherInfo(); draw();
}


void uiDirectionalPlot::gatherInfo()
{
    isempty_ = true;
    TypeSet<float> vals;
    for ( int isect=0; isect<data_.size(); isect++ )
    {
	const int nrparts = data_.nrParts(isect);
	if ( nrparts > 0 )
	{
	    if ( isempty_ )
	    {
		isempty_ = false;
		const Stats::SectorPartData& spd = data_.getPartData(isect,0);
		posrg_.start = posrg_.stop = spd.pos_;
		maxcount_ = spd.count_;
	    }
	    const Stats::SectorData& sd = *data_[isect];
	    int curcount = 0;
	    for ( int ipart=0; ipart<sd.size(); ipart++ )
	    {
		const Stats::SectorPartData& spd = sd[ipart];
		if ( spd.count_ )
		{
		    vals += spd.val_;
		    posrg_.include( spd.pos_ );
		    curcount += spd.count_;
		}
	    }
	    if ( curcount > maxcount_ ) maxcount_ = curcount;
	}
    }

    DataClipSampler dcs( vals.size() );
    dcs.add( vals.arr(), vals.size() );
    valrg_ = dcs.getRange( setup_.clipratio_ );
}


void uiDirectionalPlot::draw()
{
    if ( isempty_ )
	return;

    center_ = uiPoint( viewWidth()/2, viewHeight()/2 );
    const uiSize uitotsz( viewWidth(), viewHeight() );
    const uiBorder border( 30 );
    const uiRect avrect( border.getRect(uitotsz) );
    radius_ = (avrect.width() > avrect.height()
		     ? avrect.height() : avrect.width()) / 2;

    drawData();
    drawGrid();
    drawAnnot();
    drawSelection();
}


void uiDirectionalPlot::setNrEquicircles( int nrcircles )
{
    nrequicircles_ = nrcircles;
}


void uiDirectionalPlot::drawGrid()
{
    const float dr = 1.f / (nrequicircles_ +1);

    if ( outercircleitm_ )
    {
	outercircleitm_->setPos( center_ );
	outercircleitm_->setRadius( radius_ );
	for ( int idx=0; idx< nrequicircles_; idx++ )
	{
	    const float rad = (dr + dr*idx)*radius_ ;
	    uiCircleItem& ci = *equicircles_[idx];
	    ci.setPos( center_ ); ci.setRadius( mNINT32(rad) );
	}
    }
    else
    {
	outercircleitm_ = scene().addItem( new uiCircleItem(center_,radius_) );
	outercircleitm_->setPenStyle( setup_.circlels_ );
	outercircleitm_->setZValue( 0 );
	for ( int idx=0; idx< nrequicircles_; idx++ )
	{
	    const float rad = (dr + dr*idx)*radius_ ;
	    uiCircleItem* ci = scene().addItem(
				new uiCircleItem(center_,mNINT32(rad)) );
	    ci->setZValue( 0 );
	    equicircles_ += ci;
	    ci->setPenStyle( setup_.equils_ );
	}
    }

    sectorlines_.removeAll( true );
    if ( setup_.type_ == Setup::Scatter )
	return; // TODO: Draw grid only

    const int nrsectors = data_.nrSectors();
    for ( int isect=0; isect<nrsectors; isect++ )
    {
	const float ang = data_.angle( isect, 1 );
	const float mathang = Angle::convert( data_.setup_.angletype_, ang,
					      Angle::Rad );
	uiLineItem* li = new uiLineItem( center_, mathang, (float) radius_ );
	sectorlines_.add( li );
	li->setPenStyle( setup_.sectorls_ );
	li->setZValue( 0 );
    }
}


void uiDirectionalPlot::drawScale()
{
    const float sqrt2 = M_SQRT2f;
    const uiPoint startpt( usrUIPos(radius_*1.1f,135) );
    const uiPoint endpt( usrUIPos(radius_*(sqrt2-0.1f),135) );
    if ( !scalelineitm_ )
    {
	scalelineitm_ = scene().addItem( new uiLineItem(startpt,endpt) );
	scalestartptitem_ = scene().addItem(
	    new uiMarkerItem(startpt,MarkerStyle2D(MarkerStyle2D::Circle,3)) );
    }
    else
    {
	scalelineitm_->setLine( startpt, endpt );
	scalestartptitem_->setPos( startpt );
    }

    const bool isvisible = scalearcitm_ ? scalearcitm_->isVisible() : true;
    delete scalearcitm_; scalearcitm_ = 0;
    const Interval<float> angs( Angle::usrdeg2rad(120.F),
				Angle::usrdeg2rad(150.F) );
    const float r = (float)startpt.distTo( endpt );
    scalearcitm_ = scene().addItem(
		new uiCurvedItem(uiPointFromPolar(startpt,r,angs.start)) );
    scalearcitm_->drawTo( uiCurvedItem::ArcSpec(startpt,r,angs) );
    scalearcitm_->setVisible( isvisible );

    const char* nm = setup_.nameforpos_;
    if ( !*nm ) nm = "Values";
    uiPoint annotpt;
    annotpt.x = (startpt.x + endpt.x) / 2;
    annotpt.y = endpt.y + 20;
    uiPoint txtstartpt = startpt; txtstartpt.x += 6; txtstartpt.y -= 3;
    uiPoint txtendpt = endpt; txtendpt.x += 6; txtendpt.y -= 3;
    if ( !scaleannotitm_ )
    {
	Alignment al( mAlignment(HCenter,VCenter) );
	scaleannotitm_ = scene().addItem( new uiTextItem(annotpt,
						     toUiString(nm),al) );
	al.set( Alignment::Left );
	scalestartitm_ = scene().addItem( new uiTextItem(txtstartpt,
						uiStrings::sEmptyString(),al) );
	scalestopitm_ = scene().addItem( new uiTextItem(txtendpt,
						uiStrings::sEmptyString(),al) );
    }
    scalestartitm_->setPos( txtstartpt );
    scaleannotitm_->setPos( annotpt );
    scalestopitm_->setPos( txtendpt );
    scalestartitm_->setText( toUiString(data_.setup_.usrposrg_.start) );
    scalestopitm_->setText( toUiString(data_.setup_.usrposrg_.stop) );
}


void uiDirectionalPlot::drawHeader()
{
    Alignment al( Alignment::Left, Alignment::Top );
    if ( setup_.nameforval_.isEmpty() )
	{ delete hdrannotitm1_; hdrannotitm1_ = 0; }
    else if ( !hdrannotitm1_ )
    {
	hdrannotitm1_ = scene().addItem(
			new uiTextItem(mToUiStringTodo(setup_.nameforval_),al));
	hdrannotitm1_->setPos( uiPoint(2,0) );
    }

    if ( setup_.hdrannot_.isEmpty() )
	{ delete hdrannotitm2_; hdrannotitm2_ = 0; }
    else if ( !hdrannotitm2_ )
    {
	al.set( Alignment::Right );
	hdrannotitm2_ = scene().addItem(
			new uiTextItem(mToUiStringTodo(setup_.hdrannot_),al) );
    }

    if ( hdrannotitm2_ )
	hdrannotitm2_->setPos( uiPoint(viewWidth()-1,0) );
}


void uiDirectionalPlot::drawColTab()
{
    if ( !coltabitm_ )
    {
	uiColTabItem::Setup su( false );
	coltabitm_ = scene().addItem( new uiColTabItem(su) );
    }

    ColTab::MapperSetup ctms;
    ctms.type( ColTab::MapperSetup::Fixed );
    ctms.range( valrg_ );
    coltabitm_->setColTabMapperSetup( ctms );

    if ( colseq_ )
	coltabitm_->setColTabSequence( *colseq_ );

    const uiRect br( coltabitm_->boundingRect() );
    uiPoint targettl( 20, viewHeight() - br.height() );
    coltabitm_->setPos( targettl );
}


void uiDirectionalPlot::drawAnnot()
{
    drawDirAnnot();
    drawHeader();
    drawScale();
    if ( setup_.type_ == Setup::Vals )
	drawColTab();
}


void uiDirectionalPlot::drawDirAnnot()
{
    if ( dirtxtitms_.isEmpty() )
    {
	const uiPoint pt00( 0, 0 );
	for ( int idx=0; idx<4; idx++ )
	{
	    const bool isew = idx % 2;
	    const uiString txt = idx == 0 ? uiStrings::sNorth(true)
			      : (idx == 1 ? uiStrings::sEast(true)
			      : (idx == 2 ? uiStrings::sSouth(true)
					  : uiStrings::sWest(true)));
	    Alignment al( isew ? (idx==1 ? Alignment::Left : Alignment::Right)
					  : Alignment::HCenter,
			  isew ? Alignment::VCenter
			  : (idx == 2 ? Alignment::Top : Alignment::Bottom) );
	    uiTextItem* ti = scene().addItem( new uiTextItem(txt,al) );
	    dirtxtitms_ += ti;

	    uiPoint pt( isew ? (idx==1 ? 4 : -4) : 0,
			isew ? 0 : (idx==2 ? 4 : -4) );
	    dirlnitms_ += scene().addItem( new uiLineItem(pt00,pt) );
	}
    }

    const uiPoint nln( center_.x,		center_.y-radius_ );
    const uiPoint eln( center_.x+radius_,	center_.y );
    const uiPoint sln( center_.x,		center_.y+radius_ );
    const uiPoint wln( center_.x-radius_,	center_.y );

    const uiPoint npt( center_.x,		center_.y-radius_-5 );
    const uiPoint ept( center_.x+radius_+6,	center_.y );
    const uiPoint spt( center_.x,		center_.y+radius_+2 );
    const uiPoint wpt( center_.x-radius_-6,	center_.y );

    dirtxtitms_[0]->setPos( npt ); dirlnitms_[0]->setPos( nln );
    dirtxtitms_[1]->setPos( ept ); dirlnitms_[1]->setPos( eln );
    dirtxtitms_[2]->setPos( spt ); dirlnitms_[2]->setPos( sln );
    dirtxtitms_[3]->setPos( wpt ); dirlnitms_[3]->setPos( wln );
}


void uiDirectionalPlot::drawData()
{
    markeritems_.removeAll( true );
    curveitems_.removeAll( true );

    switch ( setup_.type_ )
    {
    case Setup::Scatter:	drawScatter();	break;
    case Setup::Vals:		drawVals();	break;
    case Setup::Rose:		drawRose();	break;
    }
}


void uiDirectionalPlot::drawScatter()
{
    for ( int isect=0; isect<data_.nrSectors(); isect++ )
    {
	const Stats::SectorData& sd = *data_[isect];
	const bool ishighlighted = highlightidxs_.isPresent( isect );
	const MarkerStyle2D& ms = ishighlighted ? setup_.hlmarkstyle_
						    : setup_.markstyle_;
	const int zval = ishighlighted ? mHLMarkerZValue : mDefMarkerZValue;
	for ( int ipart=0; ipart<sd.size(); ipart++ )
	{
	    const Stats::SectorPartData& spd = sd[ipart];
	    if ( spd.count_ < 1 ) continue;

	    const float r = spd.pos_ * radius_;
	    uiMarkerItem* itm = new uiMarkerItem( dataUIPos(r,spd.val_), ms );
	    itm->setZValue( zval );
	    itm->setFillColor( ms.color_ );
	    markeritems_.add( itm );
	}
    }
}


void uiDirectionalPlot::setHighlighted( const TypeSet<int>& dataidxs )
{
    highlightidxs_ = dataidxs;
    drawData();
}


void uiDirectionalPlot::setHighlighted( int dataidx )
{
    highlightidxs_.erase();
    if ( !data_.validIdx(dataidx) )
	return;

    highlightidxs_.add( dataidx );
    drawData();
}


void uiDirectionalPlot::drawVals()
{
    if ( !colseq_ )
	colseq_ = ColTab::SM().getAny( "Directional Plot" );

    drawSectorParts( true );
}


void uiDirectionalPlot::drawRose()
{
    drawSectorParts( false );
}


uiCurvedItem* uiDirectionalPlot::drawSectorPart( int isect, Interval<float> rrg,
						 Color col )
{
    const float dang = data_.angle(0,1) - data_.angle(0,-1);
    const float dangrad = dang * Angle::cPI<float>() / 180;
    Interval<float> angrg( data_.angle(isect,-1), 0 );
    angrg.stop = angrg.start + dang;
    Interval<float> radangrg( data_.angle(isect,Angle::Rad,-1), 0 );
    radangrg.stop = radangrg.start - dangrad;

    rrg.scale( mCast(float,radius_) );
    uiCurvedItem* ci = new uiCurvedItem(
				dataUIPos(rrg.start,angrg.start) );
    ci->drawTo( dataUIPos(rrg.stop,angrg.start) );
    uiCurvedItem::ArcSpec as( center_, rrg.stop, radangrg );
    ci->drawTo( as );

    ci->drawTo( dataUIPos(rrg.start,angrg.stop) );
    as.radius_ = rrg.start;
    Swap( as.angles_.start, as.angles_.stop );
    ci->drawTo( as );
    ci->setFillColor( col );
    ci->closeCurve();
    ci->setZValue( 5 );
    return ci;
}


void uiDirectionalPlot::drawSectorParts( bool isvals )
{
    const int nrsectors = data_.nrSectors();
    if ( nrsectors < 1 ) return;

    const bool usecount = !isvals || setup_.docount_;
    if ( usecount && maxcount_ < 1 ) return;

    for ( int isect=0; isect<nrsectors; isect++ )
    {
	const Stats::SectorData& sd = *data_[isect];
	const bool reversepos = sd.first().pos_ > sd.last().pos_;

	for ( int ipart=0; ipart<sd.size(); ipart++ )
	{
	    const Stats::SectorPartData& spd = sd[ipart];
	    if ( spd.count_ < 1 ) continue;

	    Interval<float> rrg( 0, 1 );
	    if ( usecount )
		rrg.stop = ((float)spd.count_) / maxcount_;
	    else
	    {
		if ( reversepos )
		{
		    if ( ipart < sd.size()-1 )
			rrg.start = (spd.pos_ + sd[ipart+1].pos_) * .5f;
		    if ( ipart > 0 )
			rrg.stop = (spd.pos_ + sd[ipart-1].pos_) * .5f;
		}
		else
		{
		    if ( ipart )
			rrg.start = (spd.pos_ + sd[ipart-1].pos_) * .5f;
		    if ( ipart < sd.size()-1 )
			rrg.stop = (spd.pos_ + sd[ipart+1].pos_) * .5f;
		}
	    }

	    Color col;
	    if ( !isvals )
		col = Color::stdDrawColor( ipart%Color::nrStdDrawColors() );
	    else
	    {
		float relpos = (spd.val_-valrg_.start)
			     / (valrg_.stop-valrg_.start);
		if ( relpos < 0 ) relpos = 0;
		if ( relpos > 1 ) relpos = 1;
		col = colseq_->color( relpos );
	    }

	    uiCurvedItem* ci = drawSectorPart( isect, rrg, col );
	    curveitems_.add( ci );
	}
    }

    curveitems_.setZValue( 10 );
}


void uiDirectionalPlot::drawSelection()
{
    if ( selsectoritem_ )
	{ delete selsectoritem_; selsectoritem_ = 0; }
    if ( selsector_ < 0 ) return;

    selsectoritem_ = drawSectorPart( selsector_, Interval<float>(1.01,1.05),
				     Color::Black() );
}


void uiDirectionalPlot::getMousePosInfo( int& count, float& ang, float& pos )
{
    count = 0; ang = pos = mUdf(float);
    if ( getMouseEventHandler().isHandled() )
	return;

    const MouseEvent& ev = getMouseEventHandler().event();
    uiPoint relpos( ev.x(), ev.y() ); relpos -= center_;
    if ( relpos.x == 0 && relpos.y == 0 )
	return;

    const double r = relpos.abs();
    if ( r > radius_ )
	return;

    double azimuthrad = acos( relpos.x/r );
    if ( relpos.y > 0 )
	azimuthrad = 2*M_PI - azimuthrad;
    const double azimuth =
	Angle::convert( Angle::Rad, azimuthrad, Angle::UsrDeg );
    const int sector = data_.sector( (float)azimuth );

    const int nrparts = data_.nrParts( sector );
    int part = int( nrparts * r / radius_ );
    if ( part<0 ) part = 0;
    if ( part>=nrparts ) part = nrparts-1;

    count = data_.getPartData( sector, part ).count_;
    ang = (float)azimuth;
    if ( nrparts>1 )
	pos = data_.setup_.usrposrg_.start +
	      data_.setup_.usrposrg_.width()*(float)r/radius_;
}


#define mGetMousePos()  \
    if ( getMouseEventHandler().isHandled() ) \
	return; \
    const MouseEvent& ev = getMouseEventHandler().event(); \
    if ( !(ev.buttonState() & OD::LeftButton) ) \
	return; \
    const bool isctrl = ev.ctrlStatus(); \
    const bool isoth = ev.shiftStatus() || ev.altStatus(); \
    const bool isnorm = !isctrl && !isoth; \
    if ( !isnorm ) return


void uiDirectionalPlot::mouseRelease( CallBacker* )
{
    mGetMousePos();
    uiPoint relpos( ev.x(), ev.y() ); relpos -= center_;
    if ( relpos.x == 0 && relpos.y == 0 ) return;

    const float ang = Math::Atan2( (float)-relpos.y, (float)relpos.x );
    cursector_ = data_.sector( ang, Angle::Rad );
    if ( setup_.curissel_ )
    {
	if ( cursector_ == selsector_ )
	    selsector_ = -1;
	else
	    selsector_ = cursector_;
	drawSelection();
    }

    sectorPicked.trigger();
}


uiPoint uiDirectionalPlot::dataUIPos( float r, float ang ) const
{
    return uiPointFromPolar( center_, r,
		Angle::convert( data_.setup_.angletype_, ang, Angle::Rad ) );
}


uiPoint uiDirectionalPlot::usrUIPos( float r, float ang ) const
{
    return uiPointFromPolar( center_, r, Angle::usrdeg2rad(ang) );
}


void uiDirectionalPlot::setColTab( const char* nm )
{
    delete colseq_;
    colseq_ = ColTab::SM().getAny( nm );

    if ( coltabitm_ )
	coltabitm_->setColTab( nm );

    draw();
}


void uiDirectionalPlot::showColTabItem( bool yn )
{
    if ( coltabitm_ )
	coltabitm_->setVisible( yn );
}


void uiDirectionalPlot::showScaleItem( bool yn )
{
    if ( scalelineitm_ ) scalelineitm_->setVisible( yn );
    if ( scalearcitm_ ) scalearcitm_->setVisible( yn );
    if ( scalestartptitem_ ) scalestartptitem_->setVisible( yn );
    if ( scaleannotitm_ ) scaleannotitm_->setVisible( yn );
    if ( scalestartitm_ ) scalestartitm_->setVisible( yn );
    if ( scalestopitm_ ) scalestopitm_->setVisible( yn );
}
