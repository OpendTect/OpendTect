/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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

mDefSimpleTranslatorSelector(RandomLineSet)
mDefSimpleTranslatorioContext(RandomLineSet,Loc)


bool RandomLineSetTranslator::retrieve( Geometry::RandomLineSet& rdls,
				     const IOObj* ioobj, uiString& errmsg )
{
    if ( !ioobj )
    {
	errmsg = uiStrings::phrCannotFindObjInDB();
	return false;
    }

    PtrMan<RandomLineSetTranslator> trnsl
	= dynamic_cast<RandomLineSetTranslator*>(ioobj->createTranslator());
    if ( !trnsl )
    {
	errmsg = tr("Selected object is not a Random Line");
	return false;
    }

    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
    if ( !conn )
    {
	errmsg = uiStrings::phrCannotOpen( ioobj->fullUserExpr(), true );
	return false;
    }

    errmsg = trnsl->read( rdls, *conn );
    if ( errmsg.isEmpty() )
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
				  const IOObj* ioobj, uiString& errmsg )
{
    if ( !ioobj )
    {
	errmsg = tr("No object to store set in data base");
	return false;
    }

    PtrMan<RandomLineSetTranslator> trnsl
	= dynamic_cast<RandomLineSetTranslator*>(ioobj->createTranslator());
    if ( !trnsl )
    {
	errmsg = tr("Selected object is not an Attribute Set");
	return false;
    }

    PtrMan<Conn> conn = ioobj->getConn( Conn::Write );
    if ( !conn )
    {
	errmsg = uiStrings::phrCannotOpen( ioobj->fullUserExpr(false), false );
	return false;
    }

    errmsg = trnsl->write( rdl, *conn );
    return errmsg.isEmpty();
}


static void getZRgAndName( ascistream& astrm, Interval<float>& zrg,
			   BufferString& nm )
{
    if ( !astrm.hasKeyword(sKey::ZRange()) )
	return;

    FileMultiString fms = astrm.value();
    zrg.start_ = fms.getFValue( 0 ); zrg.stop_ = fms.getFValue( 1 );
    astrm.next();

    if ( astrm.hasKeyword(sKey::Name()) )
    {
	nm = astrm.value();
	astrm.next();
    }
}


static void putZRangeAndName( ascostream& astrm,
			      const Geometry::RandomLine& rdl )
{
    const Interval<float> zrg( rdl.zRange() );
    FileMultiString fms = toString( zrg.start_ );
    fms.add( toString(zrg.stop_) );
    astrm.put( sKey::ZRange(), fms );
    if ( !rdl.name().isEmpty() )
	astrm.put( sKey::Name(), rdl.name() );
}


uiString dgbRandomLineSetTranslator::read( Geometry::RandomLineSet& rdls,
					   Conn& conn )
{
    if ( !conn.forRead() || !conn.isStream() )
    {
	pErrMsg("Internal error: bad connection");
	return uiStrings::phrCannotConnectToDB();
    }

    ascistream astrm( ((StreamConn&)conn).iStream() );
    if ( !astrm.isOK() )
	return tr("Cannot read from input file");
    if ( !astrm.isOfFileType(mTranslGroupName(RandomLineSet)) )
	return tr("Input file is not a RandomLineSet file");
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
	RefMan<Geometry::RandomLine> rl = new Geometry::RandomLine;
	if ( !issimple )
	    getZRgAndName( astrm, zrg, rlnm );
	rl->setZRange( zrg ); rl->setName( rlnm );

	while ( !atEndOfSection(astrm) )
	{
	    BufferString loc( astrm.keyWord() );
	    char* ptr = loc.getCStr();
	    mSkipBlanks(ptr); mSkipNonBlanks(ptr);
	    if ( *ptr )
	    {
		*ptr++ = '\0'; mSkipBlanks(ptr);
		rl->addNode( BinID(loc.toInt(),toInt(ptr)) );
	    }
	    astrm.next();
	}

	if ( rl->nrNodes() > 1 )
	    rdls.addLine( *rl );

	astrm.next();
    }

    return rdls.size() >= 1 ? uiString::empty()
			    : tr("No valid random line in set");
}


uiString dgbRandomLineSetTranslator::write(
			const Geometry::RandomLineSet& rdls, Conn& conn )
{
    const int nrlines = rdls.size();
    if ( nrlines < 1 )
	return tr("No random line to write");
    if ( !conn.forWrite() || !conn.isStream() )
    {
	pErrMsg("Internal error: bad connection");
	return uiStrings::phrCannotConnectToDB();
    }

    ascostream astrm( ((StreamConn&)conn).oStream() );
    astrm.putHeader( mTranslGroupName(RandomLineSet) );
    if ( !astrm.isOK() )
	return tr("Cannot write to output RandomLineSet file");

    const bool issimple = nrlines < 2 && rdls.pars().isEmpty();
    if ( issimple )
	putZRangeAndName( astrm, *rdls.lines()[0] );
    else
    {
	astrm.put( sKeyNrLines, nrlines );
	IOParIterator iter( rdls.pars() );
	BufferString key, val;
	while ( iter.next(key,val) )
	    astrm.put( key, val );
    }
    astrm.newParagraph();

    od_ostream& strm = astrm.stream();
    for ( int iln=0; iln<nrlines; iln++ )
    {
	const Geometry::RandomLine& rdl = *rdls.lines()[iln];
	if ( !issimple )
	    putZRangeAndName( astrm, rdl );

	for ( int idx=0; idx<rdl.nrNodes(); idx++ )
	{
	    const BinID bid = rdl.nodePosition( idx );
	    strm << bid.inl() << od_tab << bid.crl() << od_newline;
	}
	astrm.newParagraph();
    }

    return strm.isOK() ? uiString::empty()
		       : tr("Error during write of RandomLine Set file");
}
