/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		February 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uirubberband.cc,v 1.5 2009/07/22 16:01:38 cvsbert Exp $";

#include "uirubberband.h"

#include <QMouseEvent>
#include <QRubberBand>

#include <iostream>

uiRubberBand::~uiRubberBand()
{
    delete qrubberband_;
}


void uiRubberBand::start( QMouseEvent* event )
{
    origin_ = uiPoint( event->x(), event->y() );
    if ( !qrubberband_ )
	qrubberband_ = new QRubberBand( QRubberBand::Rectangle, parent_ );

    qrubberband_->setGeometry( QRect(event->pos(),QSize()) );
    qrubberband_->show();
}


void uiRubberBand::handleEv( QMouseEvent* event, bool setgeom )
{
    const QPoint qorigin( origin_.x, origin_.y );
    const QRect geom = QRect( qorigin, event->pos() ).normalized();
    area_ = uiRect( geom.left(), geom.top(), geom.right(), geom.bottom() );
    if ( setgeom )
	qrubberband_->setGeometry( geom );
}


void uiRubberBand::extend( QMouseEvent* event )
{
    handleEv( event, true );
}


void uiRubberBand::stop( QMouseEvent* event )
{
    handleEv( event, false );
    qrubberband_->hide();
}
