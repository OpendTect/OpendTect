/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Payraudeau
 Date:          20/01/2006
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uishortcutsmgr.h"
#include "ptrman.h"
#include "oddirs.h"
#include "settings.h"
#include "keyenum.h"
#include "keystrs.h"
#include "strmprov.h"
#include "ascstream.h"
#include "errh.h"

#include <QKeyEvent>


uiShortcutsMgr& SCMgr()
{
    static uiShortcutsMgr* scmgr = 0;
    if ( !scmgr ) scmgr = new uiShortcutsMgr;
    return *scmgr;
}


const char** uiKeyDesc::sKeyKeyStrs()
{
    static const char* strs[] =
{ "A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P","Q","R","S",
  "T","U","V","W","X","Y","Z","Plus","Minus","Asterisk","Slash","Up","Down",
  "Left","Right","Delete","PageUp","PageDown", 0 };
    return strs;
}


const int speckeystransbl[] =
{ 0x2b, 0x2d, 0x2a, 0x2f,
  0x1013, 0x1015, 0x1012, 0x1014, 0x1007, 0x1016, 0x1017 };


#define mQtbaseforA 0x41
#define mQtbasefor0 0x30


uiKeyDesc::uiKeyDesc( mQtclass(QKeyEvent*) ev )
    : key_(ev->key())
{
    state_ = (OD::ButtonState)(int)ev->modifiers();
    if ( state_ >= OD::Keypad )
	state_ = (OD::ButtonState)((int)state_ - (int)OD::Keypad);
}


uiKeyDesc::uiKeyDesc( const char* str1, const char* str2 )
    : state_(OD::NoButton)
    , key_(0)
{
    set( str1, str2 );
}


bool uiKeyDesc::set( const char* statestr, const char* keystr )
{
    if ( !statestr )
	statestr = "";

    if ( *statestr == 'S' || *statestr == 's' )
	state_ = OD::ShiftButton;
    if ( *statestr == 'C' || *statestr == 'c' )
	state_ = OD::ControlButton;
    // Not considering 'Alt' button because that's usually OS-related

    if ( !keystr || *keystr == '\0' )
	return false;

    if ( keystr[1] != '\0' )
    {
	const char** strs = sKeyKeyStrs();
	for ( int idx=26; strs[idx]; idx++ )
	{
	    if ( !strcmp( keystr, strs[idx] ) )
		{ key_ = speckeystransbl[idx-26]; break; }
	}
    }
    else
    {
	char charfound = *keystr;
	if ( (charfound >= 'a' && charfound <= 'z') )
	    key_ = mQtbaseforA + charfound - 'a';
	else if ( (charfound >= 'A' && charfound <= 'Z') )
	    key_ = mQtbaseforA + charfound - 'A';
	else if ( charfound >= '0' && charfound <= '9' )
	    key_ = mQtbasefor0 + charfound - '0';
    }

    return key_ != 0;
}


const char* uiKeyDesc::stateStr() const
{
    return OD::nameOf( state_ );
}


const char* uiKeyDesc::keyStr() const
{
    const char** strs = sKeyKeyStrs();

    if ( key_ >= mQtbaseforA && key_ < mQtbaseforA + 26 )
	return strs[ key_ - mQtbaseforA ];
    if ( key_ >= mQtbasefor0 && key_ < mQtbasefor0 + 10 )
	return strs[ key_ - mQtbasefor0 ];

    for ( int idx=26; ; idx++ )
    {
	if ( speckeystransbl[idx-26] == key_ )
	    return strs[idx];
    }
    return 0;
}


bool uiKeyDesc::isSimpleAscii() const
{
    mQtclass(QKeyEvent) qke( mQtclass(QEvent)::KeyPress, key_,
	    		     (mQtclass(Qt)::KeyboardModifiers)state_ );
    mQtclass(QString) txt = qke.text();
    return state_== 0 && !txt.isEmpty() && txt[0].isLetterOrNumber();
}


