/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Mar 2001
 * FUNCTION : CBVS I/O
-*/

static const char* rcsID = "$Id: pickset.cc,v 1.3 2001-05-24 15:40:17 bert Exp $";

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


void PickLocation::toString( char* str )
{
			strcpy( str, getStringFromDouble(0,pos.x) );
    strcat( str, " " ); strcat( str, getStringFromDouble(0,pos.y) );
    strcat( str, " " ); strcat( str, getStringFromFloat(0,z) );
}


void PickSet::add( PickGroup*& grp )
{
    if ( !grp ) return;
    const int grpsz = grp->size();
    if ( !grpsz ) { delete grp; return; }

    const int nrgrps = groups.size();
    int mrgnr = -1;
    for ( int idx=0; idx<nrgrps; idx++ )
	if ( grp->userref == groups[idx]->userref )
	    { mrgnr = idx; break; }
    if ( mrgnr < 0 ) { groups += grp; return; }

    PickGroup& mrggrp = *grp;
    grp = groups[mrgnr];
    grp->val = mrggrp.val;

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
	astrm.next(); newpg->userref = astrm.value();
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

    ascostream astrm( ((StreamConn&)conn).oStream() );
    astrm.putHeader( PickSetTranslator::classdef.name() );
    ostream& strm = astrm.stream();
    if ( !strm.good() )
	return "Cannot write to output Pick Set file";

    astrm.put( "Name", ps.name() );
    astrm.newParagraph();

    for ( int igrp=0; igrp<ps.nrGroups(); igrp++ )
    {
	const PickGroup& pg = *ps.get( igrp );
	astrm.put( "Value", pg.val );
	astrm.put( "Ref", pg.userref );
	char buf[80];
	for ( int iloc=0; iloc<pg.size(); iloc++ )
	{
	    pg[iloc].toString( buf );
	    strm << buf << '\n';
	}
	astrm.newParagraph();
	if ( !strm.good() )
	    return "Error during write to output Pick Set file";
    }

    return 0;
}
