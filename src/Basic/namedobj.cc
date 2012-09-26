/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 21-9-1995
-*/
 
static const char* rcsID mUsedVar = "$Id$";
 
#include "namedobj.h"
#include "string2.h"
#include <ctype.h>

#ifdef __debug__
#include "errh.h"
#endif


NamedObject::~NamedObject()
{
    if ( delnotify_ )
    {
	delnotify_->doCall( this, 0 );
	delete delnotify_; delnotify_ = 0;
    }

#ifdef __debug__
    static BufferString delobj( "deleted object" );

    if( name_ == &delobj )
	{ pErrMsg("Multiple delete detected"); return; }

    delete name_; name_ = &delobj;
#else
    delete name_; name_ = 0;
#endif
}


void NamedObject::deleteNotify( const CallBack& c )
{
    if ( !c.willCall() ) return;
    CallBack cb( c );
    mDynamicCastGet(NamedObject*,o,cb.cbObj())
    if ( !o ) return;

    if ( !delnotify_ ) delnotify_ = new CallBackSet;
    if ( !o->delnotify_ ) o->delnotify_ = new CallBackSet;

    *o->delnotify_ += mCB(this,NamedObject,cbRem);
    *delnotify_    += mCB(o,NamedObject,cbRem);
    *delnotify_    += cb;
}


void NamedObject::cbRem( NamedObject* o )
{
    if ( !delnotify_ ) return; // Huh?
    for ( int idx=delnotify_->size()-1; idx>=0; idx-- )
    {
	CallBack cb = (*delnotify_)[idx];
	if ( cb.cbObj() == o ) *delnotify_ -= cb;
    }
}


void NamedObject::setName( const char* nm )
{
    if ( !name_ )
	{ linkedto_->setName(nm); return; }
    else if ( !nm )
	nm = "";
    mSkipBlanks(nm);
    *name_ = nm;
    removeTrailingBlanks( name_->buf() );
}


void NamedObject::setCleanName( const char* nm )
{
    if ( !name_ )
	{ linkedto_->setCleanName(nm); return; }
    else if ( !nm || !*nm )
	{ setName( "" ); return; }
    mSkipBlanks(nm);
    if ( !*nm )
	{ setName( "" ); return; }

    BufferString uidstr( nm );
    char* ptr = uidstr.buf();
    removeTrailingBlanks(ptr);
    char* startptr = ptr;
    if ( *ptr == '!' || *ptr == '#' || *ptr == '$' ) *ptr = '_';
    else if ( *ptr == '\\' ) startptr++;
    ptr++;
    while ( *ptr )
    {
	if ( !isprint(*ptr) || *ptr == '\t' || *ptr == '\n'
			    || *ptr == ':' || *ptr == '`' )
	    *ptr = '_';
	ptr++;
    }

    setName( startptr );
}