char uiKeyDesc::asciiChar() const
{
    if ( !isascii(key_) ) return 0;

    mQtclass(QKeyEvent) qke( mQtclass(QEvent)::KeyPress, key_,
	    		     (mQtclass(Qt)::KeyboardModifiers)state_ );
    mQtclass(QString) txt = qke.text();
    return !txt.isEmpty() ? txt[0].toAscii() : 0;
}


uiShortcutsList::uiShortcutsList( const char* selkey )
	: selkey_(selkey)
{
    PtrMan<IOPar> pars = SCMgr().getStored( selkey );
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

	names_.add( name );
	BufferString proplbl;
       	int propval;
	if ( getSCProperties( *pars, index, proplbl, propval ) )
	{
	    uiExtraIntKeyDesc* uieikd =
				new uiExtraIntKeyDesc( val1, val2, propval );
	    uieikd->setIntLabel( proplbl.buf() );
	    keydescs_ += uieikd;
	}
	else
	    keydescs_ += new uiKeyDesc( val1, val2 );

	index++;
    }
}


uiShortcutsList& uiShortcutsList::operator =( const uiShortcutsList& scl )
{
    if ( this == &scl ) return *this;
    empty();
    selkey_ = scl.selkey_;
    names_ = scl.names_;

    //personalized deepCopy
    if ( &keydescs_ == &scl.keydescs_ ) return *this;
    deepErase( keydescs_ );
    keydescs_.allowNull( scl.keydescs_.nullAllowed() );
    const int sz = scl.keydescs_.size();
    for ( int idx=0; idx<sz; idx++ )
    {
	uiKeyDesc* nonconstkd = const_cast<uiKeyDesc*>(scl.keydescs_[idx]);
	mDynamicCastGet( uiExtraIntKeyDesc*, eikd, nonconstkd )
	if ( eikd )
	    keydescs_ += new uiExtraIntKeyDesc( *eikd );
	else
	    keydescs_ += new uiKeyDesc( *nonconstkd );

    }
    return *this; 
}


void uiShortcutsList::empty()
{
    deepErase(keydescs_); names_.erase();
}


bool uiShortcutsList::write( bool usr ) const
{
    return SCMgr().setList( *this, usr );
}


void uiShortcutsList::fillPar( IOPar& iop ) const
{
    for ( int idx=0; idx<names_.size(); idx++ )
    {
	BufferString basekey = IOPar::compKey(selkey_,idx);
	iop.set( IOPar::compKey(basekey,sKey::Name()), names_.get(idx) );
	iop.set( IOPar::compKey(basekey,sKey::Keys()),
			keydescs_[idx]->stateStr(), keydescs_[idx]->keyStr() );
	uiKeyDesc* nonconstkd = const_cast<uiKeyDesc*>(keydescs_[idx]);
	mDynamicCastGet( uiExtraIntKeyDesc*, eikd, nonconstkd )
	if ( eikd )
	{
	    iop.set( IOPar::compKey(basekey,sKey::Property()), eikd->getLabel());
	    iop.set( IOPar::compKey(basekey,sKey::Value()), eikd->getIntValue() );
	}
    }
}


bool uiShortcutsList::getKeyValues( const IOPar& par, int scutidx,
				    BufferString& val1,
				    BufferString& val2 ) const
{
    BufferString key = IOPar::compKey( toString(scutidx), sKey::Keys() );
    return par.get( key.buf(), val1, val2 );
}


bool uiShortcutsList::getSCProperties( const IOPar& par, int scutidx,
				       BufferString& proplbl,
				       int& propval) const
{                                                                               
    BufferString propnm = IOPar::compKey( toString(scutidx), sKey::Property() );
    BufferString propv = IOPar::compKey( toString(scutidx), sKey::Value() );
    return par.get( propnm.buf(), proplbl ) && par.get( propv.buf(), propval );
}


