/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		January 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uigraphicscoltab.cc,v 1.1 2009-05-22 08:31:03 cvsnanne Exp $";


#include "uigraphicscoltab.h"
#include "uigraphicsitemimpl.h"

#include "coltabmapper.h"
#include "coltabsequence.h"
#include "draw.h"
#include "pixmap.h"

uiColTabItem::uiColTabItem()
    : uiGraphicsItemGroup()
    , ctbarsz_(25,100)
{
    ctseqitm_ = new uiPixmapItem();
    minvalitm_ = new uiTextItem( "0",
			mAlignment(Alignment::HCenter,Alignment::Top) );
    maxvalitm_ = new uiTextItem( "1",
			mAlignment(Alignment::HCenter,Alignment::Bottom) );

    add( ctseqitm_ ); add( minvalitm_ ); add( maxvalitm_ );
}


uiColTabItem::~uiColTabItem()
{
    removeAll( true );
}


void uiColTabItem::setColTabSequence( const ColTab::Sequence& ctseq )
{
    ioPixmap pm( ctseq, ctbarsz_.hNrPics(), ctbarsz_.vNrPics() );
    ctseqitm_->setPixmap( pm );
}


void uiColTabItem::setColTabMapperSetup( const ColTab::MapperSetup& ms )
{
    Interval<float> rg( ms.start_, ms.start_+ms.width_ );
    minvalitm_->setText( toString(rg.start) );
    maxvalitm_->setText( toString(rg.stop) );
}


void uiColTabItem::setPos( const uiPoint& pt )
{
    ctseqitm_->setPos( pt );

    const int barcenterx = pt.x + ctbarsz_.hNrPics()/2;
    maxvalitm_->setPos( barcenterx, pt.y-2 );
    minvalitm_->setPos( barcenterx, pt.y+ctbarsz_.vNrPics()+2 );
}
