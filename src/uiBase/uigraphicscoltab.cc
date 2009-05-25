/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		January 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uigraphicscoltab.cc,v 1.2 2009-05-25 09:16:00 cvsdgb Exp $";


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
    minvalitm_ = new uiTextItem( "0", mAlignment(HCenter,Top) );
    maxvalitm_ = new uiTextItem( "1", mAlignment(HCenter,Bottom) );

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
