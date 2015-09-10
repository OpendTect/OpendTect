/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uigraphicscoltab.h"

#include "uigraphicsitemimpl.h"
#include "uipixmap.h"

#include "coltabmapper.h"
#include "coltabsequence.h"

uiColTabItem::uiColTabItem( const uiColTabItem::Setup& su )
    : uiGraphicsItemGroup()
    , setup_(su)
{
    ctseqitm_ = new uiPixmapItem();
    borderitm_ = new uiRectItem();
    minvalitm_ = new uiAdvancedTextItem( toUiString("0") );
    maxvalitm_ = new uiAdvancedTextItem( toUiString("1") );
    add( borderitm_ ); add( ctseqitm_ ); add( minvalitm_ ); add( maxvalitm_ );

    setColTabSequence( ColTab::Sequence("") );

    uiRect boundrec = ctseqitm_->boundingRect();
    ctseqitm_->setPos( 0, 0 );
    borderitm_->setRect( -1, -1, boundrec.width()+1, boundrec.height()+1 );

    update();
}


uiColTabItem::~uiColTabItem()
{
    removeAll( true );
}


void uiColTabItem::update()
{
    uiRect boundrec = boundingRect();
    minvalitm_->setAlignment( Alignment(Alignment::HCenter, Alignment::Bottom));
    minvalitm_->setPos( boundrec.width()/2, 0 );
    maxvalitm_->setAlignment( Alignment(Alignment::HCenter, Alignment::Top) );
    maxvalitm_->setPos( boundrec.width()/2, boundrec.height() );
/*
    uiPoint curpos_( 0, 0 );
    const uiRect rect( curpos_, setup_.sz_ );
    const int dist = -6;
    const uiRect drect( uiPoint(curpos_.x-dist,curpos_.y-dist),
		 uiSize(setup_.sz_.width()+2*dist,setup_.sz_.height()+2*dist) );
    const uiPoint center( rect.centre() );
#   define mSetAl(itm,h,v) itm->setAlignment( Alignment(h,v) )

    if ( setup_.hor_ )
    {
	const int starty =
	    setup_.startal_.vPos() == Alignment::VCenter ? center.y
	 : (setup_.startal_.vPos() == Alignment::Top ?     drect.top()
						     :     drect.bottom());
	if ( setup_.startalong_ )
	{
	    mSetAl( minvalitm_, Alignment::Left,
		    Alignment::opposite(setup_.startal_.vPos()) );
	    minvalitm_->setPos( (float) rect.left(), (float) starty );
	}
	else
	{
	    mSetAl( minvalitm_, Alignment::Right, setup_.startal_.vPos() );
	    minvalitm_->setPos( (float) drect.left(), (float) starty );
	}
	const int stopy =
	    setup_.stopal_.vPos() == Alignment::VCenter ? center.y
	 : (setup_.stopal_.vPos() == Alignment::Top ?     drect.top()
						    :     drect.bottom());
	if ( setup_.stopalong_ )
	{
	    mSetAl( maxvalitm_, Alignment::Right,
		    Alignment::opposite(setup_.stopal_.vPos()) );
	    maxvalitm_->setPos( (float) rect.right(), (float) stopy );
	}
	else
	{
	    mSetAl( maxvalitm_, Alignment::Left, setup_.stopal_.vPos() );
	    maxvalitm_->setPos( (float) drect.right(), (float) stopy );
	}
    }
    else
    {
	const int startx =
	    setup_.startal_.hPos() == Alignment::HCenter ? center.x
	 : (setup_.startal_.hPos() == Alignment::Left ?    drect.left()
						      :    drect.right());
	const Alignment::HPos oppal = Alignment::opposite(
						setup_.startal_.hPos() );

	if ( setup_.startalong_ )
	{
	    mSetAl( minvalitm_, oppal, Alignment::Bottom );
	    minvalitm_->setPos( (float) startx, (float) rect.bottom() );
	}
	else
	{
	    mSetAl( minvalitm_, oppal, Alignment::Top );
	    minvalitm_->setPos( (float) startx, (float) drect.bottom() );
	}

	if ( setup_.stopalong_ )
	{
	    mSetAl( maxvalitm_, oppal, Alignment::Top );
	    maxvalitm_->setPos( (float) startx, (float) rect.top() );
	}
	else
	{
	    mSetAl( maxvalitm_, oppal, Alignment::Bottom );
	    maxvalitm_->setPos( (float) startx, (float) drect.top() );
	}
    }
    */
}


void uiColTabItem::setColTab( const char* nm )
{
    ColTab::Sequence seq( nm );
    setColTabSequence( seq );
}


void uiColTabItem::setColTabSequence( const ColTab::Sequence& ctseq )
{
    ctseq_ = ctseq;
    uiPixmap pm( setup_.sz_.hNrPics(), setup_.sz_.vNrPics() );
    pm.fill( ctseq_, setup_.hor_ );
    ctseqitm_->setPixmap( pm );;
}


void uiColTabItem::setColTabMapperSetup( const ColTab::MapperSetup& ms )
{
    Interval<float> rg = ms.range_;
    minvalitm_->setPlainText( toUiString(rg.start) );
    maxvalitm_->setPlainText( toUiString(rg.stop) );
}
