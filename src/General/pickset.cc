/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Mar 2001
 * FUNCTION : CBVS I/O
-*/

static const char* rcsID = "$Id: pickset.cc,v 1.18 2002-04-23 06:17:29 nanne Exp $";

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


PickSet::PickSet( const char* nm )
	: UserIDObject(nm)
	, color(Color::NoColor)
{
}


void PickSetGroup::add( PickSet*& ps )
{
    if ( !ps ) return;
    const int pssz = ps->size();
    if ( !pssz ) { delete ps; return; }

    const int nrpss = sets.size();
    int mrgnr = -1;
    for ( int idx=0; idx<nrpss; idx++ )
	if ( ps->name() == sets[idx]->name() )
	    { mrgnr = idx; break; }
    if ( mrgnr < 0 ) { sets += ps; return; }

    PickSet& mrgps = *ps;
    ps = sets[mrgnr];

    for ( int idx=0; idx<pssz; idx++ )
    {
	if ( ps->indexOf( mrgps[idx] ) < 0 )
	    *ps += mrgps[idx];
    }

    delete &mrgps;
}


const IOObjContext& PickSetGroupTranslator::ioContext()
{
    static IOObjContext* ctxt = 0;

    if ( !ctxt )
    {
	ctxt = new IOObjContext( Translator::groups()[listid] );
	ctxt->crlink = false;
	ctxt->newonlevel = 1;
	ctxt->needparent = false;
	ctxt->maychdir = false;
	ctxt->stdseltype = IOObjContext::Misc;
    }

    return *ctxt;
}


int PickSetGroupTranslator::selector( const char* key )
{
    return defaultSelector( classdef.name(), key );
}


bool PickSetGroupTranslator::retrieve( PickSetGroup& psg, const IOObj* ioobj,
				  BufferString& bs, const bool* selarr )
{
    if ( !ioobj ) { bs = "Cannot find object in data base"; return false; }
    mDynamicCastGet(PickSetGroupTranslator*,t,ioobj->getTranslator())
    if ( !t ) { bs = "Selected object is not a PickSet Group"; return false; }
    PtrMan<PickSetGroupTranslator> tr = t;
    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
    if ( !conn )
        { bs = "Cannot open "; bs += ioobj->fullUserExpr(true); return false; }
    bs = tr->read( psg, *conn, selarr );
    return bs == "";
}


bool PickSetGroupTranslator::store( const PickSetGroup& inppsg,
				    const IOObj* ioobj,
				    BufferString& bs, const bool* selarr,
				    bool domrg )
{
    if ( !ioobj ) { bs = "No object to store set in data base"; return false; }
    mDynamicCastGet(PickSetGroupTranslator*,t,ioobj->getTranslator())
    if ( !t ) { bs = "Selected object is not a PickSet Group"; return false; }

    PtrMan<PickSetGroupTranslator> tr = t;
    PickSetGroup mrgd;
    if ( domrg )
    {
	Conn* conn = ioobj->getConn( Conn::Read );
	if ( conn && !conn->bad() )
	{
	    delete conn;
	    if ( !retrieve(mrgd,ioobj,bs) )
		return false;
	}

	const int orgsz = mrgd.nrSets();
	for ( int idx=0; idx<inppsg.nrSets(); idx++ )
	{
	    const PickSet& ps = *inppsg.get( idx );
	    const UserIDString& nm = ps.name();
	    bool found = false;
	    for ( int iorg=0; iorg<orgsz; iorg++ )
	    {
		PickSet& mrgdps = *mrgd.get( iorg );
		if ( nm == mrgdps.name() )
		{
		    found = true;
		    mrgdps.copy( ps );
		    mrgdps.color = ps.color;
		    mrgdps.color.setTransparency( 0 );
		    break;
		}
	    }
	    if ( !found )
	    {
		PickSet* newps = new PickSet( ps.name() );
		newps->copy( ps );
		newps->color = ps.color;
		newps->color.setTransparency( 0 );
		mrgd.add( newps );
	    }
	}
    }

    bs = "";
    const PickSetGroup& wrgrp = domrg ? mrgd : inppsg;
    PtrMan<Conn> conn = ioobj->getConn( Conn::Write );
    if ( !conn )
        { bs = "Cannot open "; bs += ioobj->fullUserExpr(false); return false; }
    bs = tr->write( wrgrp, *conn, selarr );
    return bs == "";
}


const char* dgbPickSetGroupTranslator::read( PickSetGroup& psg, Conn& conn,
					     const bool* selarr )
{
    if ( !conn.forRead() || !conn.hasClass(StreamConn::classid) )
	return "Internal error: bad connection";

    ascistream astrm( ((StreamConn&)conn).iStream() );
    istream& strm = astrm.stream();
    if ( !strm.good() )
	return "Cannot read from input file";
    if ( !astrm.isOfFileType(PickSetGroupTranslator::classdef.name()) )
	return "Input file is not a Pick Set Group";
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
	return "Input file contains no pick sets";

    psg.setName( conn.ioobj ? (const char*)conn.ioobj->name() : "" );

    for ( int ips=0; !atEndOfSection(astrm); ips++ )
    {
	PickSet* newps = selarr && !selarr[ips] ? 0
			 : new PickSet( astrm.value() );
	astrm.next();
	if ( astrm.hasKeyword("Color") )
	{
	    if ( newps ) newps->color.use( astrm.value() );
	    astrm.next();
	}
	PickLocation loc;
	while ( !atEndOfSection(astrm) )
	{
	    if ( !loc.fromString( astrm.keyWord() ) )
		break;
	    loc.z *= zfac;
	    if ( newps ) *newps += loc;
	    astrm.next();
	}
	while ( !atEndOfSection(astrm) ) astrm.next();
	astrm.next();

	if ( newps ) psg.add( newps );
    }


    return psg.nrSets() ? 0 : "No valid picks found";
}


const char* dgbPickSetGroupTranslator::write( const PickSetGroup& psg, Conn& conn,
					 const bool* selarr )
{
    if ( !conn.forWrite() || !conn.hasClass(StreamConn::classid) )
	return "Internal error: bad connection";

    ascostream astrm( ((StreamConn&)conn).oStream() );
    astrm.putHeader( PickSetGroupTranslator::classdef.name() );
    ostream& strm = astrm.stream();
    if ( !strm.good() )
	return "Cannot write to output Pick Set file";

    for ( int iset=0; iset<psg.nrSets(); iset++ )
    {
	if ( selarr && !selarr[iset] ) continue;

	const PickSet& ps = *psg.get( iset );
	astrm.put( "Ref", ps.name() );
	char buf[80];
	if ( ps.color != Color::NoColor )
	{
	    ps.color.fill( buf );
	    astrm.put( "Color", buf );
	}
	for ( int iloc=0; iloc<ps.size(); iloc++ )
	{
	    ps[iloc].toString( buf );
	    strm << buf << '\n';
	}
	astrm.newParagraph();
	if ( !strm.good() )
	    return "Error during write to output Pick Set file";
    }

    return 0;
}