const uiKeyDesc* uiShortcutsList::keyDescOf( const char* nm ) const
{
    for ( int idx=0; idx<names_.size(); idx++ )
    {
	if ( names_.get(idx) == nm && keydescs_[idx] )
	    return keydescs_[idx];
    }
    return 0;
}


const char* uiShortcutsList::nameOf( const uiKeyDesc& kd ) const
{
    for ( int idx=0; idx<names_.size(); idx++ )
    {
	if ( keydescs_[idx] && *keydescs_[idx] == kd )
	    return names_.get( idx );
    }
    return 0;
}


int uiShortcutsList::valueOf( const uiKeyDesc& kd ) const
{
    for ( int idx=0; idx<keydescs_.size(); idx++ )
    {
	if ( keydescs_[idx] && *keydescs_[idx] == kd )
	{
	    uiKeyDesc* nonconstkd = const_cast<uiKeyDesc*>(keydescs_[idx]);
	    mDynamicCastGet( uiExtraIntKeyDesc*, eikd, nonconstkd )
	    return eikd ? eikd->getIntValue(): 1;
	}
    }

    return 1;
}


bool uiShortcutsList::getSCNames( const IOPar& par, int scutidx,
				  BufferString& name ) const
{
    BufferString key = IOPar::compKey( toString(scutidx), sKey::Name() );
    return par.get( key.buf(), name );
}


static const char* sFileKey = "shortcuts";

IOPar* uiShortcutsMgr::getStored( const char* subsel )
{
    Settings& setts = Settings::fetch( sFileKey );
    IOPar* ret = setts.subselect( subsel );
    if ( ret && ret->size() )
	return ret;
    delete ret;

    StreamData sd = StreamProvider(
	    		mGetSetupFileName("ShortCuts")).makeIStream();
    if ( !sd.usable() )
	{ sd.close(); return 0; }

    ascistream astrm( *sd.istrm );
    IOPar* iop = new IOPar( astrm );
    sd.close();
    if ( iop && iop->isEmpty() )
	{ delete iop; return 0; }

    ret = iop->subselect( subsel );
    delete iop;
    if ( ret && ret->isEmpty() )
	{ delete ret; return 0; }

    return ret;
}


const uiShortcutsList& uiShortcutsMgr::getList( const char* key ) const
{
    const int idx = keys_.indexOf( key );
    if ( idx >= 0 ) return *lists_[idx];

    uiShortcutsMgr& self = *(const_cast<uiShortcutsMgr*>(this));
    uiShortcutsList* newlist = new uiShortcutsList( key );
    self.keys_.add( key );
    self.lists_ += newlist;
    return *newlist;
}


uiShortcutsMgr::uiShortcutsMgr()
    : shortcutsChanged( this )
{}


bool uiShortcutsMgr::setList( const uiShortcutsList& scl, bool usr )
{
    if ( !usr )
    {
	pErrMsg( "Needs implementation" );
	return false;
    }

    IOPar iop; scl.fillPar( iop );
    Settings& setts = Settings::fetch( sFileKey );
    setts.merge( iop );
    if ( !setts.write(false) )
	return false;

    uiShortcutsList& myscl = const_cast<uiShortcutsList&>(getList(scl.selkey_));
    myscl = scl;
    shortcutsChanged.trigger();
    return true;
}



uiExtraIntKeyDesc::uiExtraIntKeyDesc( const char* str1, const char* str2,
				      int val )
    : uiKeyDesc( str1, str2 )
    , val_( val )
{
}


uiExtraIntKeyDesc::uiExtraIntKeyDesc( const uiExtraIntKeyDesc& uieikd )
    : uiKeyDesc( uieikd.stateStr(), uieikd.keyStr() )
    , val_( uieikd.getIntValue() )
{
    setIntLabel( uieikd.getLabel() );
}


bool uiExtraIntKeyDesc::set( const char* statestr, const char* keystr, int val )
{
    val_ = val;
    return uiKeyDesc::set( statestr, keystr );
}

