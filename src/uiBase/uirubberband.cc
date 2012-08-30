/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		February 2007
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uirubberband.cc,v 1.8 2012-08-30 07:52:52 cvsnageswara Exp $";

#include "uirubberband.h"

#include <QMouseEvent>
#include <QRubberBand>

#include <iostream>

uiRubberBand::~uiRubberBand()
{
    delete qrubberband_;
}


void uiRubberBand::start( mQtclass(QMouseEvent*) event )
{
    origin_ = uiPoint( event->x(), event->y() );
    if ( !qrubberband_ )
	qrubberband_ = new mQtclass(QRubberBand)(
				   mQtclass(QRubberBand)::Rectangle, parent_ );

    qrubberband_->setGeometry( mQtclass(QRect)(event->pos(),
					       mQtclass(QSize)()) );
    qrubberband_->show();
}


void uiRubberBand::handleEv( mQtclass(QMouseEvent*) event, bool setgeom )
{
    const mQtclass(QPoint) qorigin( origin_.x, origin_.y );
    const mQtclass(QRect) geom = mQtclass(QRect)( qorigin,
	    					  event->pos() ).normalized();
    area_ = uiRect( geom.left(), geom.top(), geom.right(), geom.bottom() );
    if ( setgeom )
	qrubberband_->setGeometry( geom );
}


void uiRubberBand::extend( mQtclass(QMouseEvent*) event )
{
    handleEv( event, true );
}


void uiRubberBand::stop( mQtclass(QMouseEvent*) event )
{
    handleEv( event, false );
    qrubberband_->hide();
}
