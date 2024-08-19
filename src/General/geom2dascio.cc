/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "geom2dascio.h"

#include "od_istream.h"
#include "posinfo2d.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "tabledef.h"

#include <string.h>


Geom2dAscIO::Geom2dAscIO( const Table::FormatDesc& fd, od_istream& strm )
    : Table::AscIO( fd )
    , strm_( strm )
{
}


void Geom2dAscIO::fillDesc( Table::FormatDesc& fd, bool withline )
{
    fd.bodyinfos_.erase();
    if ( withline )
	fd.bodyinfos_ += new Table::TargetInfo( "Line Name", StringInpSpec(),
						Table::Required );
    fd.bodyinfos_ += new Table::TargetInfo( "Trace Nr", IntInpSpec() );
    fd.bodyinfos_ += new Table::TargetInfo( "SP Nr", IntInpSpec() );
    fd.bodyinfos_ +=
	Table::TargetInfo::mkHorPosition( true, false, true );
}


Table::FormatDesc* Geom2dAscIO::getDesc( bool withline )
{
    Table::FormatDesc* fd = new Table::FormatDesc( "Geom2D" );
    fillDesc( *fd, withline );
    return fd;
}


bool Geom2dAscIO::getData( PosInfo::Line2DData& geom )
{
    if ( !getHdrVals(strm_) )
	return false;

     while ( true )
     {
	const int ret = getNextBodyVals( strm_ );
	if ( ret < 0 ) return false;
	if ( ret == 0 ) break;

	const int trcnr = getIntValue(0);
	if ( mIsUdf(trcnr) )
	    continue;

	PosInfo::Line2DPos pos( trcnr );
	pos.coord_.x = getDValue( 2 );
	pos.coord_.y = getDValue( 3 );
	geom.add( pos );
     }

     return geom.positions().size();
}


bool Geom2dAscIO::readLine( int startidx, Coord& crd,
			    int& trcnr, float& spnr,
			    bool isxy, bool needsconv ) const
{
    trcnr = getIntValue( startidx );
    spnr = getFValue( startidx+1 );
    if ( mIsUdf(trcnr) && mIsUdf(spnr) )
    {
	errmsg_ = tr("Trace number or Shotpoint number has to be defined.");
	return false;
    }

    if ( mIsUdf(spnr) )
	spnr = -1;

    if ( isxy )
    {
	crd.setXY( getDValue(startidx+2), getDValue(startidx+3) );
	if ( needsconv )
	    crd = fd_.bodyinfos_.last()->convert( crd );
    }
    else
    {
	LatLong ll;
	ll.setFromString( getText(startidx+2).buf(), true );
	ll.setFromString( getText(startidx+3), false );
	crd = LatLong::transform( ll );
    }

    return true;
}


bool Geom2dAscIO::getData( Survey::Geometry2D& geom ) const
{
    if ( !getHdrVals(strm_) )
	return false;

    const bool needsconv = fd_.bodyinfos_.last()->needsConversion();
    const bool isxy = fd_.bodyinfos_.last()->selection_.form_ == 0;
    int trcidx = 0;
    while ( true )
    {
	const int ret = getNextBodyVals( strm_ );
	if ( ret < 0 ) return false;
	if ( ret == 0 ) break;

	Coord crd;
	int trcnr; float spnr;
	if ( !readLine(0,crd,trcnr,spnr,isxy,needsconv) )
	    continue;

	trcidx++;
	if ( mIsUdf(trcnr) )
	    trcnr = trcidx;

	geom.add( crd, trcnr, spnr );
    }

    geom.touch();
    return geom.size();
}


bool Geom2dAscIO::getData( ObjectSet<Survey::Geometry2D>& geoms ) const
{
    if ( !getHdrVals(strm_) )
	return false;

    const bool needsconv = fd_.bodyinfos_.last()->needsConversion();
    const bool isxy = fd_.bodyinfos_.last()->selection_.form_ == 0;
    if ( !isxy && !SI().getCoordSystem() )
    {
	errmsg_ = tr("The input file has positions defined as Lat/Long.\n"
		     "Please select a CRS for this project first.");
	return false;
    }

    Survey::Geometry2D* geom = 0;
    int trcidx = 0;
    while ( true )
    {
	const int ret = getNextBodyVals( strm_ );
	if ( ret < 0 ) return false;
	if ( ret == 0 ) break;

	const BufferString linenm = getText( 0 );
	if ( linenm.isEmpty() )
	    continue;

	if ( !geom || linenm != geom->getName() )
	{
	    geom = new Survey::Geometry2D( linenm );
	    geom->ref();
	    geoms += geom;
	    trcidx = 0;
	}

	Coord crd;
	int trcnr; float spnr;
	if ( !readLine(1,crd,trcnr,spnr,isxy,needsconv) )
	    continue;

	trcidx++;
	if ( mIsUdf(trcnr) )
	    trcnr = trcidx;

	geom->add( crd, trcnr, spnr );
    }

    for ( int idx=0; idx<geoms.size(); idx++ )
	geoms[idx]->touch();

    return true;
}



