/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Jaap Glas
 Date:          October 2010
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uivirtualkeyboard.h"

#include "pixmap.h"
#include "oddirs.h"
#include "uicombobox.h"
#include "uigraphicsscene.h"
#include "uigraphicsviewbase.h"
#include "uigraphicsitemimpl.h"
#include "uilineedit.h"
#include "uispinbox.h"
#include "uitable.h"


static int nractivevirtualkeyboards_ = 0;

bool uiVirtualKeyboard::isVirtualKeyboardActive()
{ return nractivevirtualkeyboards_; }


uiVirtualKeyboard::uiVirtualKeyboard( uiObject& inpobj, int x, int y )
    : uiMainWin( inpobj.parent(), "Virtual Keyboard", 0, false, true )
    , inputobj_( inpobj )
    , globalx_( x )
    , globaly_( y )
    , keyboardscale_( 1.0 )
    , enterpressed_( false )
    , capslock_( false )
    , shiftlock_( false )
    , ctrllock_( false )
    , altlock_( false )
    , shift_( false )
    , ctrl_( false )
    , alt_( false )
    , selectionstart_( -1 )
    , selectionlength_( 0 )
    , leds_( new uiGraphicsItemSet() )
{
    nractivevirtualkeyboards_++;

    if ( x>=0 && y>=0 && !mIsUdf(x) && !mIsUdf(y) )
	setCornerPos( x, y );

    BufferString wintitle( "Virtual Keyboard  [" );
    wintitle += inputobj_.name(); wintitle += "]";
    setCaption( wintitle );

    ioPixmap pixmap( mGetSetupFileName("virtualkeyboard") );
    const float keyboardwidth = keyboardscale_ * pixmap.width();
    const float keyboardheight = keyboardscale_ * pixmap.height();

    textline_ = new uiLineEdit( this, "Text line" );
    textline_->setPrefWidth( mNINT32(keyboardwidth) );

    textline_->returnPressed.notify( mCB(this,uiVirtualKeyboard,enterCB) );
    textline_->selectionChanged.notify( mCB(this,uiVirtualKeyboard,selChg) );

    uiPixmapItem* pixmapitm = new uiPixmapItem( pixmap );
    pixmapitm->setScale( keyboardscale_, keyboardscale_ );
    pixmapitm->setZValue( 0 );

    uiGraphicsScene* scene = new uiGraphicsScene( "Virtual keyboard scene" );
    scene->addItem( pixmapitm );

    scene->getMouseEventHandler().buttonReleased.notify(
					mCB(this,uiVirtualKeyboard,clickCB) );

    viewbase_ = new uiGraphicsViewBase( this, "Virtual keyboard view" );
    viewbase_->setScene( *scene );
    viewbase_->attach( alignedBelow, textline_ );
    viewbase_->setPrefWidth( mNINT32(keyboardwidth) );
    viewbase_->setPrefHeight( mNINT32(keyboardheight) );

    addLed(  46,  86, Color(255,0,0) );
    addLed(  62, 118, Color(255,0,0) );
    addLed(  38, 150, Color(255,0,0) );
    addLed( 118, 150, Color(255,0,0) );
    addLed( 470, 118, Color(0,255,0) );
    addLed( 358, 150, Color(0,255,0) );
    addLed( 470, 150, Color(0,255,0) );
    updateLeds();

    windowClosed.notify( mCB(this,uiVirtualKeyboard,exitCB) );
    updateKeyboard();
}


uiVirtualKeyboard::~uiVirtualKeyboard()
{
    viewbase_->scene().removeItems( *leds_ );
    delete leds_;

    viewbase_->scene().getMouseEventHandler().buttonReleased.remove(
					mCB(this,uiVirtualKeyboard,clickCB) );
    nractivevirtualkeyboards_--;
}


bool uiVirtualKeyboard::enterPressed() const
{ return enterpressed_; }


void uiVirtualKeyboard::addLed( float x, float y, const Color& color )
{
    MarkerStyle2D markerstyle( MarkerStyle2D::Circle, mNINT32(4*keyboardscale_) );
    uiPoint point( mNINT32(x*keyboardscale_), mNINT32(y*keyboardscale_) );
    uiMarkerItem* led = new uiMarkerItem( point, markerstyle );
    led->setFillColor( color );
    led->setZValue( 1 );
    leds_->add( led );
    viewbase_->scene().addItem( led );
}


