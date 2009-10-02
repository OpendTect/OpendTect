/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Karthika
 Date:          Sep 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uipolardiagram.cc,v 1.1 2009-10-02 15:49:14 cvskarthika Exp $";

#include "uipolardiagram.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "angles.h"


uiPolarDiagram::uiPolarDiagram( uiParent* p )
    : uiGraphicsView(p,"Polar diagram")
    , circleitm_(0)
    , pointeritm_(0)
    , center_(uiPoint(5, 5))
    , radius_(1)
    , azimuth_(0)
    , dip_(0)		
{
    disableScrollZoom();
    setPrefWidth( 300 );
    setPrefHeight( 300 );
    getMouseEventHandler().buttonReleased.notify(
	    mCB(this,uiPolarDiagram,mouseRelease) );
    reSize.notify( mCB(this,uiPolarDiagram,reSized) );
    setScrollBarPolicy( true, uiGraphicsView::ScrollBarAlwaysOff );
    setScrollBarPolicy( false, uiGraphicsView::ScrollBarAlwaysOff );
}


uiPolarDiagram::~uiPolarDiagram()
{
    getMouseEventHandler().buttonReleased.remove(
	    mCB(this,uiPolarDiagram,mouseRelease) );
    reSize.remove( mCB(this,uiPolarDiagram,reSized) );
    if ( circleitm_ )
	delete scene().removeItem( circleitm_ );
    if ( pointeritm_ )
	delete scene().removeItem( pointeritm_ );
    if ( segmentitms_.size() )
    {
	for ( int idx = 0; idx < segmentitms_.size(); idx++ )
	    scene().removeItem( segmentitms_[idx] );
        deepErase( segmentitms_ );
    }
    if ( textitms_.size() )
    {
	for ( int idx = 0; idx < textitms_.size(); idx++ )
	    scene().removeItem( textitms_[idx] );
        deepErase( textitms_ );
    }
}


void uiPolarDiagram::draw()
{
    center_ = uiPoint( width() / 2, height() / 2 );
    radius_ = mMIN ( width(), height() ) / 2 - 40;

    drawClock();
    drawSegments();
    drawPointer();
}


void uiPolarDiagram::drawClock()
{
    if ( circleitm_ )
    {
	circleitm_->setPos( center_ );
	circleitm_->setRadius( radius_ );
    }
    else
        circleitm_ = scene().addItem( new uiCircleItem( center_, radius_ ) );
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

	if ( create )
	{
	    uiLineItem* li = scene().addItem( 
		    new uiLineItem( center_.x, center_.y, center_.x+x, 
				    center_.y+y , true ) );
  	    segmentitms_ += li;

	    uiTextItem* ti = scene().addItem( new uiTextItem( 
			toString( angle ) ) );
	    textitms_ += ti;
	}
	else
	{
	    segmentitms_[idx]->setLine( center_.x, center_.y, center_.x+x, 
		    			 center_.y+y );
	}
	
	int hgap = ( x < 0 ) ? -25 : 5;
	int vgap = ( y < 0 ) ? -25 : 5;

	textitms_[idx]->setPos( center_.x+x+hgap, center_.y+y+vgap );
    }
}


void uiPolarDiagram::drawPointer()
{
    if ( !pointeritm_ )
    {
	pointeritm_ = scene().addItem( new uiMarkerItem );
	LineStyle ls( LineStyle::Solid, 3, Color( 255, 0, 0 ) );
	pointeritm_->setPenStyle( ls );
    }
	
    pointeritm_->setPos( center_.x, center_.y );
}


void uiPolarDiagram::mouseRelease( CallBacker* )
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

    if ( relpos.x == 0 && relpos.y == 0 ) return;

    // Formulas: r = cos(dip) and x = r cos(azimuth)
    float r = sqrt( relpos.x*relpos.x + relpos.y*relpos.y );
    if ( r > radius_ ) return;
    float diprad = acos( r );
    dip_ = Angle::convert( Angle::Rad, diprad, Angle::Deg );
    float azimuthrad = acos( relpos.x/r );
    azimuth_ = Angle::convert( Angle::Rad, azimuthrad, Angle::Deg );
   
    pointeritm_->setPos( ev.x(), ev.y() );
}


void uiPolarDiagram::reSized( CallBacker* )
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
    float azimuthrad = Angle::convert( Angle::Deg, azimuth_, Angle::Rad );
    float diprad = Angle::convert( Angle::Deg, dip_, Angle::Rad );
    
    // Formulas: r = cos(dip), x = r cos(azimuth) and y = r sin(azimuth)
    float r = cos( diprad );
    int x = (int) (r * cos( azimuthrad ));
    int y = (int) (r * sin( azimuthrad ));
    pointeritm_->setPos( center_.x+x, center_.y+y );
}


