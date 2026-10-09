/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "horizonscanner.h"

#include "binidvalset.h"
#include "emhorizonascio.h"
#include "file.h"
#include "iopar.h"
#include "od_istream.h"
#include "oddirs.h"
#include "paralleltask.h"
#include "posinfodetector.h"
#include "ptrman.h"
#include "survinfo.h"
#include "tabledef.h"
#include "unitofmeasure.h"
#include "zaxistransform.h"

#include <algorithm>
#include <istream>
#include <streambuf>


HorizonScanner::HorizonScanner( const BufferStringSet& fnms,
				Table::FormatDesc& fd, bool isgeom )
    : ReportingTask("Scan horizon file(s)")
    , dtctor_(*new PosInfo::Detector(PosInfo::Detector::Setup(false)))
    , isgeom_(isgeom)
    , fd_(fd)
    , zinfo_(SI().zDomainInfo())
    , curmsg_(tr("Scanning"))
{
    filenames_ = fnms;
    init();
}


HorizonScanner::HorizonScanner( const BufferStringSet& fnms,
				Table::FormatDesc& fd, bool isgeom,
				const ZDomain::Info& zinfo )
    : ReportingTask("Scan horizon file(s)")
    , dtctor_(*new PosInfo::Detector(PosInfo::Detector::Setup(false)))
    , isgeom_(isgeom)
    , fd_(fd)
    , zinfo_(zinfo)
{
    filenames_ = fnms;
    init();
}


HorizonScanner::~HorizonScanner()
{
    delete &dtctor_;
    deepErase( sections_ );
}


void HorizonScanner::init()
{
    totalnr_ = -1;
    nrdone_ = -1;
    valranges_.erase();
    dtctor_.reInit();
    analyzeData();
}


uiString HorizonScanner::uiMessage() const
{
    return curmsg_;
}


uiString HorizonScanner::uiNrDoneText() const
{
    return tr("Positions handled");
}


od_int64 HorizonScanner::nrDone() const
{
    return nrdone_.load();
}


od_int64 HorizonScanner::totalNr() const
{
    return totalnr_.load();
}


void HorizonScanner::report( StringPairSet& report ) const
{
    report.setEmpty();

    const int firstattribidx = isgeom_ ? 1 : 0;
    BufferString str = "Report for horizon file(s):\n";
    for ( int idx=0; idx<filenames_.size(); idx++ )
    {
	if ( idx > 0 )
	    str += "\n";

	str += filenames_.get(idx).buf();
    }

    report.setName( str.buf() );

    report.add( StringPairSet::sKeyH1(), "Geometry" );
    dtctor_.report( report );
    if ( isgeom_ && valranges_.size() > 0 )
    {
	BufferString zrgkey( zinfo_.userName().getFullString() );
	zrgkey.addSpace().add( zinfo_.unitStr(true) );
	Interval<float> zrg = valranges_[0];
	zrg.scale( zinfo_.userFactor() );
	BufferString zrgstr;
	zrgstr.add( toString(zrg.start_,0,'f',SI().nrZDecimals()) )
	      .add( " - " ).add( toString(zrg.stop_,0,'f',SI().nrZDecimals()) );
	report.add( zrgkey, zrgstr );
    }

    if ( valranges_.size() > firstattribidx )
    {
	report.add( StringPairSet::sKeyH2(), "Data values" );
	for ( int idx=firstattribidx; idx<valranges_.size(); idx++ )
	{
	    const char* attrnm = fd_.bodyinfos_[idx+1]->name().buf();
	    report.add( IOPar::compKey(attrnm,"Minimum value"),
		       valranges_[idx].start_ );
	    report.add( IOPar::compKey(attrnm,"Maximum value"),
		       valranges_[idx].stop_ );
	}
    }
    else
	report.add( StringPairSet::sKeyH2(), "No attribute data values" );

    if ( nrPositions() == 0 )
    {
	report.add( "No Valid positions found",
		   "Please re-examine input file and format definition" );
	return;
    }

    if ( !rejectedlines_.isEmpty() )
    {
	report.add( StringPairSet::sKeyH1(), "Warning" );
	report.add( "These positions were rejected", "" );
	for ( int idx=0; idx<rejectedlines_.size(); idx++ )
	    report.add( toString(idx), rejectedlines_.get(idx).buf() );
    }
}