void uiVirtualKeyboard::updateLeds() 
{
    const int sz = leds_->size();

    if ( sz > 0 ) leds_->get( 0 )->setVisible( capslock_ );
    if ( sz > 1 ) leds_->get( 1 )->setVisible( shiftlock_ );
    if ( sz > 2 ) leds_->get( 2 )->setVisible( ctrllock_ );
    if ( sz > 3 ) leds_->get( 3 )->setVisible( altlock_ );
    if ( sz > 4 ) leds_->get( 4 )->setVisible( shift_ );
    if ( sz > 5 ) leds_->get( 5 )->setVisible( alt_ );
    if ( sz > 6 ) leds_->get( 6 )->setVisible( ctrl_ );
}


enum SpecialKeys { CapsLock, ShiftLock, CtrlLock, AltLock, Shift, Alt, Ctrl,
		   Backspace, Del, Tab, Enter, LeftArrow, RightArrow };

static char mousePress2Key( int x, int y, bool caps, bool shift )
{
    if ( y < 32 )
    {
	if ( x <  32 ) return shift ? '~' : '`';
	if ( x <  64 ) return shift ? '!' : '1';
	if ( x <  96 ) return shift ? '@' : '2';
	if ( x < 128 ) return shift ? '#' : '3';
	if ( x < 160 ) return shift ? '$' : '4';
	if ( x < 192 ) return shift ? '%' : '5';
	if ( x < 224 ) return shift ? '^' : '6';
	if ( x < 256 ) return shift ? '&' : '7';
	if ( x < 288 ) return shift ? '*' : '8';
	if ( x < 320 ) return shift ? '(' : '9';
	if ( x < 352 ) return shift ? ')' : '0';
	if ( x < 384 ) return shift ? '_' : '-';
	if ( x < 416 ) return shift ? '+' : '=';
	if ( x < 480 ) return Backspace;
    }
    else if ( y < 64 )
    {
	if ( x <  48 ) return Tab;
	if ( x <  80 ) return caps!=shift ? 'Q' : 'q';
	if ( x < 112 ) return caps!=shift ? 'W' : 'w';
	if ( x < 144 ) return caps!=shift ? 'E' : 'e';
	if ( x < 176 ) return caps!=shift ? 'R' : 'r';
	if ( x < 208 ) return caps!=shift ? 'T' : 't';
	if ( x < 240 ) return caps!=shift ? 'Y' : 'y';
	if ( x < 272 ) return caps!=shift ? 'U' : 'u';
	if ( x < 304 ) return caps!=shift ? 'I' : 'i';
	if ( x < 336 ) return caps!=shift ? 'O' : 'o';
	if ( x < 368 ) return caps!=shift ? 'P' : 'p';
	if ( x < 400 ) return shift ? '{' : '[';
	if ( x < 432 ) return shift ? '}' : ']';
	if ( x < 480 ) return shift ? '|' : '\\';
    }
    else if ( y < 96 )
    {
	if ( x <  56 ) return CapsLock;
	if ( x <  88 ) return caps!=shift ? 'A' : 'a';
	if ( x < 120 ) return caps!=shift ? 'S' : 's';
	if ( x < 152 ) return caps!=shift ? 'D' : 'd';
	if ( x < 184 ) return caps!=shift ? 'F' : 'f';
	if ( x < 216 ) return caps!=shift ? 'G' : 'g';
	if ( x < 248 ) return caps!=shift ? 'H' : 'h';
	if ( x < 280 ) return caps!=shift ? 'J' : 'j';
	if ( x < 312 ) return caps!=shift ? 'K' : 'k';
	if ( x < 344 ) return caps!=shift ? 'L' : 'l';
	if ( x < 376 ) return shift ? ':' : ';';
	if ( x < 408 ) return shift ? '"' : '\'';
	if ( x < 480 ) return Enter;
    }
    else if ( y < 128 )
    {
	if ( x <  72 ) return ShiftLock;
	if ( x < 104 ) return caps!=shift ? 'Z' : 'z';
	if ( x < 136 ) return caps!=shift ? 'X' : 'x';
	if ( x < 168 ) return caps!=shift ? 'C' : 'c';
	if ( x < 200 ) return caps!=shift ? 'V' : 'v';
	if ( x < 232 ) return caps!=shift ? 'B' : 'b';
	if ( x < 264 ) return caps!=shift ? 'N' : 'n';
	if ( x < 296 ) return caps!=shift ? 'M' : 'm';
	if ( x < 328 ) return shift ? '<' : ',';
	if ( x < 360 ) return shift ? '>' : '.';
	if ( x < 392 ) return shift ? '?' : '/';
	if ( x < 480 ) return Shift;
    }
    else
    {
	if ( x <  48 ) return CtrlLock;
	if ( x <  80 ) return LeftArrow;
	if ( x < 128 ) return AltLock;
	if ( x < 320 ) return ' ';
	if ( x < 368 ) return Alt;
	if ( x < 400 ) return RightArrow;
	if ( x < 432 ) return Del;
    }

    return Ctrl;
}


