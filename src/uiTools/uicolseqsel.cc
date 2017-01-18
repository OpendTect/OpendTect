/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2017
________________________________________________________________________

-*/

#include "uicolseqsel.h"

#include "coltabsequence.h"
#include "settings.h"

#include "uicoltabman.h"


uiColorSeqDisp::uiColorSeqDisp( uiParent* p )
    : uiRGBArrayCanvas(p,mkRGBArr())
    , flipped_(false)
    , seqnm_(ColTab::Sequence::sDefaultName())
    , selReq(this)
    , menuReq(this)
{
    disableImageSave();
    disableScrollZoom();
    setDragMode( uiGraphicsView::NoDrag );

    nmitm_ = scene().addItem(
	     new uiTextItem(uiString::sEmptyString(),mAlignment(HCenter,Top)) );

    mAttachCB( postFinalize(), uiColorSeqDisp::initCB );
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

    const int height = rgbarr_->getSize( false );
    const int ctabstop = height / 2;

    const int width = rgbarr_->getSize( true );
    const ColTab::IndexedLookUpTable indextable( *colseq, width );
    for ( int ix=0; ix<width; ix++ )
    {
	const int colidx = flipseq_ ? width-ix-1 : ix;
	const Color color = indextable.colorForIndex( colidx );
	for ( int iy=0; iy<ctabstop; iy++ )
	    rgbarr_->set( ix, iy, color );
    }

    uiPixmap pixmap( width, height );
    pixmap.convertFromRGBArray( *rgbarr_ );
    setPixmap( pixmap );
    updatePixmap();

    nmitm_->setText( toUiString(seqnm_) );
    nmitm_->setPos( uiPoint(width/2,ctabstop) );
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

    const MouseEvent& event = keh.event();
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



uiColorSeqSel::uiColorSeqSel( uiParent* p, const uiString& lbl )
    : uiGroup(p,"Color Seq Selector")
    , usebasicmenu_(true)
    , mandlg_(0)
    , seqChanged(this)
{
    disp_ = new uiColorSeqDisp( this );

    if ( !lbl.isEmpty() )
	new uiLabel( this, lbl, disp_ );

    mAttachCB( postFinalize(), uiColorSeqSel::initDisp );
}


const char* uiColorSeqSel::seqName() const
{
    return disp_->seqName();
}


void uiColorSeqSel::setSeqName( const char* nm ) const
{
    disp_->setSeqName( nm );
}



void uiColorSeqSel::initDisp( CallBacker* )
{
    mAttachCB( disp_->selReq, uiColorSeqDisp::selectCB );
    mAttachCB( disp_->menuReq, uiColorSeqDisp::menuCB );
    mAttachCB( disp_->upReq, uiColorSeqDisp::upCB );
    mAttachCB( disp_->downReq, uiColorSeqDisp::downCB );
}


void uiColorSeqDisp::selectCB( CallBacker* )
{
    uiMSG().error( mTODONotImplPhrase() );
}


uiMenu* uiColorSeqDisp::getBasicMenu()
{
    uiMenu* mnu = new uiMenu( this, uiStrings::sAction() );

    mnu->insertItem( new uiAction(tr("Set as default"),
	mCB(this,uiColorSeqDisp,setAsDefaultCB)), 0 );
    mnu->insertItem( new uiAction(m3Dots(tr("Manage")),
	mCB(this,uiColorSeqDisp,manageCB)), 1 );
}


void uiColorSeqDisp::menuCB( CallBacker* )
{
    if ( !usebasicmenu_ )
	menuReq.trigger();
    else
    {
        PtrMan<uiMenu> mnu = getBasicMenu();
	mnu->exec();
    }
}


void uiColorSeqDisp::setAsDefaultCB( CallBacker* )
{
    setCurrentAsDefault();
}


void uiColorSeqDisp::manageCB( CallBacker* )
{
    showManageDlg();
}


void uiColorSeqDisp::setCurrentAsDefault()
{
    mSettUse( set, "dTect.Color table.Name", "", seqName() );
    Settings::common().write();
}


void uiColorSeqDisp::showManageDlg()
{
    if ( !mandlg_ )
    {
	//TODO find out why this flag exists
	const bool enabletransparancy = true;
	mandlg_ = new uiColorTableMan( this, seqName(), enabletransparancy );
	mAttachCB( mandlg_->tableChanged, uiColorSeqDisp::seqChgCB );
    }

    mandlg_->show();
    mandlg_->raise();
}


void uiColorSeqDisp::nextColTab( bool prev )
{
    const int curidx = ColTab::SM().indexOf( seqName() );
    if ( (prev && curidx < 1) || (!prev && curidx>=ColTab::SM().size()-1) )
	return;

    const int newidx = prev ? curidx-1 : curidx+1;
    setSeqName( ColTab::SM().get(newidx)->name() );
}
