/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 2-8-1994
-*/

static const char* rcsID = "$Id: ioobj.cc,v 1.1.1.1 1999-09-03 10:11:27 dgb Exp $";

#include "iodir.h"
#include "ioman.h"
#include "iolink.h"
#include "iopar.h"
#include "iostrm.h"
#include "iox.h"
#include "aobset.h"
#include "transl.h"
#include "ascstream.h"
#include "separstr.h"
#include "filegen.h"
#include <stdlib.h>

DefineAbstractClassDef(IOObj,"IO Object");
DefineAbstractClassDef(IOObject,"Factual IO Object");


IOObj::IOObj( const char* nm, const char* uid )
	: UnitIDObject(nm,uid)
	, dirname_(0)
	, connclassdef_(0)
	, opts(0)
{
}


IOObj::IOObj( IOObj* l, const char* uid )
	: UnitIDObject(l,uid)
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
    setParentId( obj->parentId() );
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
    return IODir::getObj( parentId() );
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

IOObj* IOObj::get( ascistream& astream, const char* dirnm, const char* diruid )
{
    if ( atEndOfSection(astream) )
	astream.next();
    if ( atEndOfSection(astream) )
	return 0;
    UnitID myid( diruid );
    if ( astream.keyword[0] == '@' )
    {
	IOLink* ln = IOLink::get( astream, dirnm );
	if ( !ln ) return 0;
	myid += ln->unitID();
	ln->setUnitID( myid );
	return ln;
    }

    UserIDString _name( astream.keyword );
    fms = astream.valstr;
    myid += fms[0];
    UnitID parid( fms[1] );
    astream.next();
    UserIDString _group( astream.keyword );
    fms = astream.valstr;
    UserIDString _trl( fms[0] );
    UserIDString _typ( fms[1] );
    if ( ! *(const char*)_typ )
    {
	Translator* tr = Translator::produce( _group, _trl );
	if ( !tr ) return 0;
	_typ = tr->connClassDef().name();
	delete tr;
    }

    IOObj* objptr = produce( _typ, _name, myid, NO );
    if ( !objptr ) return 0;

    objptr->setParentId( parid );
    objptr->setGroup( _group );
    objptr->setTranslator( _trl );

    astream.next();
    if ( astream.keyword[0] != '$' )	{ delete objptr; objptr = 0; }
    else
    {
	if ( !objptr->getFrom(astream) || objptr->bad() )
	    { delete objptr; objptr = 0; }
	else
	{
	    while ( astream.keyword[0] == '#' )
	    {
		if ( !objptr->opts ) objptr->opts = new IOPar( objptr );
		objptr->opts->set( astream.keyword+1, astream.valstr );
		astream.next();
	    }
	}
    }

    while ( !atEndOfSection(astream) ) astream.next();
    return objptr;
}


IOObj* IOObj::produce( const char* typ, const char* nm, const char* unid,
			bool gendef )
{
    IOObj* objptr = 0;

    if ( !nm || !*nm ) nm = "?";
    UnitID uid( unid );
    if ( uid == "" ) uid = IOM().dirPtr()->newId();

    if ( !strcmp(typ,"Stream") )
	objptr = new IOStream( nm, uid, gendef );
    else if ( !strcmp(typ,"X-Group") )
	objptr = new IOX( nm, uid, gendef );

    return objptr;
}


Translator* IOObj::getTranslator() const
{
    Translator* tr = Translator::produce( group(), translator() );
    if ( !tr ) return 0;

    if ( opts ) tr->usePar( opts );
    return tr;
}


IOObj* IOObj::cloneStandAlone() const
{
    const IOObj* dataobj = isLink() ? ((IOLink*)this)->link() : this;
    IOObj* newioobj = produce( dataobj->classDef().name(), name(),
				dataobj->unitID(), NO );
    newioobj->copyFrom( dataobj );
    newioobj->setStandAlone( dataobj->dirName() );
    return newioobj;
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
	if ( parentId() == "" )
	    astream.put( name(), myId() );
	else
	{
	    fms = "";
	    fms += myId();
	    fms += parentId();
	    astream.put( name(), fms );
	}
	fms = translator();
	fms += classDef().name();
	astream.put( group(), fms );
    }

    if ( !putTo( astream ) ) return NO;
    if ( opts && opts->size() )
    {
	AliasObjectSet& pars = opts->getPars();
	for ( int idx=0; idx<pars.size(); idx++ )
	{
	    astream.stream() << '#';
	    AliasObject* par = pars[idx];
	    astream.put( par->name(), par->obj->name() );
	}
    }
    astream.newParagraph();
    return YES;
}


int IOObj::myId() const
{
    return atoi( unitid.code( unitid.level() ) );
}


bool areEqual( const IOObj* o1, const IOObj* o2 )
{
    if ( !o1 && !o2 ) return YES;
    if ( !o1 || !o2 ) return NO;

    return equalIOObj(o1->unitID(),o2->unitID());
}


static void mkStd( UnitID& id )
{
    if ( id.code() == "1" ) id = id.parent();
    if ( id == "" ) id = "0";
}


bool equalIOObj( const UnitID& id1, const UnitID& id2 )
{
    UnitID u1( id1 ); UnitID u2( id2 );
    mkStd( u1 ); mkStd( u2 );
    return u1 == u2;
}