mStartAllowDeprecatedSection

void HorizonScanner::report( IOPar& iopar ) const
{
    iopar.setEmpty();

    const int firstattribidx = isgeom_ ? 1 : 0;
    BufferString str = "Report for horizon file";
    str += filenames_.size() > 1 ? "s:\n" : ": ";
    for ( int idx=0; idx<filenames_.size(); idx++ )
	{ str += filenames_.get(idx).buf(); str += "\n"; }
    str += "\n\n";
    iopar.setName( str.buf() );

    iopar.add( IOPar::sKeyHdr(), "Geometry" );
    dtctor_.report( iopar );
    if ( isgeom_ && valranges_.size() > 0 )
    {
	BufferString zrgkey( zinfo_.userName().getFullString() );
	zrgkey.addSpace().add( zinfo_.unitStr(true) );
	Interval<float> zrg = valranges_[0];
	zrg.scale( zinfo_.userFactor() );
	BufferString zrgstr;
	zrgstr.add( toString(zrg.start_,0,'f',SI().nrZDecimals()) )
	      .add( " - " ).add( toString(zrg.stop_,0,'f',SI().nrZDecimals()) );
	iopar.set( zrgkey, zrgstr );
    }

    if ( valranges_.size() > firstattribidx )
    {
	iopar.add( IOPar::sKeySubHdr(), "Data values" );
	for ( int idx=firstattribidx; idx<valranges_.size(); idx++ )
	{
	    const char* attrnm = fd_.bodyinfos_[idx+1]->name().buf();
	    iopar.set( IOPar::compKey(attrnm,"Minimum value"),
		       valranges_[idx].start_ );
	    iopar.set( IOPar::compKey(attrnm,"Maximum value"),
		       valranges_[idx].stop_ );
	}
    }
    else
	iopar.add( IOPar::sKeySubHdr(), "No attribute data values" );

    if ( nrPositions() == 0 )
    {
	iopar.add( "No Valid positions found",
		   "Please re-examine input file and format definition" );
	return;
    }

    if ( !rejectedlines_.isEmpty() )
    {
	iopar.add( IOPar::sKeyHdr(), "Warning" );
	iopar.add( "These positions were rejected", "" );
	for ( int idx=0; idx<rejectedlines_.size(); idx++ )
	    iopar.add( toString(idx), rejectedlines_.get(idx).buf() );
    }
}

mStopAllowDeprecatedSection


const char* HorizonScanner::defaultUserInfoFile()
{
    mDeclStaticString( ret );
    ret = GetProcFileName( "scan_horizon" );
    if ( GetSoftwareUser() )
    {
	ret += "_";
	ret += GetSoftwareUser();
    }

    ret += ".txt";
    return ret.buf();
}


void HorizonScanner::launchBrowser( const char* fnm ) const
{
    if ( !fnm || !*fnm )
	fnm = defaultUserInfoFile();

    StringPairSet rep;
    report( rep );
    rep.write( fnm );
    File::launchViewer( fnm );
}


bool HorizonScanner::reInitAscIO( const char* fnm )
{
    ascio_ = new EM::Horizon3DAscIO( fd_, fnm );
    if ( !ascio_ || !ascio_->isOK() )
    {
	deleteAndNullPtr( ascio_ );
	return false;
    }

    return true;
}



void HorizonScanner::getConvValue( float& zval )
{
    if ( !ascio_ && !reInitAscIO(filenames_.get(0).buf()) )
	return;

    const UnitOfMeasure* fromuom = zinfo_.isDepth() ?
					Table::AscIO::getDepthUnit() :
						Table::AscIO::getTimeUnit();
    convValue( zval, fromuom, UnitOfMeasure::zUnit(zinfo_) );
}


