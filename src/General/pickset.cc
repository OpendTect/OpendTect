/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Mar 2001
 * FUNCTION : CBVS I/O
-*/

static const char* rcsID = "$Id: pickset.cc,v 1.10 2001-10-06 17:46:40 bert Exp $";

#include "pickset.h"
#include "picksettr.h"
#include "ascstream.h"
#include "ioobj.h"
#include "iopar.h"
#include "ptrman.h"
#include "survinfo.h"


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

    str = endptr; skipLeadingBlanks(str);
    endptr = strchr( str, ' ' );
    if ( !endptr ) endptr = strchr( str, '\t' );
    if ( !endptr ) return false;
    *endptr++ = '\0'; if ( !*endptr ) return false;
    skipLeadingBlanks(endptr);

    pos.x = x;
    pos.y = atof( str );
    z = atof( endptr );

    // Check if data is in inl/crl rather than X and Y
    if ( !SI().isReasonable(pos) )
    {
	BinID bid( mNINT(pos.x), mNINT(pos.y) );
	SI().snap( bid, BinID(0,0) );
	Coord newpos = SI().transform( bid );
	if ( SI().isReasonable(newpos) )
	    pos = newpos;
    }
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
	if ( grp->name() == groups[idx]->name() )
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


bool PickSetTranslator::retrieve( PickSet& ps, const IOObj* ioobj,
				  BufferString& bs, const bool* selarr )
{
    if ( !ioobj ) { bs = "Cannot find object in data base"; return false; }
    mDynamicCastGet(PickSetTranslator*,t,ioobj->getTranslator())
    if ( !t ) { bs = "Selected object is not a Pick Set"; return false; }
    PtrMan<PickSetTranslator> tr = t;
    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
    if ( !conn )
        { bs = "Cannot open "; bs += ioobj->fullUserExpr(true); return false; }
    bs = tr->read( ps, *conn, selarr );
    return bs == "";
}


bool PickSetTranslator::store( const PickSet& ps, const IOObj* ioobj,
			       BufferString& bs, const bool* selarr )
{
    if ( !ioobj ) { bs = "No object to store set in data base"; return false; }
    mDynamicCastGet(PickSetTranslator*,t,ioobj->getTranslator())
    if ( !t ) { bs = "Selected object is not a Pick Set"; return false; }
    PtrMan<PickSetTranslator> tr = t;
    PtrMan<Conn> conn = ioobj->getConn( Conn::Write );
    if ( !conn )
        { bs = "Cannot open "; bs += ioobj->fullUserExpr(false); return false; }
    bs = tr->write( ps, *conn, selarr );
    return bs == "";
}


const char* dgbPickSetTranslator::read( PickSet& ps, Conn& conn,
					const bool* selarr )
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

    float zfac = 1;
    if ( astrm.hasKeyword("Z Factor") )
    {
	zfac = astrm.getValue();
	if ( mIS_ZERO(zfac) || mIsUndefined(zfac) )
	    zfac = 1;
	astrm.next();
	if ( atEndOfSection(astrm) ) astrm.next();
    }

    if ( atEndOfSection(astrm) )
	return "Input file contains no pick groups";

    ps.setName( conn.ioobj ? (const char*)conn.ioobj->name() : "" );

    for ( int igrp=0; !atEndOfSection(astrm); igrp++ )
    {
	PickGroup* newpg = selarr && !selarr[igrp] ? 0
			 : new PickGroup( astrm.value() );
	PickLocation loc;
	while ( !atEndOfSection(astrm.next()) )
	{
	    if ( !loc.fromString( astrm.keyWord() ) )
		break;
	    loc.z *= zfac;
	    if ( newpg ) *newpg += loc;
	}
	while ( !atEndOfSection(astrm) ) astrm.next();
	astrm.next();

	if ( newpg ) ps.add( newpg );
    }


    return ps.nrGroups() ? 0 : "No valid picks found";
}


const char* dgbPickSetTranslator::write( const PickSet& ps, Conn& conn,
					 const bool* selarr )
{
    if ( !conn.forWrite() || !conn.hasClass(StreamConn::classid) )
	return "Internal error: bad connection";

    ascostream astrm( ((StreamConn&)conn).oStream() );
    astrm.putHeader( PickSetTranslator::classdef.name() );
    ostream& strm = astrm.stream();
    if ( !strm.good() )
	return "Cannot write to output Pick Set file";

    for ( int igrp=0; igrp<ps.nrGroups(); igrp++ )
    {
	if ( selarr && !selarr[igrp] ) continue;

	const PickGroup& pg = *ps.get( igrp );
	astrm.put( "Ref", pg.name() );
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
