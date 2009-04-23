/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uidirectionalplot.cc,v 1.18 2009-04-23 15:15:20 cvsbert Exp $";

#include "uidirectionalplot.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uifont.h"
#include "angles.h"
#include "mouseevent.h"
#include "coltabsequence.h"
#include <iostream>


uiDirectionalPlot::uiDirectionalPlot( uiParent* p,
				      const uiDirectionalPlot::Setup& su )
    : uiGraphicsView(p,"Function display viewer")
    , setup_(su)
    , selsector_(-1)
    , cursector_(-1)
    , outercircleitm_(0)
    , selsectoritem_(0)
    , sectorlines_(*scene().addItemGrp(new uiGraphicsItemGroup))
    , curveitems_(*scene().addItemGrp(new uiGraphicsItemGroup))
    , markeritems_(*scene().addItemGrp(new uiGraphicsItemGroup))
    , sectorPicked(this)
    , colseq_(0)
{
    disableScrollZoom();
    setPrefWidth( setup_.prefsize_.width() );
    setPrefHeight( setup_.prefsize_.height() );
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
    delete colseq_;
}


void uiDirectionalPlot::reSized( CallBacker* )
{
    draw();
}


void uiDirectionalPlot::setData( const float* vals, int sz )
{
    data_.erase();
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

    cursector_ = selsector_ = -1;
    gatherInfo(); draw();
}


void uiDirectionalPlot::gatherInfo()
{
    isempty_ = true;
    for ( int isect=0; isect<data_.size(); isect++ )
    {
	const int nrparts = data_.nrParts(isect);
	if ( nrparts > 0 )
	{
	    if ( isempty_ )
	    {
		isempty_ = false;
		const Stats::SectorPartData& spd = data_.get(isect,0);
		valrg_.start = valrg_.stop = spd.val_;
		posrg_.start = posrg_.stop = spd.pos_;
		maxcount_ = spd.count_;
	    }
	    const Stats::SectorData& sd = *data_[isect];
	    int curcount = 0;
	    for ( int ipart=0; ipart<sd.size(); ipart++ )
	    {
		const Stats::SectorPartData& spd = sd[ipart];
		valrg_.include( spd.val_ );
		posrg_.include( spd.pos_ );
		curcount += spd.count_;
	    }
	    if ( curcount > maxcount_ ) maxcount_ = curcount;
	}
    }
}


void uiDirectionalPlot::draw()
{
    if ( isempty_ ) return;
    const uiSize uitotsz( width(), height() );
    uiBorder border( font()->height() + 5 );
    center_ = uiPoint( uitotsz.width() / 2, uitotsz.height() / 2 );
    const uiRect avrect( border.getRect(uitotsz) );
    radius_ = (avrect.width() > avrect.height()
		     ? avrect.height() : avrect.width()) / 2;

    drawData();
    drawGrid();
    drawAnnot();
    drawSelection();
}


void uiDirectionalPlot::drawGrid()
{
    if ( outercircleitm_ )
    {
	outercircleitm_->setPos( center_ );
	outercircleitm_->setRadius( radius_ );
	for ( int idx=0; idx<4; idx++ )
	{
	    const float rad = (.2 + .2*idx)*radius_ ;
	    uiCircleItem& ci = *equicircles_[idx];
	    ci.setPos( center_ ); ci.setRadius( mNINT(rad) );
	}
    }
    else
    {
	outercircleitm_ = scene().addItem( new uiCircleItem(center_,radius_) );
	outercircleitm_->setPenStyle( setup_.circlels_ );
	for ( int idx=0; idx<4; idx++ )
	{
	    const float rad = (.2 + .2*idx)*radius_ ;
	    uiCircleItem* ci = scene().addItem( new uiCircleItem(center_,
								 mNINT(rad)) );
	    equicircles_ += ci;
	    ci->setPenStyle( setup_.equils_ );
	}
    }

    sectorlines_.removeAll( true );
    const int nrsectors = data_.nrSectors();
    const float stepang = 360 / ((float)nrsectors);
    for ( int isect=0; isect<nrsectors; isect++ )
    {
	const float ang = data_.angle( isect, 1 );
	const float mathang = Angle::convert( data_.setup_.angletype_, ang,
					      Angle::Rad );
	uiLineItem* li = scene().addItem(
		new uiLineItem(center_,mathang,radius_,true) );
	sectorlines_.add( li );
	li->setPenStyle( setup_.sectorls_ );
    }
}