bool HorizonScanner::analyzeData()
{
    if ( !reInitAscIO( filenames_.get(0).buf() ) )
	return false;

    const Interval<float> validrg( zinfo_.getReasonableZRange() );
    int maxcount = 100;
    int count, nrxy, nrbid, nrvalid, nrnotvalid;
    count = nrxy = nrbid = nrvalid = nrnotvalid = 0;
    Coord crd;
    float val;
    TypeSet<float> data;
    selxy_ = ascio_->isXY();
    while ( ascio_->getNextLine(crd,data) > 0 )
    {
	if ( data.isEmpty() )
	    break;

	if ( count > maxcount )
	    break;

        BinID bid( mNINT32(crd.x_), mNINT32(crd.y_) );

	bool validplacement = false;
	if ( selxy_ )
	{
	    if ( SI().isReasonable(crd) )
	    {
		nrxy++;
		validplacement=true;
	    }
	    else if ( SI().isReasonable(bid) )
	    {
		nrbid++;
		validplacement=true;
	    }
	}
	else
	{
	    if ( SI().isReasonable(bid) )
	    {
		nrbid++;
		validplacement=true;
	    }
	    else if ( SI().isReasonable(crd) )
	    {
		nrxy++;
		validplacement=true;
	    }
	}

	const BinID selbid = selxy_ ? SI().transform( crd ) : bid;
	val = data[0];
	bool validvert = false;
	if ( !mIsUdf(val) )
	{
	    getConvValue( val );
	    validvert = true;
	    if ( validrg.includes(val,false) )
		nrvalid++;
	    else
		nrnotvalid++;
	}

	if ( validplacement && validvert )
	    count++;
    }

    const bool apparentisxy = nrxy > nrbid;
    if ( apparentisxy != selxy_ )
    {
	curmsg_ = tr("You have selected positions in %1, "
		     "but the positions in file appear to be in %2.")
		     .arg( selxy_ ? "X/Y" : "Inl/Crl" )
		     .arg( apparentisxy ? "X/Y" : "Inl/Crl" );
    }

    if ( nrnotvalid > nrvalid )
    {
	const UnitOfMeasure* selzunit = ascio_->getSelZUnit();

	uiString zmsg = tr("You have selected Z in %1. "
			   "In this unit many Z values "
			   "appear to be outside the survey range.")
			   .arg( selzunit->name() );
	if ( curmsg_.isEmpty() )
	    curmsg_ = zmsg;
	else
	    curmsg_.append( zmsg, true );
    }

    isxy_ = selxy_;
    deleteAndNullPtr( ascio_ );
    return true;
}


bool HorizonScanner::isInsideSurvey( const BinID& bid, float zval ) const
{
    if ( !SI().isReasonable(bid) )
	return false;

    ZGate zrg = zinfo_.getReasonableZRange();
    return zrg.includes( zval, false );
}


