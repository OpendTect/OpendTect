/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uidirectionalplot.cc,v 1.3 2009-04-02 10:04:03 cvsbert Exp $";

#include "uidirectionalplot.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uifont.h"
#include "mouseevent.h"
#include "angles.h"
#include <iostream>


uiDirectionalPlot::uiDirectionalPlot( uiParent* p,
				      const uiDirectionalPlot::Setup& su )
    : uiGraphicsView(p,"Function display viewer",true)
    , setup_(su)
    , selsector_(0)
    , selpart_(0)
    , outercircleitm_(0)
    , sectorlines_(*new uiGraphicsItemGroup)
    , sectorPartSelected(this)
{
    setZoomOnCtrlScroll( false );
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
}


void uiDirectionalPlot::reSized( CallBacker* )
{
    draw();
}


void uiDirectionalPlot::setVals( const float* vals, int sz )
{
    data_.erase();
    for ( int idx=0; idx<sz; idx++ )
    {
	Stats::SectorData* sd = new Stats::SectorData;
	*sd += Stats::SectorPartData( vals[idx] );
	data_ += sd;
    }

    gatherInfo(); draw();
}


void uiDirectionalPlot::setVals( const Stats::DirectionalData& dird )
{
    data_ = dird;

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
	    }
	    const Stats::SectorData& sd = *data_[isect];
	    for ( int ipart=0; ipart<sd.size(); ipart++ )
	    {
		const Stats::SectorPartData& spd = sd[ipart];
		valrg_.include( spd.val_ );
		posrg_.include( spd.pos_ );
	    }
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
    const uiRect workrect( center_.x - radius_, center_.y - radius_,
	    		   center_.x + radius_, center_.y + radius_ );

    //TODO plot data here

    drawGrid();
    drawAnnot();
}


void uiDirectionalPlot::drawGrid()
{
    if ( outercircleitm_ )
    {
	//TODO resize
    }
    else
    {
	outercircleitm_ = scene().addItem( new uiCircleItem(center_,radius_) );
	outercircleitm_->setPenStyle( setup_.circlels_ );
	for ( int idx=0; idx<4; idx++ )
	{
	    const float rad = (.2 + .2*idx)*radius_ ;
	    uiCircleItem* ci = scene().addItem( new uiCircleItem(center_,rad) );
	    equicircles_ += ci;
	    ci->setPenStyle( setup_.equils_ );
	}
    }

    sectorlines_.removeAll( true );
    const int nrsectors = data_.nrSectors();
    for ( int isect=0; isect<nrsectors; isect++ )
    {
	const float ang = Angle::usrdeg2rad( isect * 360 / ((float)nrsectors) );
	uiLineItem* li = scene().addLine( center_, ang, radius_ );
	sectorlines_.add( li );
	li->setPenStyle( setup_.sectorls_ );
    }
}


void uiDirectionalPlot::drawAnnot()
{
    if ( dirtxtitms_.isEmpty() )
    {
	for ( int idx=0; idx<4; idx++ )
	{
	    const char* txt = idx == 0 ? "N"
			   : (idx == 1 ? "E"
			   : (idx==2 ?	 "S"
			   :		 "W"));
	    uiTextItem* ti = scene().addText( txt );
	    Alignment al( idx%2 ? (idx==1 ? Alignment::Left : Alignment::Right)
		    			  : Alignment::HCenter,
		          idx%2 ? Alignment::VCenter
			  : (idx == 2 ? Alignment::Top : Alignment::Bottom) );
std::cerr << txt << '=' << eString(Alignment::HPos,al.hPos());
std::cerr << '/' << eString(Alignment::VPos,al.vPos()) << std::endl;
	    ti->setAlignment( al );
	    dirtxtitms_ += ti;
	}
    }

    dirtxtitms_[0]->setPos( center_.x, center_.y - radius_ - 2 );
    dirtxtitms_[1]->setPos( center_.x + radius_ + 2, center_.y );
    dirtxtitms_[2]->setPos( center_.x, center_.y + radius_ + 2 );
    dirtxtitms_[3]->setPos( center_.x - radius_ - 2, center_.y );
}


#define mGetMousePos()  \
    if ( getMouseEventHandler().isHandled() ) \
	return; \
    const MouseEvent& ev = getMouseEventHandler().event(); \
    if ( !(ev.buttonState() & OD::RightButton) ) \
        return; \
    const bool isctrl = ev.ctrlStatus(); \
    const bool isoth = ev.shiftStatus() || ev.altStatus(); \
    const bool isnorm = !isctrl && !isoth; \
    if ( !isnorm ) return


void uiDirectionalPlot::mouseRelease( CallBacker* )
{
    mGetMousePos();
}
