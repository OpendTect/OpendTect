/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          20/01/2006
________________________________________________________________________

-*/


#include "uishortcutsmgr.h"
#include "ptrman.h"
#include "oddirs.h"
#include "settings.h"
#include "keyenum.h"
#include "keystrs.h"
#include <qevent.h>

uiShortcutsMgr& SCMgr()
{
    static PtrMan<uiShortcutsMgr> scmgr = 0;

    if ( !scmgr ) 
	scmgr = new uiShortcutsMgr();
	    
    return *scmgr;
}


IOPar* uiShortcutsMgr::readShortcutsFile( const char* subsel )
{
    IOPar tmppars;
    IOPar* iopar;
    bool isdefaultfile = false;
    BufferString firstsc;
    BufferString basekey = IOPar::compKey( sKey::Shortcuts, subsel );

    basekey = IOPar::compKey( basekey, 0 );
    if ( !mSettUse(get,basekey,"Name",firstsc) && 
	 ( !mSettUse(get,"Shortcuts.0","Name",firstsc)//compat with old files
	   && !strcmp( subsel, uiShortcutsList::ODSceneStr() ) ) )
    {
	tmppars.read( GetDataFileName(sKey::Shortcuts), 0 );
	isdefaultfile = true;
    }
    else
	tmppars = Settings::common();

    if ( isdefaultfile )
	iopar = tmppars.subselect( subsel );
    else
    {
	iopar = tmppars.subselect( IOPar::compKey( sKey::Shortcuts,subsel ) );

	if ( !iopar && !strcmp( subsel, uiShortcutsList::ODSceneStr() ) )
	    iopar = tmppars.subselect( sKey::Shortcuts );
    }

    return iopar;
}


uiShortcutsList* uiShortcutsMgr::getList( const char* subsel )
{
    static PtrMan<uiShortcutsList> scodscenelist = 0;
    //if some shortcuts are added to other places, like 2D viewer for instance,
    //just create another case here. example:
    //static PtrMan<ShortcutsList> sc2dviewerlist = 0;
   
    if ( !strcmp( subsel, uiShortcutsList::ODSceneStr() ) )
    {
        if ( !scodscenelist )
	    scodscenelist = new uiShortcutsList( uiShortcutsList::ODSceneStr());

	return scodscenelist;
    }

    return 0;
}


uiShortcutsList::uiShortcutsList( const char* elementsel )
{
    init( elementsel );
}


void uiShortcutsList::init( const char* elementsel )
{
    deepErase( keyslist_ );
    nameslist_.deepErase();
    PtrMan<IOPar> pars = SCMgr().readShortcutsFile( elementsel );
    if ( !pars ) return;
    
    int index = 0;
    while ( true )
    {
	BufferString name;
	if ( !getSCNames( *pars, index, name ) )
	    return;
	
	BufferString val1, val2;
	if ( !getKeyValues( *pars, index, val1, val2 ) )
	    return;

	nameslist_.add( name );
	uiKeyDesc* ev = new uiKeyDesc( val1, val2 );
	keyslist_ += ev;
	index++;
    }
}


bool uiShortcutsList::getKeyValues( const IOPar& par, int scutidx,
				    BufferString& val1, BufferString& val2 )
{
    BufferString scutidxstr = scutidx;
    BufferString key = IOPar::compKey( scutidxstr.buf(), sKey::Keys );
    return par.get( key.buf(), val1, val2 );
}


bool uiShortcutsList::getSCNames( const IOPar& par, int scutidx,
				  BufferString& name ) 
{
    BufferString scutidxstr = scutidx;
    BufferString key = IOPar::compKey( scutidxstr.buf(), sKey::Name );
    return par.get( key.buf(), name );
}


const char* uiShortcutsList::getAct( QKeyEvent* event )
{
    uiKeyDesc kd( event );
    if ( kd.state() >= (int)OD::Keypad )
	kd.setState( kd.state() - (int)OD::Keypad );
    
    const int index = indexOf( keyslist_, kd );
    return index>-1 ? nameslist_.get(index).buf() : 0;
}