namespace
{

class CharRangeBuf : public std::streambuf
{
public:
CharRangeBuf( const char* start, od_int64 len )
{
    char* buf = const_cast<char*>( start );
    setg( buf, buf, buf + len );
}

protected:
std::streampos seekoff( std::streamoff off, std::ios_base::seekdir way,
			std::ios_base::openmode which ) override
{
    if ( (which & std::ios_base::in) == 0 )
	return std::streampos(-1);

    char* ptr = nullptr;
    if ( way == std::ios_base::beg )
	ptr = eback() + off;
    else if ( way == std::ios_base::cur )
	ptr = gptr() + off;
    else
	ptr = egptr() + off;

    if ( ptr < eback() || ptr > egptr() )
	return std::streampos(-1);

    setg( eback(), ptr, egptr() );
    return std::streampos( ptr - eback() );
}

std::streampos seekpos( std::streampos pos,
			std::ios_base::openmode which ) override
{
    return seekoff( off_type(pos), std::ios_base::beg, which );
}

};


class CharRangeStream : public std::istream
{
public:
CharRangeStream( const char* start, od_int64 len )
    : std::istream(nullptr)
    , buf_(start, len)
{
    rdbuf( &buf_ );
}

private:
    CharRangeBuf	buf_;
};


class LineStartFinder : public ParallelTask
{
public:
LineStartFinder( const char* text, od_int64 bodystart, od_int64 filesz,
		 TypeSet<od_int64>& starts )
    : ParallelTask("Index horizon file")
    , text_(text)
    , bodystart_(bodystart)
    , filesz_(filesz)
    , starts_(starts)
{}

od_int64 nrIterations() const override
{
    return filesz_ - bodystart_;
}

int minThreadSize() const override
{
    return 256 * 1024;
}

bool doPrepare( int nrthreads ) override
{
    deepErase( perthread_ );
    for ( int idx=0; idx<nrthreads; idx++ )
	perthread_ += new TypeSet<od_int64>;

    return true;
}

bool doWork( od_int64 start, od_int64 stop, int threadidx ) override
{
    TypeSet<od_int64>& out = *perthread_[threadidx];
    const od_int64 absstart = bodystart_ + start;
    const od_int64 absstop = bodystart_ + stop;
    od_int64 ich = absstart;
    if ( absstart > bodystart_ && text_[absstart-1] != '\n' )
    {
	while ( ich<=absstop && ich<filesz_ && text_[ich] != '\n' )
	    ich++;

	ich++;
    }

    while ( ich<=absstop && ich<filesz_ )
    {
	out += ich;
	while ( ich<=absstop && ich<filesz_ && text_[ich] != '\n' )
	    ich++;

	ich++;
    }

    return true;
}

bool doFinish( bool success ) override
{
    if ( !success )
	return false;

    for ( int idx=0; idx<perthread_.size(); idx++ )
	starts_.append( *perthread_[idx] );

    return true;
}

private:
    const char*			text_;
    od_int64			bodystart_;
    od_int64			filesz_;
    TypeSet<od_int64>&		starts_;
    ObjectSet<TypeSet<od_int64>> perthread_;
};


class ParsedChunk
{
public:
    TypeSet<int>		inls;
    TypeSet<int>		crls;
    TypeSet<float>		vals;
    TypeSet<od_int64>		seq;
    TypeSet<Interval<float>>	ranges;
    BufferStringSet		rejected;
    bool			failed		= false;
};


static void convZ( float& zval, const ZDomain::Info& zinfo )
{
    const UnitOfMeasure* fromuom = zinfo.isDepth()
		? Table::AscIO::getDepthUnit() : Table::AscIO::getTimeUnit();
    convValue( zval, fromuom, UnitOfMeasure::zUnit(zinfo) );
}


static bool insideSurvey( const BinID& bid, float zval,
			  const ZDomain::Info& zinfo )
{
    if ( !SI().isReasonable(bid) )
	return false;

    const ZGate zrg = zinfo.getReasonableZRange();
    return zrg.includes( zval, false );
}


class BodyParser : public ParallelTask
{
public:
BodyParser( const char* text, od_int64 filesz,
	    const TypeSet<od_int64>& lineoffs, const Table::FormatDesc& fd,
	    float udfval, bool isxy, bool isgeom, const ZDomain::Info& zinfo,
	    int nrvals, Threads::Atomic<od_int64>& nrdone )
    : ParallelTask("Interpret horizon file")
    , text_(text)
    , filesz_(filesz)
    , lineoffs_(lineoffs)
    , fd_(fd)
    , udfval_(udfval)
    , isxy_(isxy)
    , isgeom_(isgeom)
    , zinfo_(zinfo)
    , nrvals_(nrvals)
    , nrdone_(nrdone)
{}

~BodyParser()
{
    deepErase( chunks_ );
}

od_int64 nrIterations() const override
{
    return lineoffs_.size();
}

int minThreadSize() const override
{
    return 1000;
}

bool doPrepare( int nrthreads ) override
{
    deepErase( chunks_ );
    for ( int idx=0; idx<nrthreads; idx++ )
    {
	auto* chunk = new ParsedChunk;
	chunk->ranges.setSize( nrvals_,
			       Interval<float>(mUdf(float), -mUdf(float)) );
	chunks_ += chunk;
    }

    return nrvals_ > 0;
}

bool doWork( od_int64 start, od_int64 stop, int threadidx ) override
{
    ParsedChunk& chunk = *chunks_[threadidx];
    const od_int64 from = lineoffs_[(int)start];
    const od_int64 to = stop+1 < lineoffs_.size()
			? lineoffs_[(int)stop+1] : filesz_;
    CharRangeStream crs( text_+from, to-from );
    od_istream istrm( crs );
    EM::Horizon3DAscIO aio( fd_, istrm );
    aio.prepareForBody( udfval_ );

    Coord crd;
    TypeSet<float> data;
    od_int64 nlocal = 0;
    static int nlocalmax = 4096;
    for ( od_int64 iline=start; iline<=stop; iline++ )
    {
	if ( (iline-start) % 4096 == 0 && !shouldContinue() )
	    return false;

	const int ret = aio.getNextLine( crd, data );
	if ( ret < 0 )
	{
	    chunk.failed = true;
	    return false;
	}

	if ( ret == 0 )
	    break;

	BinID bid;
	if ( isxy_ )
	    bid = SI().transform( crd );
	else
	{
	    bid.inl() = mNINT32( crd.x_ );
	    bid.crl() = mNINT32( crd.y_ );
	}

	if ( !SI().isReasonable(bid) )
	    continue;

	if ( !data.isEmpty() )
	    convZ( data[0], zinfo_ );

	bool validpos = true;
	for ( int validx=0; validx<data.size(); validx++ )
	{
	    const float val = data[validx];
	    if ( isgeom_ && validx==0 && !insideSurvey(bid, val, zinfo_) )
	    {
		validpos = false;
		break;
	    }

	    if ( !mIsUdf(val) && chunk.ranges.validIdx(validx) )
		chunk.ranges[validx].include( val, false );
	}

	if ( validpos && data.isEmpty() )
	    validpos = false;

	if ( validpos )
	{
	    chunk.inls += bid.inl();
	    chunk.crls += bid.crl();
	    chunk.seq += iline;
	    for ( int iv=0; iv<nrvals_; iv++ )
		chunk.vals += data.validIdx(iv) ? data[iv] : mUdf(float);
	}
	else if ( chunk.rejected.size() < 1024 )
	{
	    BufferString rej( "", crd.x_, "\t" );
	    rej += crd.y_;
	    if ( isgeom_ && !data.isEmpty() )
		{ rej += "\t"; rej += data[0]; }
	    chunk.rejected.add( rej );
	}

	nlocal++;
	if ( nlocal >= nlocalmax )
	{
	    nrdone_ += nlocal;
	    nlocal = 0;
	}
    }

    if ( nlocal )
	nrdone_ += nlocal;

    return !chunk.failed;
}

