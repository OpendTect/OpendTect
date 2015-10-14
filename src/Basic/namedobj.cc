/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 21-9-1995
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "namedobj.h"
#include "string2.h"
#include <ctype.h>


NamedObject::NamedObject( const char* nm )
    : linkedto_(0)
    , delnotify_(0)
{
    name_ = new BufferString( nm );
}


NamedObject::NamedObject( const NamedObject* lnk )
    : linkedto_(const_cast<NamedObject*>(lnk))
    , name_(0)
    , delnotify_(0)
{
}


NamedObject::NamedObject( const NamedObject& oth )
    : CallBacker(oth)
    , linkedto_(oth.linkedto_)
    , name_(0)
    , delnotify_(0)
{
    if ( oth.name_ )
	name_ = new BufferString( *oth.name_ );
}


NamedObject::~NamedObject()
{
    if ( delnotify_ )
    {
	delnotify_->doCall( this, 0 );
	delnotify_->unRef(); delnotify_ = 0;
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


void NamedObject::setLinkedTo( NamedObject* oth )
{
    if ( oth )
	{ delete name_; name_ = 0; }
    else if ( !name_ )
	name_ = new BufferString;
    linkedto_ = oth;
}


#define mMkDelnotif(obj) \
{ obj->delnotify_ = new CallBackSet; obj->delnotify_->ref(); }

void NamedObject::deleteNotify( const CallBack& c )
{
    if ( !c.willCall() ) return;
    CallBack cb( c );
    mDynamicCastGet(NamedObject*,o,cb.cbObj())
    if ( !o ) return;

    if ( !delnotify_ )
	mMkDelnotif(this)
    if ( !o->delnotify_ )
	mMkDelnotif(o)

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
    if ( linkedto_ )
	linkedto_->setName( nm );
    else
    {
	*name_ = nm;
	name_->trimBlanks();
    }
}


void NamedObject::setCleanName( const char* nm )
{
    if ( linkedto_ )
	{ linkedto_->setCleanName(nm); return; }

    BufferString clnnm( nm );
    clnnm.trimBlanks();
    if ( clnnm.isEmpty() )
	{ setName( clnnm ); return; }

    char* ptr = clnnm.getCStr();
    char* startptr = ptr;
    if ( *ptr == '!' || *ptr == '#' || *ptr == '$' )
	*ptr = '_';
    else if ( *ptr == '\\' )
	startptr++;

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

