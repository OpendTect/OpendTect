/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seisscanner.h"
#include "seisinfo.h"
#include "seisread.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "trckeyzsampling.h"
#include "posinfodetector.h"
#include "sorting.h"
#include "oddirs.h"
#include "ioobj.h"
#include "iopar.h"
#include "statrand.h"
#include "perthreadrepos.h"
#include "conn.h"
#include "math2.h"
#include "file.h"
#include "uistrings.h"

SeisScanner::SeisScanner( const IOObj& ioobj, Seis::GeomType gt, int mtr )
    : Executor( "Scan seismic data" )
    , rdr_(*new SeisTrcReader(ioobj,&gt))
    , trc_(*new SeisTrc)
    , dtctor_(*new PosInfo::Detector(
		PosInfo::Detector::Setup(Seis::is2D(gt))
			.isps(Seis::isPS(gt)).reqsorting(true) ) )
    , curmsg_(uiStrings::sScanning())
    , totalnr_(mtr < 0 ? -2 : mtr)
    , maxnrtrcs_(mtr)
    , nrnulltraces_(0)
    , invalidsamplenr_(-1)
{
    dtctor_.reInit();
    valrg_.start = mUdf(float);
    nonnullsamplerg_.stop = 0;
    nonnullsamplerg_.start = invalidsamplebid_.inl() = mUdf(int);
}


SeisScanner::~SeisScanner()
{
    delete &dtctor_;
    delete &trc_;
    delete &rdr_;
}


uiString SeisScanner::uiMessage() const
{
    return !rdr_.errMsg().isEmpty() ? rdr_.errMsg() : curmsg_;
}


uiString SeisScanner::uiNrDoneText() const
{
    return tr("Traces handled");
}


od_int64 SeisScanner::nrDone() const
{
    return dtctor_.nrPositions( false );
}


od_int64 SeisScanner::totalNr() const
{
    if ( totalnr_ == -2 )
    {
	SeisScanner& self = *(const_cast<SeisScanner*>(this));
	self.totalnr_ = -1;
	if ( maxnrtrcs_ )
	    self.totalnr_ = maxnrtrcs_;
	else if ( rdr_.ioObj() )
	{
	    TrcKeyZSampling cs;
	    if ( SeisTrcTranslator::getRanges(*rdr_.ioObj(),cs) )
		self.totalnr_ = cs.hsamp_.nrInl() * cs.hsamp_.nrCrl();
	}
    }
    return totalnr_;
}


bool SeisScanner::getSurvInfo( TrcKeyZSampling& cs, Coord crd[3] ) const
{
    const uiString msg = dtctor_.getSurvInfoWithMsg( cs.hsamp_, crd );
    if ( !msg.isEmpty() )
	{ curmsg_ = msg; return false; }

    cs.zsamp_.start = sampling_.atIndex( nonnullsamplerg_.start );
    cs.zsamp_.stop = sampling_.atIndex( nonnullsamplerg_.stop );
    cs.zsamp_.step = sampling_.step;
    return true;
}


