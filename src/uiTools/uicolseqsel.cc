/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2017
________________________________________________________________________

-*/

#include "uicolseqdisp.h"
#include "uicolseqsel.h"

#include "settings.h"
#include "coltabindex.h"
#include "coltabsequence.h"

#include "uimsg.h"
#include "uimenu.h"
#include "uilabel.h"
#include "uipixmap.h"
#include "uirgbarray.h"
#include "uicoltabman.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"


uiColorSeqDisp::uiColorSeqDisp( uiParent* p )
    : uiRGBArrayCanvas(p,mkRGBArr())
    , flipped_(false)
    , seqnm_(ColTab::Sequence::sDefaultName())
    , selReq(this)
    , menuReq(this)
    , upReq(this)
    , downReq(this)
{
    disableImageSave();
    disableScrollZoom();
    setDragMode( uiGraphicsView::NoDrag );

    nmitm_ = scene().addItem(
	     new uiTextItem(uiString::emptyString(),mAlignment(HCenter,Top)) );

    mAttachCB( postFinalise(), uiColorSeqDisp::initCB );
}


uiRGBArray& uiColorSeqDisp::mkRGBArr()
{
    rgbarr_ = new uiRGBArray( false );
    return *rgbarr_;
}


uiColorSeqDisp::~uiColorSeqDisp()
{
    delete rgbarr_;
}


void uiColorSeqDisp::setSeqName( const char* nm )
{
    if ( ColTab::SM().indexOf(nm) < 0 )
	return;
    seqnm_ = nm;
    reDraw();
}


void uiColorSeqDisp::setFlipped( bool yn )
{
    flipped_ = yn;
    reDraw();
}


void uiColorSeqDisp::initCB( CallBacker* )
{
    reDraw();

    MouseEventHandler& meh = getMouseEventHandler();
    mAttachCB( meh.buttonPressed, uiColorSeqDisp::mouseButCB );
    mAttachCB( meh.buttonReleased, uiColorSeqDisp::mouseButCB );
    mAttachCB( meh.wheelMove, uiColorSeqDisp::mouseWheelCB );

    KeyboardEventHandler& keh = getKeyboardEventHandler();
    mAttachCB( keh.keyReleased, uiColorSeqDisp::keybCB );

    mAttachCB( reSize, uiColorSeqDisp::reSizeCB );
}


void uiColorSeqDisp::reSizeCB( CallBacker* )
{
    rgbarr_->setSize( (int)(scene().width()+.5), (int)(scene().height()+.5) );
    reDraw();
}


void uiColorSeqDisp::reDraw()
{
    if ( seqnm_.isEmpty() )
	return;
    const ColTab::Sequence* colseq = ColTab::SM().getByName( seqnm_ );
    if ( !colseq )
	return;

    beforeDraw();
    rgbarr_->clear( Color::White() );

    const int arrh = rgbarr_->getSize( false );
    const int ctabstop = arrh / 2;

    const int arrw = rgbarr_->getSize( true );
    const ColTab::IndexedLookUpTable indextable( *colseq, arrw );
    for ( int ix=0; ix<arrw; ix++ )
    {
	const int colidx = flipped_ ? arrw-ix-1 : ix;
	const Color color = indextable.colorForIndex( colidx );
	for ( int iy=0; iy<ctabstop; iy++ )
	    rgbarr_->set( ix, iy, color );
    }

    uiPixmap pixmap( arrw, arrh );
    pixmap.convertFromRGBArray( *rgbarr_ );
    setPixmap( pixmap );
    updatePixmap();

    nmitm_->setText( toUiString(seqnm_) );
    nmitm_->setPos( uiPoint(arrw/2,ctabstop) );
}


#define mMouseTrigger(trig) \
    { \
	if ( !trig.isEmpty() ) \
	{ \
	    trig.trigger(); \
	    meh.setHandled( true ); \
	} \
    }

void uiColorSeqDisp::mouseButCB( CallBacker* )
{
    MouseEventHandler& meh = getMouseEventHandler();
    if ( meh.isHandled() )
	return;
    const MouseEvent& event = meh.event();
    if ( event.isWithKey() )
	return;

    if ( event.leftButton() && !event.isPressed() )
	mMouseTrigger( selReq )
    else if ( event.rightButton() && event.isPressed() )
	mMouseTrigger( menuReq )

}


