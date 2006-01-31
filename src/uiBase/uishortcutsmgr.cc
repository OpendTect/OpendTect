/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          20/01/2006
________________________________________________________________________

-*/


#include "uihandleshortcuts.h"
#include "ptrman.h"
#include "oddirs.h"
#include "keyenum.h"
#include <qevent.h>


DefineEnumNames(uiHandleShortcuts,SCLabels,0,"Shortcuts Labels")
{
	"Move slice forward",
	"Move slice backward",
	0
};

bool uiHandleShortcuts::handleEvent( QKeyEvent* event, SCLabels& sclabel )
{
    int scidx = getShortcutIdx( event );
    if ( scidx == -1 )
	return false;

    sclabel = (SCLabels)scidx;
    return true;
}


int uiHandleShortcuts::getShortcutIdx( QKeyEvent* event )
{
    EventKeyAndState ev( event );
    return indexOf( SCList().getList(), ev );
}


//---------------------------------------------------------------------------
ShortcutsList& SCList()
{
    static PtrMan<ShortcutsList> sclist = 0;

    if ( !sclist ) 
	sclist = new ShortcutsList();
	
    return *sclist;
}


ShortcutsList::ShortcutsList()
{
    init();
}


void ShortcutsList::init()
{
    IOPar pars = readShorcutsFile();
    int index = 0;
    while ( true )
    {
	BufferString val1, val2;
	if ( !getKeyValues( pars, index, val1, val2 ) )
	    return;

	EventKeyAndState* ev = new EventKeyAndState( val1, val2 );
	list_ += ev;
	index++;
    }
}


bool ShortcutsList::getKeyValues( const IOPar& pars, int scutidx, 
				  BufferString& val1, BufferString& val2 )
{
    BufferString key = IOPar::compKey( uiHandleShortcuts::keyStr(), scutidx );
    const bool found = pars.get( key.buf(), val1, val2 );
    return found;
}


IOPar ShortcutsList::readShorcutsFile()
{
    IOPar pars("ShortCuts Table");
    pars.read( GetDataFileName("ShortCuts") );
    return pars;
}

//---------------------------------------------------------------------------
EventKeyAndState::EventKeyAndState( QKeyEvent* event )
	: state_( event->state() )
	, key_( event->key() ) 
{
    if ( mIsUdf(state_) )
	state_ = -1;
    if ( mIsUdf(key_) )
	key_ = -1;
};


EventKeyAndState::EventKeyAndState( const char* str1, const char* str2 )
{
    if ( !strcmp( str1, "ShiftButton" ) )
	state_ = OD::ShiftButton;
    else if ( !strcmp( str1, "ControlButton" ) )
	state_ = OD::ControlButton;
    else if ( !strcmp( str1, "NoButton" ) )
	state_ = OD::NoButton;

    if ( !strcmp( str2, "A" ) )
	key_ = OD::A;
    else if ( !strcmp( str2, "B" ) )
	key_ = OD::B;
    else if ( !strcmp( str2, "C" ) )
	key_ = OD::C;
    else if ( !strcmp( str2, "D" ) )
	key_ = OD::D;
    else if ( !strcmp( str2, "E" ) )
	key_ = OD::E;
    else if ( !strcmp( str2, "F" ) )
	key_ = OD::F;
    else if ( !strcmp( str2, "G" ) )
	key_ = OD::G;
    else if ( !strcmp( str2, "H" ) )
	key_ = OD::H;
    else if ( !strcmp( str2, "I" ) )
	key_ = OD::I;
    else if ( !strcmp( str2, "J" ) )
	key_ = OD::J;
    else if ( !strcmp( str2, "K" ) )
	key_ = OD::K;
    else if ( !strcmp( str2, "L" ) )
	key_ = OD::L;
    else if ( !strcmp( str2, "M" ) )
	key_ = OD::M;
    else if ( !strcmp( str2, "N" ) )
	key_ = OD::N;
    else if ( !strcmp( str2, "O" ) )
	key_ = OD::O;
    else if ( !strcmp( str2, "P" ) )
	key_ = OD::P;
    else if ( !strcmp( str2, "Q" ) )
	key_ = OD::Q;
    else if ( !strcmp( str2, "R" ) )
	key_ = OD::R;
    else if ( !strcmp( str2, "S" ) )
	key_ = OD::S;
    else if ( !strcmp( str2, "T" ) )
	key_ = OD::T;
    else if ( !strcmp( str2, "U" ) )
	key_ = OD::U;
    else if ( !strcmp( str2, "V" ) )
	key_ = OD::V;
    else if ( !strcmp( str2, "W" ) )
	key_ = OD::W;
    else if ( !strcmp( str2, "X" ) )
	key_ = OD::X;
    else if ( !strcmp( str2, "Y" ) )
	key_ = OD::Y;
    else if ( !strcmp( str2, "Z" ) )
	key_ = OD::Z;
    else if ( !strcmp( str2, "Plus" ) )
	key_ = OD::Plus;
    else if ( !strcmp( str2, "Minus" ) )
	key_ = OD::Minus;
    else if ( !strcmp( str2, "Asterisk" ) )
	key_ = OD::Asterisk;
    else if ( !strcmp( str2, "Slash" ) )
	key_ = OD::Slash;
    else if ( !strcmp( str2, "Up" ) )
	key_ = OD::Up;
    else if ( !strcmp( str2, "Down" ) )
	key_ = OD::Down;
    else if ( !strcmp( str2, "Left" ) )
	key_ = OD::Left;
    else if ( !strcmp( str2, "Right" ) )
	key_ = OD::Right;
    else if ( !strcmp( str2, "Delete" ) )
	key_ = OD::Delete;
    else if ( !strcmp( str2, "PageUp" ) )
	key_ = OD::PageUp;
    else if ( !strcmp( str2, "PageDown" ) )
	key_ = OD::PageDown;
}

