/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 1995
 * FUNCTION : Translator functions
-*/

static const char* rcsID = "$Id: iox.cc,v 1.1.1.2 1999-09-16 09:33:38 arend Exp $";

#include "iox.h"
#include "iolink.h"
#include "ioman.h"
#include "ascstream.h"
#include "conn.h"

DefineConcreteClassDef(IOX,"X-Group");


IOX::IOX( const char* nm, const char* uid, bool )
	: IOObject(nm,uid)
	, uid("")
{
    connclassdef_ = &StreamConn::classdef;
}


IOX::~IOX()
{
}


void IOX::setUid( const UnitID& id )
{
    uid = id;
}


bool IOX::hasConn( const ClassDef& cd ) const
{
    IOObj* ioobj = IOM().get( uid );
    if ( !ioobj ) return &cd == connclassdef_;
    bool rv = ioobj->hasConn( cd );
    delete ioobj;
    return rv;
}


bool IOX::bad() const
{
    return uid == "";
}


void IOX::copyFrom( const IOObj* obj )
{
    if ( !obj ) return;
    if ( obj->isLink() ) obj = ((IOLink*)obj)->link();
    if ( !obj ) return;

    IOObj::copyFrom(obj);
    if ( obj->hasClass(IOX::classid) )
    {
	const IOX* trobj = (const IOX*)obj;
	uid = trobj->uid;
    }
}


const char* IOX::fullUserExpr( bool i ) const
{
    IOObj* ioobj = IOM().get( uid );
    if ( !ioobj ) return NO;
    const char* s = ioobj->fullUserExpr(i);
    delete ioobj;
    return s;
}


bool IOX::implExists( bool i ) const
{
    IOObj* ioobj = IOM().get( uid );
    if ( !ioobj ) return NO;
    bool yn = ioobj->implExists(i);
    delete ioobj;
    return yn;
}


bool IOX::implRemovable() const
{
    return NO;
}


bool IOX::implRemove() const
{
    return NO;
}


Conn* IOX::conn( Conn::State rw ) const
{
    IOObj* ioobj = getIOObj();
    if ( !ioobj ) return 0;
    Conn* conn = ioobj->conn( rw );
    delete ioobj;
    conn->ioobj = (IOObj*)this;
    return conn;
}


IOObj* IOX::getIOObj() const
{
    return uid == "" ? 0 : IOM().get( uid );
}


int IOX::getFrom( ascistream& stream )
{
    uid = stream.valstr;
    stream.next();
    return YES;
}


int IOX::putTo( ascostream& stream ) const
{
    stream.stream() << '$';
    stream.put( "ID", uid );
    return YES;
}
