/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2017
________________________________________________________________________

-*/

#include "uicolseqdisp.h"

#include "coltabindex.h"
#include "coltabsequence.h"
#include "coltabmapper.h"

#include "uipixmap.h"
#include "uirgbarray.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"


uiColSeqDisp::uiColSeqDisp( uiParent* p, OD::Orientation orient, bool wucd )
    : uiRGBArrayCanvas(p,mkRGBArr())
    , orientation_(orient)
    , withudfcoldisp_(wucd)
    , sequsemode_(ColTab::UnflippedSingle)
    , colseq_(ColTab::SeqMGR().getDefault())
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

    setPrefHeight( orient == OD::Vertical ? 160 : 25 );
    setPrefWidth( orient == OD::Vertical ? 30 : 80 );

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


void uiColSeqDisp::setNewSeq( const Sequence& newseq )
{
    replaceMonitoredRef( colseq_, const_cast<Sequence&>(newseq), this );
}


void uiColSeqDisp::setSeqName( const char* nm )
{
    if ( !nm || !*nm || FixedString(nm) == seqName() )
	return;
    ConstRefMan<Sequence> newseq = ColTab::SeqMGR().getByName( nm );
    if ( !newseq )
	return;

    setNewSeq( *newseq );
    reDraw();
}


void uiColSeqDisp::setSequence( const Sequence& seq )
{
    if ( colseq_.ptr() == &seq )
	return;

    setNewSeq( seq );
    reDraw();
}


void uiColSeqDisp::setSeqUseMode( ColTab::SeqUseMode mode )
{
    if ( sequsemode_ != mode )
    {
	sequsemode_ = mode;
	reDraw();
    }
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
    mAttachCB( reSize, uiColSeqDisp::reSizeCB );
    reDraw();
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

    rgbarr_->clear( colseq_->undefColor() );
    const bool isvert = orientation_ != OD::Horizontal;
    const int arrw = rgbarr_->getSize( true );
    const int arrh = rgbarr_->getSize( false );
    const int shortarrlen = isvert ? arrw : arrh;
    const int longarrlen = isvert ? arrh : arrw;
    const ColTab::IndexedLookUpTable indextable( *colseq_, longarrlen,
						 sequsemode_ );
    const int startshort = withudfcoldisp_ ? shortarrlen / 7 : 0;
    const int stopshort = shortarrlen - startshort;
    const int startlong = 0;
    const int stoplong = longarrlen;
    for ( int ilong=startlong; ilong<stoplong; ilong++ )
    {
	const Color color = indextable.colorForIndex( ilong );
	for ( int ishort=startshort; ishort<stopshort; ishort++ )
	{
	    if ( isvert )
		rgbarr_->set( ishort, ilong, color );
	    else
		rgbarr_->set( ilong, ishort, color );
	}
    }

    uiPixmap pixmap( arrw, arrh );
    pixmap.convertFromRGBArray( *rgbarr_ );
    setPixmap( pixmap );
    updatePixmap();
}


uiColSeqDisp::PosType uiColSeqDisp::seqPosFor( const uiPoint& pt ) const
{
    const float relpos = orientation_ == OD::Horizontal
			? ((float)pt.x_) / ((float)scene().nrPixX())
			: ((float)pt.y_) / ((float)scene().nrPixY());
    return ColTab::Mapper::seqPos4RelPos( sequsemode_, relpos );
}


#define mMouseTrigger(trig) \
    { \
	if ( !trig.isEmpty() ) \
	{ \
	    trig.trigger( seqPosFor(uiPoint(event.x(),event.y())) ); \
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
