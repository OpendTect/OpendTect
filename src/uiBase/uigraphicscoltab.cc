/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
                setup_.startal_.vPos() == Alignment::VCenter? rect.centre().y_
	 : (setup_.startal_.vPos() == Alignment::Top	? rect.top()
							: rect.bottom());

	al = Alignment( setup_.startal_.hPos(),
			Alignment::opposite(setup_.startal_.vPos()) );
	minvalitm_->setAlignment( al );
	minvalitm_->setPos( sCast(float,rect.left()), sCast(float,starty) );

	const int stopy =
                setup_.stopal_.vPos() == Alignment::VCenter ? rect.centre().y_
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
                setup_.startal_.hPos() == Alignment::HCenter? rect.centre().x_
	 : (setup_.startal_.hPos() == Alignment::Left	? rect.left()
							: rect.right());
	const int stopx =
                setup_.stopal_.hPos() == Alignment::HCenter ? rect.centre().x_
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
}


void uiColTabItem::setPixmap()
{
    ColTab::Sequence seqcp;
    seqcp = ctseq_;
    if ( ctms_.flipseq_ )
	seqcp.flipColor();

    uiPixmap pm( setup_.sz_.hNrPics(), setup_.sz_.vNrPics() );
    pm.fill( seqcp, setup_.hor_ );
    ctseqitm_->setPixmap( pm );
}


void uiColTabItem::setColTabMapperSetup( const ColTab::MapperSetup& ms )
{
    ctms_ = ms;

    BufferString precision;
    minvalitm_->setPlainText( toUiString(precision.set(ms.range_.start_,2)) );
    maxvalitm_->setPlainText( toUiString(precision.set(ms.range_.stop_,2)) );
}


void uiColTabItem::update()
{
    setPixmap();
    adjustLabel();
}
