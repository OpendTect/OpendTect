/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		December 2006
 RCS:		$Id: randomlinetr.cc,v 1.4 2007-11-06 16:31:49 cvsbert Exp $
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


static void getZRange( ascistream& astrm, Interval<float>& zrg )
{
    if ( !astrm.hasKeyword(sKey::ZRange) )
	return;

    FileMultiString fms = astrm.value();
    zrg.start = atof( fms[0] );
    zrg.stop = atof( fms[1] );
    astrm.next();
}


static void putZRange( ascostream& astrm, const Interval<float>& zrg )
{
    FileMultiString fms = Conv::to<const char*>( zrg.start );
    fms.add( Conv::to<const char*>(zrg.stop) );
    astrm.put( sKey::ZRange, fms );
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

    const bool issimple = !astrm.hasKeyword( sKeyNrLines );
    const int nrlines = issimple ? astrm.getVal() : 1;
    Interval<float> zrg; assign( zrg, SI().zRange(true) );
    if ( issimple )
	getZRange( astrm, zrg );
    else
    {
	while ( !atEndOfSection(astrm.next()) )
	    rdls.pars().set( astrm.keyWord(), astrm.value() );
    }
    astrm.next();

    for ( int iln=0; iln<nrlines; iln++ )
    {
	Geometry::RandomLine* rl = new Geometry::RandomLine;
	getZRange( astrm, zrg ); rl->setZRange( zrg );
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

    const bool issimple = nrlines < 2 && rdls.pars().isEmpty();
    if ( issimple )
    {
	putZRange( astrm, rdls.lines()[0]->zRange() );
	astrm.newParagraph();
    }
    else
    {
	astrm.put( sKeyNrLines, nrlines );
	for ( int idx=0; idx<rdls.pars().size(); idx++ )
	    astrm.put( rdls.pars().getKey(idx), rdls.pars().getValue(idx) );
    }

    for ( int iln=0; iln<nrlines; iln++ )
    {
	const Geometry::RandomLine& rdl = *rdls.lines()[iln];
	if ( !issimple )
	    putZRange( astrm, rdl.zRange() );

	for ( int idx=0; idx<rdl.nrNodes(); idx++ )
	{
	    const BinID bid = rdl.nodePosition( idx );
	    strm << bid.inl << '\t' << bid.crl << '\n';
	}
	astrm.newParagraph();
    }

    return strm.good() ? 0 : "Error during write of RandomLine Set file";
}
