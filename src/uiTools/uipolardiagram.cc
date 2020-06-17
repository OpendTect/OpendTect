/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Karthika
 Date:          Sep 2009
________________________________________________________________________

-*/

#include "uipolardiagram.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "angles.h"
#include "uistrings.h"

//#define mShowEast

uiPolarDiagram::uiPolarDiagram( uiParent* p )
    : uiGraphicsView(p,"Polar diagram")
    , valueChanged(this)
    , pointeritm_(0)
    , center_(uiPoint(5, 5))
    , radius_(1)
    , azimuth_(0)
    , dip_(90)
{
    disableScrollZoom();
    setPrefWidth( 250 );
    setPrefHeight( 250 );
    getMouseEventHandler().buttonPressed.notify(
	    mCB(this,uiPolarDiagram,mouseEventCB) );
    getMouseEventHandler().movement.notify(
	    mCB(this,uiPolarDiagram,mouseEventCB) );
    reSize.notify( mCB(this,uiPolarDiagram,reSizedCB) );
    setScrollBarPolicy( true, uiGraphicsView::ScrollBarAlwaysOff );
    setScrollBarPolicy( false, uiGraphicsView::ScrollBarAlwaysOff );
}


uiPolarDiagram::~uiPolarDiagram()
{
    getMouseEventHandler().buttonPressed.remove(
	    mCB(this,uiPolarDiagram,mouseEventCB) );
    getMouseEventHandler().movement.remove(
	    mCB(this,uiPolarDiagram,mouseEventCB) );
    reSize.remove( mCB(this,uiPolarDiagram,reSizedCB) );

    delete pointeritm_;
    deepErase( circleitms_ );
    deepErase( segmentitms_ );
    deepErase( azimuthtextitms_ );
    deepErase( diptextitms_ );
}


void uiPolarDiagram::draw()
{
    center_ = uiPoint( viewWidth()/2, viewHeight()/2 );
    radius_ = mMIN(viewWidth(),viewHeight()) / 2 - 40;

    drawCircles();
    drawSegments();
    drawPointer();
}


void uiPolarDiagram::drawCircles()
{
    if ( circleitms_.size() )
    {
	circleitms_[0]->setPos( center_ );
	circleitms_[1]->setPos( center_ );
	circleitms_[2]->setPos( center_ );
	circleitms_[0]->setRadius( radius_ );
	circleitms_[1]->setRadius( radius_*2/3 );
	circleitms_[2]->setRadius( radius_/3 );
    }
    else
    {
	Color maroon = Color( 128, 0, 0 );

#define mAddDipLabel(text)	\
	{	\
        uiTextItem* ti = scene().addItem( new uiTextItem( text ) );	\
	ti->setTextColor( maroon );	\
	diptextitms_ += ti;	\
	}

	uiCircleItem* ci = scene().addItem(
		new uiCircleItem( center_, radius_ ) );
        circleitms_ += ci;
	mAddDipLabel( toUiString("0") );

	ci = scene().addItem( new uiCircleItem( center_, radius_*2/3 ) );
        circleitms_ += ci;
	mAddDipLabel( toUiString("30") );

        ci = scene().addItem( new uiCircleItem( center_, radius_*1/3 ) );
        circleitms_ += ci;
	mAddDipLabel( toUiString("60") );

	mAddDipLabel( toUiString("90") );
    }

    diptextitms_[0]->setPos( uiPoint( center_.x_+radius_, center_.y_-20) );
    diptextitms_[1]->setPos( uiPoint( center_.x_+(radius_*2/3), center_.y_-20));
    diptextitms_[2]->setPos( uiPoint( center_.x_+(radius_/3), center_.y_-20) );
    diptextitms_[3]->setPos( uiPoint( center_.x_, center_.y_-20) );
}


