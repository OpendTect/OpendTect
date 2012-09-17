/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		December 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: randomlinetr.cc,v 1.12 2011/02/01 19:57:22 cvskris Exp $";

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
    if ( bs.isEmpty() )
    {
	for ( int iln=0; iln<rdls.lines().size(); iln++ )
	{
	    const Geometry::RandomLine& rdl = *rdls.lines()[iln];
	    if ( rdl.name().isEmpty() )
	    {
		BufferString nm( ioobj->name() );
		if ( rdls.lines().size() > 1 )
		    { nm += " - "; nm += iln + 1; }
		const_cast<Geometry::RandomLine&>(rdl).setName( nm );
	    }
	}
	return true;
    }

    return false;
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


static void getZRgAndName( ascistream& astrm, Interval<float>& zrg,
			   BufferString& nm )
{
    if ( !astrm.hasKeyword(sKey::ZRange) )
	return;

    FileMultiString fms = astrm.value();
    zrg.start = toFloat( fms[0] ); zrg.stop = toFloat( fms[1] );
    astrm.next();

    if ( astrm.hasKeyword(sKey::Name) )
    {
	nm = astrm.value();
	astrm.next();
    }
}


static void putZRangeAndName( ascostream& astrm,
			      const Geometry::RandomLine& rdl )
{
    const Interval<float> zrg( rdl.zRange() );
    FileMultiString fms = toString( zrg.start );
    fms.add( toString(zrg.stop) );
    astrm.put( sKey::ZRange, fms );
    if ( !rdl.name().isEmpty() )
	astrm.put( sKey::Name, rdl.name() );
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
    const int nrlines = issimple ? 1 : astrm.getIValue();
    Interval<float> zrg; assign( zrg, SI().zRange(true) );
    BufferString rlnm;
    if ( issimple )
	getZRgAndName( astrm, zrg, rlnm );
    else
    {
	while ( !atEndOfSection(astrm.next()) )
	    rdls.pars().set( astrm.keyWord(), astrm.value() );
    }
    astrm.next();

    for ( int iln=0; iln<nrlines; iln++ )
    {
	Geometry::RandomLine* rl = new Geometry::RandomLine;
	if ( !issimple )
	    getZRgAndName( astrm, zrg, rlnm );
	rl->setZRange( zrg ); rl->setName( rlnm );

	while ( !atEndOfSection(astrm) )
	{
	    BufferString loc( astrm.keyWord() );
	    char* ptr = loc.buf();
	    mSkipBlanks(ptr); mSkipNonBlanks(ptr);
	    if ( *ptr )
	    {
		*ptr++ = '\0'; mSkipBlanks(ptr);
		rl->addNode( BinID(toInt(loc.buf()),toInt(ptr)) );
	    }
	    astrm.next();
	}

	if ( rl->nrNodes() < 2 )
	    delete rl;
	else
	    rdls.addLine( rl );
	astrm.next();
    }

    return rdls.size() >= 1 ? 0 : "No valid random line in set";
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
	putZRangeAndName( astrm, *rdls.lines()[0] );
    else
    {
	astrm.put( sKeyNrLines, nrlines );
	for ( int idx=0; idx<rdls.pars().size(); idx++ )
	    astrm.put( rdls.pars().getKey(idx), rdls.pars().getValue(idx) );
    }
    astrm.newParagraph();

    for ( int iln=0; iln<nrlines; iln++ )
    {
	const Geometry::RandomLine& rdl = *rdls.lines()[iln];
	if ( !issimple )
	    putZRangeAndName( astrm, rdl );

	for ( int idx=0; idx<rdl.nrNodes(); idx++ )
	{
	    const BinID bid = rdl.nodePosition( idx );
	    strm << bid.inl << '\t' << bid.crl << '\n';
	}
	astrm.newParagraph();
    }

    return strm.good() ? 0 : "Error during write of RandomLine Set file";
}
