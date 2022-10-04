/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uirubberband.h"

#include <QMouseEvent>
#include <QRubberBand>

mUseQtnamespace

uiRubberBand::uiRubberBand( QWidget* qw )
    : parent_(qw)
{}


uiRubberBand::~uiRubberBand()
{
    delete qrubberband_;
}


void uiRubberBand::start( QMouseEvent* event )
{
    origin_ = uiPoint( event->pos().x(), event->pos().y() );
    if ( !qrubberband_ )
	qrubberband_ = new QRubberBand( QRubberBand::Rectangle, parent_ );

    qrubberband_->setGeometry( QRect(event->pos(), QSize()) );
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