void SeisScanner::report( IOPar& iopar ) const
{
    if ( !rdr_.ioObj() )
    {
	iopar.setEmpty();
	iopar.setName( "No scan executed" );
	return;
    }

    BufferString str = "Report for "; str += rdr_.ioObj()->translator();
    str += dtctor_.is2D() ? " line set '" : " cube '";
    str += rdr_.ioObj()->name(); str += "'\n\n";
    iopar.setName( str );

    iopar.add( IOPar::sKeyHdr(), "Sampling info" );
    iopar.set( "Z step", sampling_.step );
    iopar.set( "Z start in file", sampling_.start );
    iopar.set( "Z stop in file", zRange().stop );

    iopar.set( "Number of samples in file", (int)nrsamples_ );
    if ( nonnullsamplerg_.start != 0 )
	iopar.set( "First non-zero sample", nonnullsamplerg_.start + 1 );
    if ( nonnullsamplerg_.stop != nrsamples_-1 )
	iopar.set( "Last non-zero sample", nonnullsamplerg_.stop + 1 );

    iopar.add( IOPar::sKeyHdr(), "Global stats" );
    iopar.set( "Number of null traces", (int)nrnulltraces_ );
    dtctor_.report( iopar );
    if ( !dtctor_.is2D() )
    {
	TrcKeyZSampling cs; Coord crd[3];
	getSurvInfo(cs,crd);
	iopar.set( "Z.start", cs.zsamp_.start );
	iopar.set( "Z.stop", cs.zsamp_.stop );
	iopar.set( "Z.step", cs.zsamp_.step );
    }

    if ( !mIsUdf(valrg_.start) )
    {
	iopar.add( IOPar::sKeyHdr(), "Data values" );
	iopar.set( "Minimum value", valrg_.start );
	iopar.set( "Maximum value", valrg_.stop );
	const float* vals = clipsampler_.vals();
	const int nrvals = mCast( int, clipsampler_.nrVals() );
	iopar.set( "Median value", vals[nrvals/2] );
	iopar.set( "1/4 value", vals[nrvals/4] );
	iopar.set( "3/4 value", vals[3*nrvals/4] );
	if ( !mIsUdf(invalidsamplebid_.inl()) )
	{
	    iopar.set( "First invalid value at", invalidsamplebid_ );
	    iopar.set( "First invalid value sample number", invalidsamplenr_ );
	}
	iopar.add( "0.1% clip range", getClipRgStr(0.1) );
	iopar.add( "0.2% clip range", getClipRgStr(0.2) );
	iopar.add( "0.3% clip range", getClipRgStr(0.3) );
	iopar.add( "0.5% clip range", getClipRgStr(0.5) );
	iopar.add( "1% clip range", getClipRgStr(1) );
	iopar.add( "1.5% clip range", getClipRgStr(1.5) );
	iopar.add( "2% clip range", getClipRgStr(2) );
	iopar.add( "3% clip range", getClipRgStr(3) );
	iopar.add( "5% clip range", getClipRgStr(5) );
	iopar.add( "10% clip range", getClipRgStr(10) );
	iopar.add( "20% clip range", getClipRgStr(20) );
	iopar.add( "30% clip range", getClipRgStr(30) );
	iopar.add( "50% clip range", getClipRgStr(50) );
	iopar.add( "90% clip range", getClipRgStr(90) );
    }
}


const char* SeisScanner::getClipRgStr( float pct ) const
{
    const float* vals = clipsampler_.vals();
    const int nrvals = mCast( int, clipsampler_.nrVals() );
    const float ratio = nrvals * .005f * pct;
    int idx0 = mNINT32(ratio);
    int idx1 = nrvals - idx0 - 1;
    if ( idx0 > idx1 ) Swap( idx0, idx1 );

    mDeclStaticString( ret );
    ret = vals[idx0]; ret += " - "; ret += vals[idx1];

    float maxabs = fabs( vals[idx0] );
    if ( fabs( vals[idx1] ) > maxabs ) maxabs = fabs( vals[idx1] );
    if ( maxabs != 0 )
    {
	const float sc8 = 127 / maxabs;
	const float sc16 = 32767 / maxabs;
	ret += " [scl 16/8-bits: "; ret += sc16;
	ret += " ; "; ret += sc8; ret += "]";
    }

    return ret.buf();
}


void SeisScanner::launchBrowser( const IOPar& startpar, const char* fnm ) const
{
    if ( !fnm || !*fnm )
	fnm = GetProcFileName( "seisscan_tmp.txt" );

    IOPar iopar( startpar ); report( iopar );
    iopar.write( fnm, IOPar::sKeyDumpPretty() );

    File::launchViewer( fnm );
}


