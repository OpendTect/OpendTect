/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 7-1-1996
-*/

static const char* rcsID = "$Id: ctxtioobj.cc,v 1.2 2001-02-13 17:21:02 bert Exp $";

#include "ioobj.h"
#include "ctxtioobj.h"
#include "ioman.h"


IOObjContext::IOObjContext( const Translator* trg, const char* prefname )
	: UserIDObject(prefname)
	, trgroup(trg)
{
    init();
}


IOObjContext::IOObjContext( const IOObjContext& rp )
	: UserIDObject("")
{
    *this = rp;
}


IOObjContext& IOObjContext::operator=( const IOObjContext& ct )
{
    if ( this != &ct )
    {
	setName( ct.name() );
	trgroup = ct.trgroup;
	newonlevel = ct.newonlevel;
	crlink = ct.crlink;
	needparent = ct.needparent;
	parentlevel = ct.parentlevel;
	partrgroup = ct.partrgroup;
	multi = ct.multi;
	selkey = ct.selkey;
	forread = ct.forread;
	maychdir = ct.maychdir;
	maydooper = ct.maydooper;
	ioobjclassid = ct.ioobjclassid;
	parentkey = ct.parentkey;
	deftransl = ct.deftransl;
    }
    return *this;
}


void CtxtIOObj::setObj( IOObj* obj )
{
    if ( obj == ioobj ) return;

    delete ioobj; ioobj = obj;
    if ( ioobj ) ctxt.parentkey = ioobj->parentKey();
}


int CtxtIOObj::fillObj( const MultiID& uid )
{
    if ( ioobj && (ctxt.name() == ioobj->name() || ctxt.name() == "") )
	return 1;
    IOM().getEntry( *this, uid );
    return ioobj ? 2 : 0;
}