void uiDirectionalPlot::drawAnnot()
{
    if ( dirtxtitms_.isEmpty() )
    {
	const uiPoint pt00( 0, 0 );
	for ( int idx=0; idx<4; idx++ )
	{
	    const bool isew = idx % 2;
	    const char* txt = idx == 0 ? "N"
			   : (idx == 1 ? "E"
			   : (idx==2 ?	 "S"
			   :		 "W"));
	    Alignment al( isew ? (idx==1 ? Alignment::Left : Alignment::Right)
		    			  : Alignment::HCenter,
		          isew ? Alignment::VCenter
			  : (idx == 2 ? Alignment::Top : Alignment::Bottom) );
	    uiTextItem* ti = scene().addItem( new uiTextItem(txt,al) );
	    dirtxtitms_ += ti;

	    uiPoint pt( isew ? (idx==1 ? 2 : -2) : 0,
		        isew ? 0 : (idx==2 ? 2 : -2) );
	    dirlnitms_ += scene().addItem( new uiLineItem(pt00,pt,false) );
	}
    }

    const uiPoint npt( center_.x, center_.y - radius_ - 2 );
    const uiPoint ept( center_.x + radius_ + 2, center_.y );
    const uiPoint spt( center_.x, center_.y + radius_ + 2 );
    const uiPoint wpt( center_.x - radius_ - 2, center_.y );
    dirtxtitms_[0]->setPos( npt ); dirlnitms_[0]->setPos( npt );
    dirtxtitms_[1]->setPos( ept ); dirlnitms_[1]->setPos( ept );
    dirtxtitms_[2]->setPos( spt ); dirlnitms_[2]->setPos( spt );
    dirtxtitms_[3]->setPos( wpt ); dirlnitms_[3]->setPos( wpt );
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
	for ( int ipart=0; ipart<sd.size(); ipart++ )
	{
	    const Stats::SectorPartData& spd = sd[ipart];
	    if ( spd.count_ < 1 ) continue;

	    const float r = spd.pos_ * radius_;
	    markeritems_.add( new uiMarkerItem(getUIPos(r,spd.val_),
					       setup_.markstyle_) );
	}
    }
}