int SeisScanner::nextStep()
{
    if ( !rdr_.errMsg().isEmpty() )
	return Executor::ErrorOccurred();

    int res = rdr_.get( trc_.info() );
    if ( res < 1 )
    {
	dtctor_.finish();
	curmsg_ = tr("Done");
	if ( res != 0 )
	{
	    uiString posmsg;
	    if ( dtctor_.nrPositions(false) == 0 )
		posmsg = tr("opening file");
	    else
	    {
		const BinID& bid( dtctor_.lastPosition().binid_ );
		if ( dtctor_.is2D() )
		{
		    posmsg = tr("trace number %1")
			   .arg(toString(bid.crl()));
		}
		else
		    { posmsg = toUiString(bid.toString()); }
	    }

	    curmsg_ = tr("Error during read of trace header after %1")
		    .arg( posmsg );

	}
	wrapUp();
	return res;
    }
    if ( res > 1 )
	return Executor::MoreToDo();

    if ( !rdr_.get( trc_ ) )
    {
	dtctor_.finish();

	const BinID bid = trc_.info().binID();
	uiString posmsg;
	if ( dtctor_.is2D() )
	{ posmsg = tr("trace number %1").arg(toString(bid.crl())); }
	else
	    { posmsg = toUiString(bid.toString()); }
	curmsg_ = tr("Error during read of trace data at %1").arg(posmsg);
	wrapUp();
	return Executor::ErrorOccurred();
    }

    if ( doValueWork() && !addTrc() )
	{ dtctor_.finish(); wrapUp(); return Executor::ErrorOccurred(); }

    if ( maxnrtrcs_ > -1 && dtctor_.nrPositions(false) >= maxnrtrcs_ )
	{ dtctor_.finish(); wrapUp(); return Executor::Finished(); }

    return Executor::MoreToDo();
}


void SeisScanner::wrapUp()
{
    dtctor_.finish();
    rdr_.close();
    clipsampler_.finish();
}


bool SeisScanner::doValueWork()
{
    bool nonnull_seen = false;
    int nullstart = trc_.size();
    for ( int idx=nullstart-1; idx!=-1; idx-- )
    {
	float val = trc_.get(idx,0);
	if ( !mIsZero(val,mDefEps) ) break;
	nullstart = idx;
    }
   if ( nullstart-1 > nonnullsamplerg_.stop )
       nonnullsamplerg_.stop = nullstart - 1;

    bool needinitvalrg = mIsUdf(valrg_.start);
    for ( int idx=0; idx<nullstart; idx++ )
    {
	float val = trc_.get(idx,0);
	bool iszero = mIsZero(val,mDefEps);
	if ( !nonnull_seen )
	{
	   if ( iszero ) continue;
	   else if ( nonnullsamplerg_.start > idx )
	       nonnullsamplerg_.start = idx;
	   nonnull_seen = true;
	}

	if ( !Math::IsNormalNumber(val) || val > 1e30 || val < -1e30 )
	{
	    if ( invalidsamplenr_ < 0 )
	    {
		invalidsamplenr_ = idx;
		invalidsamplebid_ = trc_.info().binID();
	    }
	    continue;
	}

	if ( !needinitvalrg )
	    valrg_.include( val );
	else
	{
	    valrg_.start = valrg_.stop = val;
	    needinitvalrg = false;
	}

	clipsampler_.add( val );
    }

    if ( !nonnull_seen )
    {
	nrnulltraces_++;
	return false;
    }
    return true;
}


bool SeisScanner::addTrc()
{
    if ( nrsamples_ < 1 )
    {
	nrsamples_ = trc_.size();
	sampling_ = trc_.info().sampling;
    }

    if ( !dtctor_.add(trc_.info().coord, trc_.info().binID(),
		      trc_.info().trcNr(), trc_.info().offset) )
    {
	curmsg_ = dtctor_.errMsg();
	return false;
    }

    return true;
}
