/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2017
________________________________________________________________________

-*/

#include "uicolseqdisp.h"

#include "coltabseqmgr.h"
#include "coltabmapper.h"

#include "uipixmap.h"
#include "uirgbarray.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"


void ColTab::fillRGBArray( uiRGBArray& rgbarr, const Sequence& seq,
			   const Mapper* mpr, OD::Orientation orient,
			   int bordernrpix )
{
    if ( bordernrpix )
	rgbarr.clear( seq.undefColor() );

    const int szx = rgbarr.getSize( true );
    const int szy = rgbarr.getSize( false );
    const int stopxpix = szx - bordernrpix;
    const int stopypix = szy - bordernrpix;

    if ( !mpr )
	mpr = new Mapper;
    const ColTab::Table table( seq, orient==OD::Horizontal ? szx : szy, *mpr );

    if ( orient == OD::Horizontal )
    {
	for ( int ix=bordernrpix; ix<stopxpix; ix++ )
	{
	    const Color color = table.color( ix );
	    for ( int iy=bordernrpix; iy<stopypix; iy++ )
		rgbarr.set( ix, iy, color );
	}
    }
    else
    {
	for ( int iy=bordernrpix; iy<stopypix; iy++ )
	{
	    const Color color = table.color( iy );
	    for ( int ix=bordernrpix; ix<stopxpix; ix++ )
		rgbarr.set( ix, iy, color );
	}
    }
}


uiPixmap* ColTab::getuiPixmap( const Sequence& seq, int szx, int szy,
			       const Mapper* mpr, OD::Orientation orient,
			       int bordernrpix )
{
    if ( szx < 1 || szy < 1 )
	return 0;

    uiPixmap* pm = new uiPixmap( szx, szy );
    pm->setSource( "[colortable]" );
    if ( seq.isEmpty() )
	{ pm->fill( seq.undefColor() ); return pm; }

    uiRGBArray rgbarr( false );
    rgbarr.setSize( szx, szy );

    fillRGBArray( rgbarr, seq, mpr, orient, bordernrpix );

    pm->convertFromRGBArray( rgbarr );
    return pm;
}


uiColSeqDisp::uiColSeqDisp( uiParent* p, OD::Orientation orient, bool wucd )
    : uiRGBArrayCanvas(p,mkRGBArr())
    , orientation_(orient)
    , withudfcoldisp_(wucd)
    , colseq_(ColTab::SeqMGR().getDefault())
    , mapper_(new ColTab::Mapper())
    , selReq(this)
    , menuReq(this)
    , upReq(this)
    , downReq(this)
{
    disableImageSave();
    disableScrollZoom();
    setDragMode( uiGraphicsView::NoDrag );

    setDrawArr( true );
    setStretch( 1, 0 );
    scene().useBackgroundPattern( true );

    setViewHeight( orient == OD::Vertical ? 160 : 25 );
    setViewWidth( orient == OD::Vertical ? 30 : 80 );

    mAttachCB( postFinalise(), uiColSeqDisp::initCB );
}


uiRGBArray& uiColSeqDisp::mkRGBArr()
{
    rgbarr_ = new uiRGBArray( false );
    return *rgbarr_;
}


uiColSeqDisp::~uiColSeqDisp()
{
    detachAllNotifiers();
    delete rgbarr_;
}


const char* uiColSeqDisp::seqName() const
{
    return colseq_ ? colseq_->name() : "";
}


bool uiColSeqDisp::setNewSeq( const Sequence& newseq )
{
    return replaceMonitoredRef( colseq_, newseq, this );
}


void uiColSeqDisp::setSeqName( const char* nm )
{
    if ( !nm || !*nm || FixedString(nm) == seqName() )
	return;
    ConstRefMan<Sequence> newseq = ColTab::SeqMGR().getByName( nm );
    if ( !newseq )
	return;

    if ( setNewSeq(*newseq) )
	reDraw();
}


void uiColSeqDisp::setSequence( const Sequence& seq )
{
    if ( setNewSeq(seq) )
	reDraw();
}


void uiColSeqDisp::setMapper( const Mapper* mpr )
{
    if ( !mpr )
	mpr = new Mapper;

    replaceMonitoredRef( mapper_, mpr, this );
    reDraw();
}


void uiColSeqDisp::setOrientation( OD::Orientation orient )
{
    if ( orientation_ != orient )
    {
	orientation_ = orient;
	reDraw();
    }
}


