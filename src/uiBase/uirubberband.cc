/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		February 2007
 RCS:		$Id: uirubberband.cc,v 1.2 2007-02-12 13:53:02 cvsnanne Exp $
________________________________________________________________________

-*/

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


void uiRubberBand::extend( QMouseEvent* event )
{
    const QPoint qorigin( origin_.x, origin_.y );
    const QRect geom = QRect( qorigin, event->pos() ).normalized();
    area_ = uiRect( geom.left(), geom.top(), geom.right(), geom.bottom() );
    qrubberband_->setGeometry( geom );
}


void uiRubberBand::stop( QMouseEvent* event )
{
    const QRect geom = qrubberband_->geometry();
    area_ = uiRect( geom.left(), geom.top(), geom.right(), geom.bottom() );
    qrubberband_->hide();
}
