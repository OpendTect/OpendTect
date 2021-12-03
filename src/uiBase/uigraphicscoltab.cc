/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2008
________________________________________________________________________

-*/


#include "uigraphicscoltab.h"

#include "uigraphicsitemimpl.h"
#include "uipixmap.h"

#include "coltabmapper.h"
#include "coltabsequence.h"

uiColTabItem::uiColTabItem( const uiColTabItem::Setup& su )
    : uiGraphicsItem()
    , setup_(su)
{
    ctseqitm_ = new uiPixmapItem();
    minvalitm_ = new uiAdvancedTextItem( toUiString("0") );
    maxvalitm_ = new uiAdvancedTextItem( toUiString("1") );
    addChild( ctseqitm_ );
    addChild( minvalitm_ );
    addChild( maxvalitm_ );

    setColTabSequence( ColTab::Sequence("") );
    ctseqitm_->setPos( 0.f, 0.f );
    adjustLabel();
}


uiColTabItem::~uiColTabItem()
{
    removeAll( true );
}


void uiColTabItem::adjustLabel()
{
    const uiRect rect( 0, 0, setup_.sz_.width(), setup_.sz_.height() );
    Alignment al;

    if ( setup_.hor_ )
    {
	const int starty =
	    setup_.startal_.vPos() == Alignment::VCenter? rect.centre().y
	 : (setup_.startal_.vPos() == Alignment::Top	? rect.top()
							: rect.bottom());

	al = Alignment( setup_.startal_.hPos(),
			Alignment::opposite(setup_.startal_.vPos()) );
	minvalitm_->setAlignment( al );
	minvalitm_->setPos( sCast(float,rect.left()), sCast(float,starty) );

	const int stopy =
	    setup_.stopal_.vPos() == Alignment::VCenter ? rect.centre().y
	 : (setup_.stopal_.vPos() == Alignment::Top	? rect.top()
							: rect.bottom());
	al = Alignment( setup_.stopal_.hPos(),
			Alignment::opposite(setup_.stopal_.vPos()) );
	maxvalitm_->setAlignment( al );
	maxvalitm_->setPos( sCast(float,rect.right()), sCast(float,stopy) );
    }
    else
    {
	const int startx =
	    setup_.startal_.hPos() == Alignment::HCenter? rect.centre().x
	 : (setup_.startal_.hPos() == Alignment::Left	? rect.left()
							: rect.right());
	const int stopx =
	    setup_.stopal_.hPos() == Alignment::HCenter ? rect.centre().x
	 : (setup_.stopal_.hPos() == Alignment::Left	? rect.left()
							: rect.right());

	al = Alignment( Alignment::opposite(setup_.startal_.hPos()),
			setup_.startal_.vPos() );
	minvalitm_->setAlignment( al );
	minvalitm_->setPos( sCast(float,startx), sCast(float,rect.bottom()) );

	al = Alignment( Alignment::opposite(setup_.stopal_.hPos()),
			setup_.stopal_.vPos() );
	maxvalitm_->setAlignment( al );
	maxvalitm_->setPos( sCast(float,stopx), sCast(float,rect.top()) );
    }
}


void uiColTabItem::setColTab( const char* nm )
{
    ColTab::Sequence seq( nm );
    setColTabSequence( seq );
}


void uiColTabItem::setColTabSequence( const ColTab::Sequence& ctseq )
{
    ctseq_ = ctseq;
    setPixmap();
}


void uiColTabItem::setPixmap()
{
    uiPixmap pm( setup_.sz_.hNrPics(), setup_.sz_.vNrPics() );
    pm.fill( ctseq_, setup_.hor_ );
    ctseqitm_->setPixmap( pm );
}


void uiColTabItem::setColTabMapperSetup( const ColTab::MapperSetup& ms )
{
    BufferString precision;
    minvalitm_->setPlainText( toUiString(precision.set(ms.range_.start,2)) );
    maxvalitm_->setPlainText( toUiString(precision.set(ms.range_.stop,2)) );
    adjustLabel();
}


void uiColTabItem::setupChanged()
{
    setPixmap();
    adjustLabel();
}