void uiDirectionalPlot::drawVals()
{
    if ( data_.nrSectors() < 1 ) return;
    if ( !colseq_ )
    {
	colseq_ = new ColTab::Sequence;
	if ( !ColTab::SM().get("Directional Plot",*colseq_)
	  && !ColTab::SM().get("Rainbow",*colseq_) )
	{
	    colseq_->setColor( 0, 255, 255, 0 );
	    colseq_->setColor( 1, 0, 0, 255 );
	}
    }

    const float dang = data_.angle(0,1) - data_.angle(0,-1);
    const float dangrad = dang * Angle::cPI(dang) / 180;

    const int nrsectors = data_.nrSectors();
    for ( int isect=0; isect<nrsectors; isect++ )
    {
	const Stats::SectorData& sd = *data_[isect];
	Interval<float> angrg( data_.angle(isect,-1), 0 );
	angrg.stop = angrg.start + dang;
	Interval<float> radangrg( data_.angle(isect,Angle::Rad,-1), 0 );
	radangrg.stop = radangrg.start - dangrad;

	const bool reversepos = sd.first().pos_ > sd.last().pos_;
	for ( int ipart=0; ipart<sd.size(); ipart++ )
	{
	    const Stats::SectorPartData& spd = sd[ipart];
	    if ( spd.count_ < 1 ) continue;

	    Interval<float> rrg( 0, 1 );
	    if ( reversepos )
	    {
		if ( ipart < sd.size()-1 )
		    rrg.start = (spd.pos_ + sd[ipart+1].pos_) * .5;
		if ( ipart > 0 )
		    rrg.stop = (spd.pos_ + sd[ipart-1].pos_) * .5;
	    }
	    else
	    {
		if ( ipart )
		    rrg.start = (spd.pos_ + sd[ipart-1].pos_) * .5;
		if ( ipart < sd.size()-1 )
		    rrg.stop = (spd.pos_ + sd[ipart+1].pos_) * .5;
	    }

	    rrg.scale( radius_ );
	    uiCurvedItem* ci = new uiCurvedItem(
					getUIPos(rrg.start,angrg.start) );
	    ci->drawTo( getUIPos(rrg.stop,angrg.start) );
	    uiCurvedItem::ArcSpec as( center_, rrg.stop, radangrg );
	    ci->drawTo( as );

	    ci->drawTo( getUIPos(rrg.start,angrg.stop) );
	    as.radius_ = rrg.start;
	    Swap( as.angles_.start, as.angles_.stop );
	    ci->drawTo( as );
	    // Grey scales
	    //TODO use real color bar
<<<<<<< uidirectionalplot.cc
	    float relpos = (valrg_.stop - spd.val_)/(valrg_.stop-valrg_.start);
	    ci->setFillColor( colseq_->color(relpos) );
=======
	    float v = 255 * (valrg_.stop - spd.val_)/(valrg_.stop-valrg_.start);
	    int cval = mNINT(v);
	    ci->setFillColor( Color(cval,cval,cval) );
>>>>>>> 1.17

	    ci->closeCurve();
	    curveitems_.add( ci );
	}
    }
}


void uiDirectionalPlot::drawSelection()
{
    scene().removeItem( selsectoritem_ );
    delete selsectoritem_; selsectoritem_ = 0;
    const int selsect = selSector();
    if ( selsect < 0 ) return;

    const Stats::SectorData& sd = *data_[selsect];
    const Interval<float> angrg( data_.angle(selsect,1),
				 data_.angle(selsect,-1) );
    const Interval<float> radangrg( data_.angle(selsect,Angle::Deg,1),
				    data_.angle(selsect,Angle::Rad,-1) );
    selsectoritem_ = new uiCurvedItem( getUIPos(radius_+1,angrg.start) );
    selsectoritem_->drawTo( getUIPos(radius_+10,angrg.start) );
    selsectoritem_->drawTo( getUIPos(radius_+1,angrg.start) );
    uiCurvedItem::ArcSpec as( center_, radius_+1, radangrg );
    selsectoritem_->drawTo( as );
    selsectoritem_->drawTo( getUIPos(radius_+10,angrg.stop) );
    scene().addItem( selsectoritem_ );
}


void uiDirectionalPlot::drawRose()
{
    scene().addItem( new uiTextItem(center_,"TODO: Rose diagrams") );

    bool isstacked = false;
    for ( int isect=0; isect<data_.nrSectors(); isect++ )
	{ if ( data_[isect]->size() > 1 ) isstacked = true; break; }

    for ( int isect=0; isect<data_.nrSectors(); isect++ )
    {
	const Stats::SectorData& sd = *data_[isect];
	for ( int ipart=0; ipart<sd.size(); ipart++ )
	{
	}
    }
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

    const float ang = atan2( (float)-relpos.y, (float)relpos.x );
    cursector_ = data_.sector( ang, Angle::Rad );

    sectorPicked.trigger();
}


uiPoint uiDirectionalPlot::getUIPos( float r, float ang ) const
{
    const float angrad =
		Angle::convert( data_.setup_.angletype_, ang, Angle::Rad );
    Geom::Point2D<float> fpt( center_.x + r * cos(angrad),
			      center_.y - r * sin(angrad) );
    return uiPoint( mNINT(fpt.x), mNINT(fpt.y) );
}


int uiDirectionalPlot::selSector() const
{
    return setup_.curissel_ ? cursector_ : selsector_;
}