    ObjectSet<ParsedChunk>	chunks_;

private:
    const char*			text_;
    od_int64			filesz_;
    const TypeSet<od_int64>&	lineoffs_;
    const Table::FormatDesc&	fd_;
    float			udfval_;
    bool			isxy_;
    bool			isgeom_;
    const ZDomain::Info&	zinfo_;
    int				nrvals_;
    Threads::Atomic<od_int64>&	nrdone_;
};

} // namespace


bool HorizonScanner::execute()
{
    curmsg_.setEmpty();
    deepErase( sections_ );
    rejectedlines_.setEmpty();
    valranges_.erase();
    nrdone_ = 0;
    totalnr_ = 0;
    dtctor_.reInit();

    for ( int idx=0; idx<filenames_.size(); idx++ )
    {
	if ( !importFile(filenames_.get(idx).buf()) )
	{
	    dtctor_.finish();
	    return false;
	}
    }

    for ( int idx=0; idx<sections_.size(); idx++ )
    {
	PosInfo::Detector* secdtctr = !idx ? &dtctor_
	    : new PosInfo::Detector( PosInfo::Detector::Setup(false) );
	const BinIDValueSet& bivs = *sections_[idx];
	BinID bid;
	BinIDValueSet::SPos pos;
	while ( bivs.next(pos) )
	{
	    bid = bivs.getBinID( pos );
	    secdtctr->add( SI().transform(bid), bid );
	}

	secdtctr->finish();
	if ( idx )
	{
	    dtctor_.mergeResults( *secdtctr );
	    delete secdtctr;
	}
    }

    return true;
}


