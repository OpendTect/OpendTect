/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Mar 2001
 * FUNCTION : CBVS I/O
-*/

static const char* rcsID = "$Id: pickset.cc,v 1.1 2001-05-14 13:21:51 bert Exp $";

#include "pickset.h"
#include "picksettr.h"


void PickSet::add( PickGroup*& grp )
{
    if ( !grp ) return;
    const int grpsz = grp->size();
    if ( !grpsz ) { delete grp; return; }

    const int nrgrps = groups.size();
    int mrgnr = -1;
    for ( int idx=0; idx<nrgrps; idx++ )
	if ( mIS_ZERO(grp->val - groups[idx]->val) )
	    { mrgnr = idx; break; }
    if ( mrgnr < 0 ) { groups += grp; return; }

    PickGroup& mrggrp = *grp;
    grp = groups[mrgnr];

    for ( int idx=0; idx<grpsz; idx++ )
    {
	if ( grp->indexOf( mrggrp[idx] ) < 0 )
	    *grp += mrggrp[idx];
    }

    delete &mrggrp;
}


IOObjContext PickSetTranslator::ioContext()
{
    IOObjContext ctxt( Translator::groups()[listid] );
    ctxt.crlink = false;
    ctxt.newonlevel = 1;
    ctxt.needparent = false;
    ctxt.maychdir = false;
    ctxt.stdseltype = IOObjContext::Misc;

    return ctxt;
}


int PickSetTranslator::selector( const char* key )
{
    return defaultSelector( classdef.name(), key );
}


const char* dgbPickSetTranslator::read( PickSet& ps, Conn& conn )
{
    if ( !conn.forRead() || !conn.hasClass(StreamConn::classid) )
	return "Internal error: bad connection";

    // PickSetTranslator::classdef.name()
    // ((StreamConn&)conn).iStream()

    return 0;
}


const char* dgbPickSetTranslator::write( const PickSet& ps, Conn& conn )
{
    if ( !conn.forWrite() || !conn.hasClass(StreamConn::classid) )
	return "Internal error: bad connection";

    // PickSetTranslator::classdef.name()
    // ((StreamConn&)conn).oStream()

    return 0;
}
