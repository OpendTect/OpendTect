/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		December 2006
 RCS:		$Id: randomlinetr.cc,v 1.1 2006-12-14 21:44:35 cvsnanne Exp $
________________________________________________________________________

-*/

#include "randomlinetr.h"
#include "randomlinefact.h"
#include "randomlinegeom.h"

#include "ascstream.h"
#include "bufstringset.h"
#include "ioobj.h"
#include "conn.h"
#include "ptrman.h"


mDefSimpleTranslatorSelector(RandomLine,sKeyRandomLineTranslatorGroup)
mDefSimpleTranslatorioContext(RandomLine,Loc)


bool RandomLineTranslator::retrieve( Geometry::RandomLine& rdl,
				     const IOObj* ioobj, BufferString& bs )
{
    if ( !ioobj )
	{ bs = "Cannot find object in data base"; return false; }

    PtrMan<RandomLineTranslator> tr
	= dynamic_cast<RandomLineTranslator*>(ioobj->getTranslator());
    if ( !tr )
	{ bs = "Selected object is not a Random Line"; return false; }

    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
    if ( !conn )
	{ bs = "Cannot open "; bs += ioobj->fullUserExpr(true); return false; }

    bs = tr->read( rdl, *conn );
    return bs.isEmpty();
}


bool RandomLineTranslator::store( const Geometry::RandomLine& rdl,
				  const IOObj* ioobj, BufferString& bs )
{
    if ( !ioobj )
	{ bs = "No object to store set in data base"; return false; }
    
    PtrMan<RandomLineTranslator> tr
	= dynamic_cast<RandomLineTranslator*>(ioobj->getTranslator());
    if ( !tr )
	{ bs = "Selected object is not an Attribute Set"; return false; }

    PtrMan<Conn> conn = ioobj->getConn( Conn::Write );
    if ( !conn )
	{ bs = "Cannot open "; bs += ioobj->fullUserExpr(false); return false; }

    bs = tr->write( rdl, *conn );
    return bs.isEmpty();
}


const char* dgbRandomLineTranslator::read( Geometry::RandomLine& rdl,
					   Conn& conn )
{
    if ( !conn.forRead() || !conn.isStream() )
	return "Internal error: bad connection";

    ascistream astrm( ((StreamConn&)conn).iStream() );
    std::istream& strm = astrm.stream();
    if ( !strm.good() )
	return "Cannot read from input file";
    if ( !astrm.isOfFileType(mTranslGroupName(RandomLine)) )
	return "Input file is not a RandomLine file";
    if ( atEndOfSection(astrm) ) astrm.next();

    Interval<float> zrg;
    strm >> zrg.start >> zrg.stop;
    rdl.setZRange( zrg );
    astrm.next();

    while ( !atEndOfSection(astrm) )
    {
	BinID bid;
	strm >> bid.inl >> bid.crl;
	rdl.addNode( bid );
    }

    return 0;
}


const char* dgbRandomLineTranslator::write( const Geometry::RandomLine& rdl,
					    Conn& conn )
{
    if ( !conn.forWrite() || !conn.isStream() )
	return "Internal error: bad connection";

    ascostream astrm( ((StreamConn&)conn).oStream() );
    astrm.putHeader( mTranslGroupName(RandomLine) );
    std::ostream& strm = astrm.stream();
    if ( !strm.good() )
	return "Cannot write to output RandomLine file";

    strm << rdl.zRange().start << '\t' << rdl.zRange().stop << '\n';
    for ( int idx=0; idx<rdl.nrNodes(); idx++ )
    {
	const BinID& bid = rdl.nodePosition( idx );
	strm << bid.inl << '\t' << bid.crl << '\n';
    }

    astrm.newParagraph();
    return strm.good() ? 0 : "Error during write to output RandomLine file";
}