bool HorizonScanner::importFile( const char* fnm )
{
    const int nrvals = fd_.bodyinfos_.size() - 1;
    if ( nrvals < 1 )
    {
	curmsg_ = tr("Not enough data read to analyze");
	return false;
    }

    od_istream filestrm( fnm );
    if ( !filestrm.isOK() )
    {
	curmsg_ = tr("Error during initialization."
		     "\nPlease check the format definition");
	return false;
    }

    const od_int64 filesz = File::getFileSize( fnm );
    if ( filesz < 0 )
    {
	curmsg_ = tr("Error during initialization."
		     "\nPlease check the format definition");
	return false;
    }

    mDeclareAndTryAlloc( char*, text, char[filesz+1] );
    if ( !text )
    {
	curmsg_ = tr("Not enough memory to read the horizon file");
	return false;
    }

    if ( filesz>0 && !filestrm.getBin(text, filesz) )
    {
	delete [] text;
	curmsg_ = tr("Error during initialization."
		     "\nPlease check the format definition");
	return false;
    }

    text[filesz] = '\0';

    CharRangeStream crs( text, filesz );
    od_istream memstrm( crs );
    EM::Horizon3DAscIO headerio( fd_, memstrm );
    if ( !headerio.readHeader() )
    {
	delete [] text;
	curmsg_ = tr("Error during initialization."
		     "\nPlease check the format definition");
	return false;
    }

    od_int64 bodystart = memstrm.position();
    if ( bodystart<0 || bodystart>filesz )
	bodystart = 0;

    TypeSet<od_int64> lineoffs;
    if ( bodystart < filesz )
    {
	LineStartFinder indexer( text, bodystart, filesz, lineoffs );
	if ( !indexer.execute() )
	{
	    delete [] text;
	    curmsg_ = tr("Error during data interpretation."
			 "\nPlease check the format definition");
	    return false;
	}
    }

    totalnr_ = nrdone_.load() + lineoffs.size();
    auto* bvs = new BinIDValueSet( nrvals, false );
    bvs->allowDuplicateBinIDs( true );
    if ( !lineoffs.isEmpty() )
    {
	BodyParser parser( text, filesz, lineoffs, fd_, headerio.udfValue(),
			   isxy_, isgeom_, zinfo_, nrvals, nrdone_ );
	if ( !parser.execute() )
	{
	    delete bvs;
	    delete [] text;
	    curmsg_ = tr("Error during data interpretation."
			 "\nPlease check the format definition");
	    return false;
	}

	struct CommitKey
	{
	    bool operator==( const CommitKey& o ) const
	    {
		return inl==o.inl && crl==o.crl && seq==o.seq
		    && chunk==o.chunk && idx==o.idx;
	    }

	    int		inl;
	    int		crl;
	    od_int64	seq;
	    int		chunk;
	    int		idx;
	};

	TypeSet<CommitKey> keys;
	for ( int ich=0; ich<parser.chunks_.size(); ich++ )
	{
	    const ParsedChunk& chunk = *parser.chunks_[ich];
	    if ( chunk.failed )
	    {
		delete bvs;
		delete [] text;
		curmsg_ = tr("Error during data interpretation."
			     "\nPlease check the format definition");
		return false;
	    }

	    if ( valranges_.isEmpty() )
		valranges_.setSize( nrvals,
			    Interval<float>(mUdf(float), -mUdf(float)) );

	    for ( int iv=0; iv<nrvals && iv<chunk.ranges.size(); iv++ )
		valranges_[iv].include( chunk.ranges[iv], false );

	    for ( int irej=0; irej<chunk.rejected.size() &&
			      rejectedlines_.size()<1024; irej++ )
		rejectedlines_.add( chunk.rejected.get(irej) );

	    for ( int irow=0; irow<chunk.inls.size(); irow++ )
	    {
		CommitKey key;
		key.inl = chunk.inls[irow];
		key.crl = chunk.crls[irow];
		key.seq = chunk.seq[irow];
		key.chunk = ich;
		key.idx = irow;
		keys += key;
	    }
	}

	if ( !keys.isEmpty() )
	    std::sort( keys.arr(), keys.arr()+keys.size(),
	    []( const CommitKey& a, const CommitKey& b )
	    {
		if ( a.inl != b.inl ) return a.inl < b.inl;
		if ( a.crl != b.crl ) return a.crl < b.crl;
		return a.seq < b.seq;
	    } );

	for ( int ikey=0; ikey<keys.size(); ikey++ )
	{
	    const CommitKey& key = keys[ikey];
	    const ParsedChunk& chunk = *parser.chunks_[key.chunk];
	    const float* vals = chunk.vals.arr() + (od_int64)key.idx * nrvals;
	    bvs->add( BinID(key.inl, key.crl), vals );
	}
    }

    sections_ += bvs;
    delete [] text;
    return true;
}


int HorizonScanner::nrPositions() const
{ return dtctor_.nrPositions(); }

StepInterval<int> HorizonScanner::inlRg() const
{ return dtctor_.getRange(true); }

StepInterval<int> HorizonScanner::crlRg() const
{ return dtctor_.getRange(false); }

bool HorizonScanner::gapsFound( bool inl ) const
{ return dtctor_.haveGaps(inl); }
