/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 2-8-1994
-*/

static const char* rcsID = "$Id: ioobj.cc,v 1.5 2001-07-06 11:40:37 bert Exp $";

#include "iodir.h"
#include "ioman.h"
#include "iolink.h"
#include "iopar.h"
#include "iostrm.h"
#include "iox.h"
#include "transl.h"
#include "ascstream.h"
#include "separstr.h"
#include "filegen.h"
#include <stdlib.h>

DefineAbstractClassDef(IOObj,"IO Object");
DefineAbstractClassDef(IOObject,"Factual IO Object");


IOObj::IOObj( const char* nm, const char* ky )
	: UserIDObject(nm)
	, key_(ky)
	, dirname_(0)
	, connclassdef_(0)
	, opts(0)
{
}


IOObj::IOObj( IOObj* l, const char* ky )
	: UserIDObject(l)
	, key_(ky)
	, dirname_(0)
	, connclassdef_(0)
	, opts(0)
{
    if ( l )
	connclassdef_ = l->connclassdef_;
}


IOObj::~IOObj()
{
    delete dirname_;
    delete opts;
}


void IOObj::copyFrom( const IOObj* obj )
{
    if ( !obj ) return;
    setParentKey( obj->parentKey() );
    setGroup( obj->group() );
    setTranslator( obj->translator() );
    setName( obj->name() );
    if ( obj->opts )
    {
	opts = new IOPar( this );
	*opts = *obj->opts;
    }
}


IOObj* IOObj::getParent() const
{
    return IODir::getObj( parentKey() );
}


int IOObj::setName( const char* nm )
{
    if ( !nm || !*nm ) return NO;
    return UserIDObject::setName(nm) ? YES : NO;
}


const char* IOObj::dirName() const
{
    if ( dirname_ ) return *dirname_;
    return IOM().curDir();
}


void IOObj::mkOpts()
{
   opts = new IOPar( this );
}


static FileMultiString fms;

IOObj* IOObj::get( ascistream& astream, const char* dirnm, const char* dirky )
{
    if ( atEndOfSection(astream) )
	astream.next();
    if ( atEndOfSection(astream) )
	return 0;
    MultiID mykey( dirky );
    if ( *astream.keyWord() == '@' )
    {
	IOLink* ln = IOLink::get( astream, dirnm );
	if ( !ln ) return 0;
	mykey += ln->key();
	ln->setKey( mykey );
	return ln;
    }

    UserIDString _name( astream.keyWord() );
    fms = astream.value();
    mykey += fms[0];
    MultiID parkey( fms[1] );
    astream.next();
    UserIDString _group( astream.keyWord() );
    fms = astream.value();
    UserIDString _trl( fms[0] );
    UserIDString _typ( fms[1] );
    if ( ! *(const char*)_typ )
    {
	Translator* tr = Translator::produce( _group, _trl );
	if ( !tr ) return 0;
	_typ = tr->connClassDef().name();
	delete tr;
    }

    IOObj* objptr = produce( _typ, _name, mykey, NO );
    if ( !objptr ) return 0;

    objptr->setParentKey( parkey );
    objptr->setGroup( _group );
    objptr->setTranslator( _trl );

    astream.next();
    if ( *astream.keyWord() != '$' )	{ delete objptr; objptr = 0; }
    else
    {
	if ( !objptr->getFrom(astream) || objptr->bad() )
	    { delete objptr; objptr = 0; }
	else
	{
	    while ( *astream.keyWord() == '#' )
	    {
		if ( !objptr->opts ) objptr->opts = new IOPar( objptr );
		objptr->opts->set( astream.keyWord()+1, astream.value() );
		astream.next();
	    }
	}
    }

    while ( !atEndOfSection(astream) ) astream.next();
    return objptr;
}


IOObj* IOObj::produce( const char* typ, const char* nm, const char* keyin,
			bool gendef )
{
    IOObj* objptr = 0;

    if ( !nm || !*nm ) nm = "?";
    MultiID ky( keyin );
    if ( ky == "" ) ky = IOM().dirPtr()->newKey();

    if ( !strcmp(typ,"Stream") )
	objptr = new IOStream( nm, ky, gendef );
    else if ( !strcmp(typ,"X-Group") )
	objptr = new IOX( nm, ky, gendef );

    return objptr;
}


Translator* IOObj::getTranslator() const
{
    Translator* tr = Translator::produce( group(), translator() );
    if ( !tr ) return 0;

    if ( opts ) tr->usePar( opts );
    return tr;
}


IOObj* IOObj::clone() const
{
    const IOObj* dataobj = isLink() ? ((IOLink*)this)->link() : this;
    IOObj* newioobj = produce( dataobj->classDef().name(), name(),
				dataobj->key(), NO );
    newioobj->copyFrom( dataobj );
    newioobj->setStandAlone( dataobj->dirName() );
    return newioobj;
}


void IOObj::acquireNewKey()
{
    key_ = IOM().dirPtr()->newKey();
}


void IOObj::setStandAlone( const char* dirname )
{
    if ( !dirname )
    {
	delete dirname_;
	dirname_ = 0;
	return;
    }
    if ( dirname_ )	*dirname_ = dirname;
    else		dirname_ = new FileNameString( dirname );
}


int IOObj::put( ascostream& astream ) const
{
    if ( !isLink() )
    {
	if ( parentKey() == "" )
	    astream.put( name(), myKey() );
	else
	{
	    fms = "";
	    fms += myKey();
	    fms += parentKey();
	    astream.put( name(), fms );
	}
	fms = translator();
	fms += classDef().name();
	astream.put( group(), fms );
    }

    if ( !putTo( astream ) )
	return NO;
    if ( opts )
    {
	for ( int idx=0; idx<opts->size(); idx++ )
	{
	    astream.stream() << '#';
	    astream.put( opts->getKey(idx), opts->getValue(idx) );
	}
    }
    astream.newParagraph();
    return YES;
}


int IOObj::myKey() const
{
    return atoi( key_.key( key_.nrKeys()-1 ) );
}


bool areEqual( const IOObj* o1, const IOObj* o2 )
{
    if ( !o1 && !o2 ) return YES;
    if ( !o1 || !o2 ) return NO;

    return equalIOObj(o1->key(),o2->key());
}


static void mkStd( MultiID& ky )
{
    if ( ky.ID(ky.nrKeys()-1) == 1 ) ky = ky.upLevel();
    if ( ky == "" ) ky = "0";
}


bool equalIOObj( const MultiID& ky1, const MultiID& ky2 )
{
    MultiID k1( ky1 ); MultiID k2( ky2 );
    mkStd( k1 ); mkStd( k2 );
    return k1 == k2;
}
