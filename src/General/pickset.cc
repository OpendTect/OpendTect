/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Mar 2001
 * FUNCTION : CBVS I/O
-*/

static const char* rcsID = "$Id: pickset.cc,v 1.2 2001-05-14 13:57:54 bert Exp $";

#include "pickset.h"
#include "picksettr.h"
#include "ascstream.h"


bool PickLocation::fromString( const char* s )
{
    if ( !s || !*s ) return false;

    BufferString bufstr( s );
    char* str = bufstr.buf();
    char* endptr = strchr( str, ' ' );
    if ( !endptr ) endptr = strchr( str, '\t' );
    if ( !endptr ) return false;
    *endptr++ = '\0';
    double x = atof( str );

    str = endptr; endptr = strchr( str, ' ' );
    if ( !endptr ) endptr = strchr( str, '\t' );
    if ( !endptr ) return false;
    *endptr++ = '\0'; if ( !*endptr ) return false;

    pos.x = x;
    pos.y = atof( str );
    z = atof( endptr );
    return true;
}


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

    ascistream astrm( ((StreamConn&)conn).iStream() );
    istream& strm = astrm.stream();
    if ( !strm.good() )
	return "Cannot read from input file";
    if ( !astrm.isOfFileType(PickSetTranslator::classdef.name()) )
	return "Input file is not a Pick Set";
    if ( atEndOfSection(astrm) ) astrm.next();
    if ( atEndOfSection(astrm) )
	return "Input file contains no pick groups";

    ps.setName( astrm.value() );
    astrm.next(); if ( atEndOfSection(astrm) ) astrm.next();
    if ( atEndOfSection(astrm) )
	return "Input file contains no picks";

    do
    {
	PickGroup* newpg = new PickGroup( astrm.getValue() );
	PickLocation loc;
	while ( !atEndOfSection(astrm.next()) )
	{
	    if ( !loc.fromString( astrm.keyWord() ) )
		break;
	    *newpg += loc;
	}
	while ( !atEndOfSection(astrm) ) astrm.next();
	astrm.next();

	ps.add( newpg );

    } while ( !atEndOfSection(astrm) );

    return ps.nrGroups() ? 0 : "No valid picks found";
}


const char* dgbPickSetTranslator::write( const PickSet& ps, Conn& conn )
{
    if ( !conn.forWrite() || !conn.hasClass(StreamConn::classid) )
	return "Internal error: bad connection";

    return "Pick Set write not implemented yet";
}