void uiColSeqDisp::initCB( CallBacker* )
{
    MouseEventHandler& meh = getMouseEventHandler();
    mAttachCB( meh.buttonPressed, uiColSeqDisp::mousePressCB );
    mAttachCB( meh.buttonReleased, uiColSeqDisp::mouseReleaseCB );
    mAttachCB( meh.wheelMove, uiColSeqDisp::mouseWheelCB );

    KeyboardEventHandler& keh = getKeyboardEventHandler();
    mAttachCB( keh.keyReleased, uiColSeqDisp::keybCB );

    mAttachCB( colseq_->objectChanged(), uiColSeqDisp::seqChgCB );
    mAttachCB( mapper_->objectChanged(), uiColSeqDisp::mapperChgCB );
    mAttachCB( reSize, uiColSeqDisp::reSizeCB );

    reSizeCB(0);
}



void uiColSeqDisp::reSizeCB( CallBacker* )
{
    rgbarr_->setSize( scene().nrPixX(), scene().nrPixY() );
    reDraw();
}


void uiColSeqDisp::reDraw()
{
    if ( !finalised() || !colseq_ )
	return;

    beforeDraw();

    ColTab::fillRGBArray( *rgbarr_, *colseq_, mapper_, orientation_,
			  withudfcoldisp_ ? 4 : 0 );
    uiPixmap pixmap( rgbarr_->getSize(true), rgbarr_->getSize(false) );
    pixmap.convertFromRGBArray( *rgbarr_ );
    setPixmap( pixmap );
    updatePixmap();
}


int uiColSeqDisp::longSz() const
{
    return orientation_ == OD::Horizontal ? scene().nrPixX() : scene().nrPixY();
}


int uiColSeqDisp::pixFor( ValueType val ) const
{
    const float relpos = mapper_->relPosition( val );
    return mCast(int,relpos * longSz());
}


ColTab::PosType uiColSeqDisp::relPosFor( const MouseEvent& ev ) const
{
    const int ipos = orientation_ == OD::Horizontal ? ev.x() : ev.y();
    return ((float)ipos) / longSz();
}


#define mMouseTrigger(trig) \
    { \
	if ( !trig.isEmpty() ) \
	{ \
	    trig.trigger( relPosFor(event) ); \
	    meh.setHandled( true ); \
	} \
    }

void uiColSeqDisp::handleMouseBut( bool ispressed )
{
    MouseEventHandler& meh = getMouseEventHandler();
    if ( meh.isHandled() )
	return;
    const MouseEvent& event = meh.event();
    if ( event.isWithKey() )
	return;

    if ( event.leftButton() && !ispressed )
	mMouseTrigger( selReq )
    else if ( event.rightButton() && ispressed )
	mMouseTrigger( menuReq )
}


void uiColSeqDisp::mouseWheelCB( CallBacker* )
{
    MouseEventHandler& meh = getMouseEventHandler();
    if ( meh.isHandled() )
	return;
    const MouseEvent& event = meh.event();
    if ( event.isWithKey() )
	return;

    //TODO how does the wheel stuff work?
    // if ( wheel-up )
	// mMouseTrigger( upReq )
    // else if ( wheel-down )
	// mMouseTrigger( downReq )
}


void uiColSeqDisp::keybCB( CallBacker* )
{
    KeyboardEventHandler& keh = getKeyboardEventHandler();
    if ( keh.isHandled() )
	return;

    const KeyboardEvent& event = keh.event();
    if ( event.modifier_ != OD::NoButton )
	return;

    if ( event.key_ == OD::KB_Enter || event.key_ == OD::KB_Return )
	selReq.trigger( mUdf(float) );
    else if ( event.key_ == OD::KB_Space )
	menuReq.trigger( mUdf(float) );
    else if ( event.key_ == OD::KB_Up || event.key_ == OD::KB_PageUp )
	upReq.trigger( mUdf(float) );
    else if ( event.key_ == OD::KB_Down || event.key_ == OD::KB_PageDown )
	downReq.trigger( mUdf(float) );
}


bool uiColSeqDisp::handleLongTabletPress()
{
    const Geom::Point2D<int> pos = TabletInfo::currentState()->globalpos_;
    MouseEvent me( OD::RightButton, pos.x_, pos.y_ );
    const int refnr = beginCmdRecEvent( "rightButtonPressed" );
    getMouseEventHandler().triggerButtonPressed( me );
    endCmdRecEvent( refnr, "rightButtonPressed" );
    return true;
}