void uiVirtualKeyboard::clickCB( CallBacker* )
{
    const MouseEvent& ev = viewbase_->scene().getMouseEventHandler().event();
    

    const bool shiftstatus = (shiftlock_!=shift_) != ev.rightButton();

    char str[2]; str[1] = '\0';
    str[0] = mousePress2Key( mNINT32( ev.x()/keyboardscale_ ), 
	    		     mNINT32( ev.y()/keyboardscale_ ),
			     capslock_, shiftstatus ); 
    restoreSelection();

    if ( str[0] == CapsLock )
	capslock_ = !capslock_;
    if ( str[0] == ShiftLock )
	shiftlock_ = !shiftlock_;
    if ( str[0] == CtrlLock )
	ctrllock_ = !ctrllock_;
    if ( str[0] == AltLock )
	altlock_ = !altlock_;
    if ( str[0] == Shift )
	shift_ = !shift_;
    if ( str[0] == Alt )
	alt_ = !alt_;
    if ( str[0] == Ctrl )
	ctrl_ = !ctrl_;

    if ( str[0] == Enter )
	enterCB( 0 );
    if ( str[0] == Backspace )
	textline_->backspace();
    if ( str[0] == Del )
	textline_->del();
    if ( str[0] == LeftArrow )
	textline_->cursorBackward( shiftstatus );
    if ( str[0] == RightArrow )
	textline_->cursorForward( shiftstatus );

    if ( str[0] >= ' ' )
	textline_->insert( str );

    if ( str[0]>Ctrl )
    {
	shift_ = false; ctrl_ = false; alt_ = false;
    }

    updateLeds();
}


void uiVirtualKeyboard::exitCB( CallBacker* cb )
{
    inputobj_.setSensitive( true );
    updateInputObj();
}


void uiVirtualKeyboard::updateKeyboard()
{
    inputobj_.setSensitive( false );

    mDynamicCastGet( uiLineEdit*, uilined, &inputobj_ );
    if ( uilined )
	textline_->setvalue_( uilined->text() );

    mDynamicCastGet( uiSpinBox*, uispin, &inputobj_ );
    if ( uispin )
	textline_->setvalue_( uispin->text() );

    mDynamicCastGet( uiComboBox*, uicombo, &inputobj_ );
    if ( uicombo )
	textline_->setvalue_( uicombo->text() );

    mDynamicCastGet( uiTable*, uitable, &inputobj_ );
    if ( uitable )
	textline_->setvalue_( uitable->text(uitable->notifiedCell()) );

    textline_->cursorForward( false, 10000 );
}


void uiVirtualKeyboard::updateInputObj()
{
    mDynamicCastGet( uiLineEdit*, uilined, &inputobj_ );
    if ( uilined )
	uilined->setvalue_( textline_->text() );

    mDynamicCastGet( uiSpinBox*, uispin, &inputobj_ );
    if ( uispin )
	uispin->setValue( textline_->text() );

    mDynamicCastGet( uiComboBox*, uicombo, &inputobj_ );
    if ( uicombo )
	uicombo->setText( textline_->text() );

    mDynamicCastGet( uiTable*, uitable, &inputobj_ );
    if ( uitable )
	uitable->setText( uitable->notifiedCell(), textline_->text() );
}


void uiVirtualKeyboard::enterCB( CallBacker* )
{
    enterpressed_ = true;
    close();
}


void uiVirtualKeyboard::selChg( CallBacker* )
{
    if ( textline_->hasFocus() )
    {
	selectionstart_ = textline_->selectionStart();
	BufferString seltext = textline_->selectedText();
	selectionlength_ = seltext.size();

	if ( selectionstart_ == textline_->cursorPosition() )
	{
	    selectionstart_ += selectionlength_;
	    selectionlength_ *= -1;
	}
    }
}


void uiVirtualKeyboard::restoreSelection()
{
    textline_->setFocus();
    if ( selectionstart_ >= 0 )
	textline_->setSelection( selectionstart_, selectionlength_ );
}