//---------------------------------------------------------------------------
#define mQtbaseforA 0x41
#define mQtbasefor0 0x30



uiKeyDesc::uiKeyDesc( QKeyEvent* event )
	: state_( event->state() )
	, key_( event->key() )
   	, qkeyev_( event )
   	, qkeyevmine_( false ) 
{
    set( event );
}
 

uiKeyDesc::uiKeyDesc( const char* str1, const char* str2 )
    : state_(0)
    , key_(0)
    , qkeyev_(0)
    , qkeyevmine_( false )
{
    set( str1, str2 );
}


uiKeyDesc::~uiKeyDesc()
{
    if ( qkeyev_ && qkeyevmine_ ) delete qkeyev_;
}


bool uiKeyDesc::set( QKeyEvent* event )
{
    bool isudf;
    if ( mIsUdf(state_) )
    {
	state_ = 0;
	isudf = true;
    }
    if ( mIsUdf(key_) )
    {
	key_ = 0;
	isudf = true;
    }

    return !isudf;
}


bool uiKeyDesc::set( const char* str1, const char* str2 )
{
    if ( *str1 == 'S' || *str1 == 's' )
	state_ = OD::ShiftButton;
    if ( *str1 == 'C' || *str1 == 'c' )
	state_ = OD::ControlButton;
    // Not considering 'Alt' button because that's most often an OS-related key

    if ( *str2 == '\0' )
	return false;

    int keypressed = 0;
    if ( str2[1] == '\0' )
    {
	char charfound = *str2;
	if ( (charfound >= 'a' && charfound <= 'z') )
	    key_ = mQtbaseforA + charfound - 'a';
	else if ( (charfound >= 'A' && charfound <= 'Z') )
	    key_ = mQtbaseforA + charfound - 'A';
	else if ( charfound >= '0' && charfound <= '9' )
	    key_ = mQtbasefor0 + charfound - '0';
    }
    else
	handleSpecialKey(str2);

    if ( qkeyev_ && qkeyevmine_ ) delete qkeyev_;
    
    if ( key_!=0 && state_!=0 )
	qkeyev_ = new QKeyEvent( QEvent::KeyPress, key_, 0, state_ );
    else
	qkeyev_ = new QKeyEvent( QEvent::KeyPress, 0, 0, 0 );
    
    qkeyevmine_ = true;
    return key_ != 0;
}


void uiKeyDesc::handleSpecialKey( const char* str )
{
    if ( !strcmp( str, "Plus" ) )
	key_ = 0x2b;
    else if ( !strcmp( str, "Minus" ) )
	key_ = 0x2d;
    else if ( !strcmp( str, "Asterisk" ) )
	key_ = 0x2a;
    else if ( !strcmp( str, "Slash" ) )
	key_ = 0x2f;
    else if ( !strcmp( str, "Delete" ) )
	key_ = 0x1007;
    else if ( !strcmp( str, "Left" ) )
	key_ = 0x1012;
    else if ( !strcmp( str, "Up" ) )
	key_ = 0x1013;
    else if ( !strcmp( str, "Right" ) )
	key_ = 0x1014;
    else if ( !strcmp( str, "Down" ) )
	key_ = 0x1015;
    else if ( !strcmp( str, "PageUp" ) )
	key_ = 0x1016;
    else if ( !strcmp( str, "PageDown" ) )
	key_ = 0x1017;
}


bool uiKeyDesc::isSimpleAscii() const
{
    return state_== 0 && isascii( *(qkeyev_->text()) );
}


char uiKeyDesc::asciiChar() const
{
    if ( !isascii(key_) ) return 0;
    return qkeyev_->ascii();
}


void uiKeyDesc::setState( int state )
{
    state_  = state;
    if ( qkeyev_ && qkeyevmine_ ) delete qkeyev_;

    qkeyev_ = new QKeyEvent( QEvent::KeyPress, key_, 0, state_ );
    qkeyevmine_ = true;
}