// SEGP1Header
SEGP1Header::SEGP1Header()
{}

SEGP1Header::~SEGP1Header()
{}


// SEGP1Entry
SEGP1Entry::SEGP1Entry()
{
}

SEGP1Entry::~SEGP1Entry()
{}


void SEGP1Entry::setLineName( const char* lnm )
{
    if ( !geom_ )
	geom_ = new Survey::Geometry2D( lnm );
}


static int sNrHeaderLines()		{ return 20; }

// SEGP1Importer
SEGP1Importer::SEGP1Importer( const char* fnm )
    : Executor("Reading SEG P1 data")
{
    strm_ = new od_istream( fnm );
    nrdonetxt_ = tr("Positions read");
}


SEGP1Importer::~SEGP1Importer()
{
    delete strm_;
}


void SEGP1Importer::setOrigin( const Coord& crd )
{
    origin_ = crd;
}


void SEGP1Importer::setUseLatLong( bool yn )
{
    uselatlong_ = yn;
}


void SEGP1Importer::setFalseEasting( float val )
{
    falseeasting_ = val;
}


void SEGP1Importer::setFalseNorthing( float val )
{
    falsenorthing_ = val;
}


bool SEGP1Importer::readHeader()
{
    return true;
}


static void parse( const char* src, int col0, int col1, BufferString& dest )
{
    dest.setEmpty();
    const int sz = col1 - col0 + 1;
    const int start = col0 - 1;
#ifdef __win__
    strncpy_s( dest.getCStr(), dest.bufSize(), src+start, sz );
#else
    strncpy( dest.getCStr(), src+start, sz );
#endif
    dest[sz] = '\0';
    dest.trimBlanks();
}


bool SEGP1Importer::readRecord( const BufferString& record )
{
    auto* entry = entries_.last();
    BufferString part( 32, false );

//  parse( record.buf(), 1, 1, part ); // Data record identifier

    parse( record.buf(), 2, 17, part ); // Line name
    if ( entry->geom_ && part != entry->geom_->getName() )
    {
	auto* newentry = new SEGP1Entry;
	entries_ += newentry;
	newentry->header_ = entry->header_;
	entry = newentry;
    }

    entry->setLineName( part.buf() );
    Survey::Geometry2D* geom = entry->geom_;
    if ( !geom )
	return false;

    parse( record.buf(), 18, 25, part ); // Shotpoint number
    const int trcnr = toInt( part.buf() );
    const float spnr = toFloat( part.buf() );

    parse( record.buf(), 26, 26, part ); // Reshoot code

    Coord crd;
    if ( uselatlong_ )
    {
	LatLong ll;
	parse( record.buf(), 27, 35, part ); // Latitude
	ll.setFromString( part.buf(), true );
	parse( record.buf(), 36, 45, part ); // Longitude
	ll.setFromString( part.buf(), false );
	crd = LatLong::transform( ll );
    }
    else
    {
	parse( record.buf(), 46, 53, part ); // Map grid Easting
	crd.x = toDouble( part.buf() ) / 10;
	parse( record.buf(), 54, 61, part ); // Map grid Northing
	crd.y = toDouble( part.buf() ) / 10;
	crd += origin_;
    }

    parse( record.buf(), 62, 66, part ); // Water depth or elev above sealevel
    parse( record.buf(), 67, 68, part ); // Year
    parse( record.buf(), 69, 71, part ); // Day of year
    parse( record.buf(), 72, 73, part ); // Hours
    parse( record.buf(), 74, 75, part ); // Minutes
    parse( record.buf(), 76, 77, part ); // Seconds
    parse( record.buf(), 78, 80, part ); // Spare

    geom->add( crd, trcnr, spnr );

    return true;
}


int SEGP1Importer::nextStep()
{
    BufferString linestr;
    if ( !strm_->getLine(linestr) )
	return Finished();

    if ( linestr.isEmpty() )
	return MoreToDo();

    if ( linestr[0] == 'H' )
    {
	auto* entry = new SEGP1Entry;
	entries_ += entry;
	entry->header_.add( linestr.buf() );
	for ( int idx=1; idx<sNrHeaderLines(); idx++ )
	{
	    strm_->getLine( linestr );
	    entry->header_.add( linestr.buf() );
	}

	return MoreToDo();
    }

    readRecord( linestr );
    return MoreToDo();
}