void uiPolarDiagram::drawSegments()
{
    bool create = !segmentitms_.size();

    for ( int angle = 0, idx = 0; angle < 360; angle += 30, idx++ )
    {
	float angle_rad = Angle::convert(
		Angle::Deg, (float) angle, Angle::Rad );
	int x = (int) (radius_ * cos( angle_rad ));
	int y = (int) (radius_ * sin( angle_rad ));
        // y-axis direction on the canvas is the opposite of that in geometry
	y = -y;

	if ( create )
	{
	    uiLineItem* li = scene().addItem(
		    new uiLineItem( center_.x_,center_.y_,
				    center_.x_+x,center_.y_+y ) );
	    segmentitms_ += li;

	    float usrangle = Angle::convert(
		    Angle::Deg, float(angle), Angle::UsrDeg );
	    uiTextItem* ti = scene().addItem( new uiTextItem(
			toUiString( usrangle ) ) );
	    azimuthtextitms_ += ti;
	}
	else
	{
	    segmentitms_[idx]->setLine( center_.x_, center_.y_, center_.x_+x,
					 center_.y_+y );
	}

	int hgap = ( x < 0 ) ? -25 : 5;
	int vgap = ( y < 0 ) ? -25 : 5;

	azimuthtextitms_[idx]->setPos( mCast(float,center_.x_+x+hgap),
				       mCast(float,center_.y_+y+vgap) );
    }

    if ( create )
    {
	// create E and N text items
#ifdef mShowEast
        uiTextItem* tiE = scene().addItem(
			    new uiTextItem( uiStrings::sEast(true) ) );
	tiE->setTextColor( Color( 0, 0, 255 ) );
        azimuthtextitms_ += tiE;
#endif
        uiTextItem* tiN = scene().addItem(
			    new uiTextItem( uiStrings::sNorth(true) ) );
	tiN->setTextColor( Color( 0, 0, 255 ) );
        azimuthtextitms_ += tiN;
    }

#ifdef mShowEast
    azimuthtextitms_[azimuthtextitms_.size()-2]->setPos(
	    center_.x_+radius_+5, center_.y_-10 );
#endif
    azimuthtextitms_[azimuthtextitms_.size()-1]->setPos(
	    mCast(float,center_.x_-5), mCast(float,center_.y_-radius_-25) );
}


void uiPolarDiagram::drawPointer()
{
    if ( !pointeritm_ )
    {
	pointeritm_ = scene().addItem( new uiMarkerItem );
	OD::LineStyle ls( OD::LineStyle::Solid, 3, Color( 255, 0, 0 ) );
	pointeritm_->setPenStyle( ls );
    }

    updatePointer();
}


void uiPolarDiagram::mouseEventCB( CallBacker* )
{
    if ( getMouseEventHandler().isHandled() )
	return;

    const MouseEvent& ev = getMouseEventHandler().event();
    if ( !(ev.buttonState() & OD::LeftButton) )
        return;

    const bool isctrl = ev.ctrlStatus();
    const bool isoth = ev.shiftStatus() || ev.altStatus();
    const bool isnorm = !isctrl && !isoth;
    if ( !isnorm ) return;

    uiPoint relpos( ev.x(), ev.y() );
    relpos -= center_;

    if ( relpos.x_ == 0 && relpos.y_ == 0 ) return;

    // Formula: x = r cos(azimuth)
    float r = (float) Math::Sqrt( (float)
				  (relpos.x_*relpos.x_ + relpos.y_*relpos.y_) );
    if ( r > radius_ ) return;
    float azimuthrad = acos( relpos.x_/r );
    if ( relpos.y_ > 0 )
	azimuthrad = (float) ( 2*M_PI - azimuthrad );
    azimuth_ = Angle::convert( Angle::Rad, azimuthrad, Angle::UsrDeg );

    // Outermost circle - dip = 0, center - dip = 90 degrees
    dip_ = (float) (radius_ - r) * 90 / radius_;
    pointeritm_->setPos( mCast(float,ev.x()), mCast(float,ev.y()) );

    valueChanged.trigger();
}


void uiPolarDiagram::reSizedCB( CallBacker* )
{
    draw();
}


void uiPolarDiagram::setValues(float azimuth, float dip)
{
    if ( azimuth >= 0 && azimuth <= 360 )
        azimuth_ = azimuth;
    if ( dip >= 0 && dip <= 90 )
        dip_ = dip;

    updatePointer();
}


void uiPolarDiagram::getValues(float* azimuth, float* dip) const
{
    *azimuth = azimuth_;
    *dip = dip_;
}


// Relocate pointer to position specified by azimuth_ and dip_.
void uiPolarDiagram::updatePointer()
{
    float azimuthrad = Angle::convert( Angle::UsrDeg, azimuth_, Angle::Rad );

    float r = radius_ - dip_*radius_/90;
    int x = (int) (r * cos( azimuthrad ));
    int y = (int) (r * sin( azimuthrad ));
    if ( pointeritm_ )
    {
      pointeritm_->setPos( mCast(float,center_.x_+x),
			   mCast(float,center_.y_-y) );
    }
        // y-axis direction on the canvas is the opposite of that in geometry
}
