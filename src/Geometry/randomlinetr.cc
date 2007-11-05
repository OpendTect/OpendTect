/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		December 2006
 RCS:		$Id: randomlinetr.cc,v 1.3 2007-11-05 15:20:06 cvsbert Exp $
________________________________________________________________________

-*/

#include "randomlinetr.h"
#include "randomlinefact.h"
#include "randomlinegeom.h"

#include "ascstream.h"
#include "bufstringset.h"
#include "conn.h"
#include "convert.h"
#include "ioobj.h"
#include "keystrs.h"
#include "ptrman.h"
#include "separstr.h"
#include "survinfo.h"

static const char* sKeyNrLines = "Nr Lines";

mDefSimpleTranslatorSelector(RandomLineSet,sKeyRandomLineSetTranslatorGroup)
mDefSimpleTranslatorioContext(RandomLineSet,Loc)


bool RandomLineSetTranslator::retrieve( Geometry::RandomLineSet& rdls,
				     const IOObj* ioobj, BufferString& bs )
{
    if ( !ioobj )
	{ bs = "Cannot find object in data base"; return false; }

    PtrMan<RandomLineSetTranslator> tr
	= dynamic_cast<RandomLineSetTranslator*>(ioobj->getTranslator());
    if ( !tr )
	{ bs = "Selected object is not a Random Line"; return false; }

    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
    if ( !conn )
	{ bs = "Cannot open "; bs += ioobj->fullUserExpr(true); return false; }

    bs = tr->read( rdls, *conn );
    return bs.isEmpty();
}


bool RandomLineSetTranslator::store( const Geometry::RandomLineSet& rdl,
				  const IOObj* ioobj, BufferString& bs )
{
    if ( !ioobj )
	{ bs = "No object to store set in data base"; return false; }
    
    PtrMan<RandomLineSetTranslator> tr
	= dynamic_cast<RandomLineSetTranslator*>(ioobj->getTranslator());
    if ( !tr )
	{ bs = "Selected object is not an Attribute Set"; return false; }

    PtrMan<Conn> conn = ioobj->getConn( Conn::Write );
    if ( !conn )
	{ bs = "Cannot open "; bs += ioobj->fullUserExpr(false); return false; }

    bs = tr->write( rdl, *conn );
    return bs.isEmpty();
}


const char* dgbRandomLineSetTranslator::read( Geometry::RandomLineSet& rdls,
					   Conn& conn )
{
    if ( !conn.forRead() || !conn.isStream() )
	return "Internal error: bad connection";

    ascistream astrm( ((StreamConn&)conn).iStream() );
    std::istream& strm = astrm.stream();
    if ( !strm.good() )
	return "Cannot read from input file";
    if ( !astrm.isOfFileType(mTranslGroupName(RandomLineSet)) )
	return "Input file is not a RandomLineSet file";
    if ( atEndOfSection(astrm) )
	astrm.next();

    const bool isold = !astrm.hasKeyword( sKeyNrLines );
    const int nrlines = isold ? astrm.getVal() : 1;
    if ( !isold )
    {
	astrm.next();
	while ( !atEndOfSection(astrm.next())
	     && !astrm.hasKeyword(sKey::ZRange) )
	    rdls.pars().set( astrm.keyWord(), astrm.value() );
    }

    Interval<float> zrg;
    if ( !astrm.hasKeyword(sKey::ZRange) )
	assign( zrg, SI().zRange(true) );
    else
    {
	FileMultiString fms = astrm.value();
	zrg.start = atof( fms[0] );
	zrg.stop = atof( fms[1] );
    }

    while ( !atEndOfSection(astrm) )
	astrm.next();

    rdls.setZRange( zrg );

    for ( int iln=0; iln<nrlines; iln++ )
    {
	Geometry::RandomLine* rl = new Geometry::RandomLine;
	while ( !atEndOfSection(astrm) )
	{
	    BufferString loc( astrm.keyWord() );
	    char* ptr = loc.buf();
	    skipLeadingBlanks(ptr);
	    while ( *ptr && !isspace(*ptr) )
		ptr++;
	    if ( *ptr )
	    {
		*ptr++ = '\0'; skipLeadingBlanks(ptr);
		rl->addNode( BinID(atoi(loc.buf()),atoi(ptr)) );
	    }
	    astrm.next();
	}

	if ( rl->nrNodes() < 2 )
	    delete rl;
	else
	    rdls.addLine( rl );
    }

    return rdls.size() >= 1 ? 0 : "No valid random line found";
}


const char* dgbRandomLineSetTranslator::write(
			const Geometry::RandomLineSet& rdls, Conn& conn )
{
    const int nrlines = rdls.size();
    if ( nrlines < 1 )
	return "No random line to write";
    if ( !conn.forWrite() || !conn.isStream() )
	return "Internal error: bad connection";

    ascostream astrm( ((StreamConn&)conn).oStream() );
    astrm.putHeader( mTranslGroupName(RandomLineSet) );
    std::ostream& strm = astrm.stream();
    if ( !strm.good() )
	return "Cannot write to output RandomLineSet file";

    if ( nrlines != 1 || !rdls.pars().isEmpty() )
    {
	astrm.put( sKeyNrLines, nrlines );
	for ( int idx=0; idx<rdls.pars().size(); idx++ )
	    astrm.put( rdls.pars().getKey(idx), rdls.pars().getValue(idx) );
    }

    FileMultiString fms = Conv::to<const char*>( rdls.zRange().start );
    fms.add( Conv::to<const char*>(rdls.zRange().stop) );
    astrm.put( sKey::ZRange, fms );
    astrm.newParagraph();

    for ( int iln=0; iln<rdls.size(); iln++ )
    {
	const Geometry::RandomLine& rdl = *rdls.lines()[iln];
	for ( int idx=0; idx<rdl.nrNodes(); idx++ )
	{
	    const BinID bid = rdl.nodePosition( idx );
	    strm << bid.inl << '\t' << bid.crl << '\n';
	}
	astrm.newParagraph();
    }

    return strm.good() ? 0 : "Error during write of RandomLine Set file";
}