void uiColorSeqDisp::mouseWheelCB( CallBacker* )
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


void uiColorSeqDisp::keybCB( CallBacker* )
{
    KeyboardEventHandler& keh = getKeyboardEventHandler();
    if ( keh.isHandled() )
	return;

    const KeyboardEvent& event = keh.event();
    if ( event.modifier_ != OD::NoButton )
	return;

    if ( event.key_ == OD::KB_Enter || event.key_ == OD::KB_Return )
	selReq.trigger();
    else if ( event.key_ == OD::KB_Space )
	menuReq.trigger();
    else if ( event.key_ == OD::KB_Up || event.key_ == OD::KB_PageUp )
	upReq.trigger();
    else if ( event.key_ == OD::KB_Down || event.key_ == OD::KB_PageDown )
	downReq.trigger();
}



uiColorSeqSel::uiColorSeqSel( uiParent* p, uiString lbl )
    : uiGroup(p,"Color Seq Selector")
    , usebasicmenu_(true)
    , mandlg_(0)
    , seqChanged(this)
    , menuReq(this)
{
    disp_ = new uiColorSeqDisp( this );

    if ( !lbl.isEmpty() )
	new uiLabel( this, lbl, disp_ );

    mAttachCB( postFinalise(), uiColorSeqSel::initDisp );
}


uiColorSeqSel::~uiColorSeqSel()
{
}


const char* uiColorSeqSel::seqName() const
{
    return disp_->seqName();
}


void uiColorSeqSel::setSeqName( const char* nm )
{
    disp_->setSeqName( nm );
}


bool uiColorSeqSel::isFlipped() const
{
    return disp_->isFlipped();
}


void uiColorSeqSel::setFlipped( bool yn )
{
    disp_->setFlipped( yn );
}



void uiColorSeqSel::initDisp( CallBacker* )
{
    mAttachCB( disp_->selReq, uiColorSeqSel::selectCB );
    mAttachCB( disp_->menuReq, uiColorSeqSel::menuCB );
    mAttachCB( disp_->upReq, uiColorSeqSel::upCB );
    mAttachCB( disp_->downReq, uiColorSeqSel::downCB );
}


void uiColorSeqSel::selectCB( CallBacker* )
{
    uiMSG().error( mTODONotImplPhrase() );
}


uiMenu* uiColorSeqSel::getBasicMenu()
{
    uiMenu* mnu = new uiMenu( this, uiStrings::sAction() );

    mnu->insertItem( new uiAction(tr("Set as default"),
	mCB(this,uiColorSeqSel,setAsDefaultCB)), 0 );
    mnu->insertItem( new uiAction(m3Dots(tr("Manage")),
	mCB(this,uiColorSeqSel,manageCB)), 1 );

    return mnu;
}


void uiColorSeqSel::menuCB( CallBacker* )
{
    if ( !usebasicmenu_ )
	menuReq.trigger();
    else
    {
        PtrMan<uiMenu> mnu = getBasicMenu();
	mnu->exec();
    }
}


void uiColorSeqSel::setAsDefaultCB( CallBacker* )
{
    setCurrentAsDefault();
}


void uiColorSeqSel::manageCB( CallBacker* )
{
    showManageDlg();
}


void uiColorSeqSel::setCurrentAsDefault()
{
    mSettUse( set, "dTect.Color table.Name", "", seqName() );
    Settings::common().write();
}


void uiColorSeqSel::showManageDlg()
{
	/*
    if ( !mandlg_ )
    {
	//TODO find out why this flag exists
	const bool enabletransparancy = true;
	mandlg_ = new uiColorTableMan( this, seqName(), enabletransparancy );
	mAttachCB( mandlg_->tableChanged, uiColorSeqDisp::seqChgCB );
    }

    mandlg_->show();
    mandlg_->raise();
	*/
}


void uiColorSeqSel::nextColTab( bool prev )
{
    const int curidx = ColTab::SM().indexOf( seqName() );
    if ( (prev && curidx < 1) || (!prev && curidx>=ColTab::SM().size()-1) )
	return;

    const int newidx = prev ? curidx-1 : curidx+1;
    setSeqName( ColTab::SM().get(newidx)->name() );
}
