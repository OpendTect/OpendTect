/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		February 2007
 RCS:		$Id: uirubberband.cc,v 1.1 2007-02-08 21:26:48 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uirubberband.h"

#include <QRubberBand>


uiRubberBand::~uiRubberBand()
{
    delete qrubberband_;
}


uiRubberBand::start( QMouseEvent* event )
{
    origin = event->pos();
    if ( !qrubberband_ )
	qrubberband_ = new QRubberBand( QRubberBand::Rectangle, parent_ );

    qrubberband_->setGeometry( QRect(origin,QSize()) );
    qrubberband_->show();
}


uiRubberBand::move( QMouseEvent* event )
{
     qrubberband_->setGeometry( QRect(origin,event->pos()).normalized() );
}


uiRubberBand::stop( QMouseEvent* event )
{
    qrubberband_->hide();
}
