/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Feb 2004
-*/

static const char* rcsID = "$Id: seisscanner.cc,v 1.1 2004-02-26 22:41:01 bert Exp $";

#include "seisscanner.h"
#include "seisinfo.h"
#include "seisread.h"
#include "seistrc.h"
#include "binidselimpl.h"
#include "binid2coord.h"
#include "sorting.h"
#include "ioobj.h"
#include "iopar.h"
#include "conn.h"
#include "stats.h"
#include <values.h>


SeisScanner::SeisScanner( const IOObj& ioobj )
    	: Executor( "Scan seismic volume file(s)" )
    	, reader(*new SeisTrcReader(&ioobj))
	, trc(*new SeisTrc)
    	, chnksz(10)
{
    init();
    Stat_initRandom(0);
}


SeisScanner::~SeisScanner()
{
    delete &reader;
    delete &trc;
}


void SeisScanner::init()
{
    curmsg = "Scanning";
    first_trace = true;
    nrvalidtraces = nrnulltraces = nrlines = 0;
    nrtrcsperposn = tracesthisposition = 1;
    nrdistribvals = 0;
    invalidsamplenr = -1;
    nonnullsamplerg.start = MAXINT;
    nonnullsamplerg.stop = 0;
    inlgapsfound = crlgapsfound = varcrlstart = varcrlend = false;
    nonstdnrtrcsbid.inl = -999;
    invalidsamplebid.inl = -999;
}


const char* SeisScanner::message() const
{
    return *reader.errMsg() ? reader.errMsg() : curmsg.buf();
}


const char* SeisScanner::nrDoneText() const
{
    return "Traces handled";
}


int SeisScanner::nrDone() const
{
    return reader.tracesHandled();
}


bool SeisScanner::getSurvInfo( BinIDSampler& bs, StepInterval<double>& zrg,
			       Coord crd[3] ) const
{
    bs.start.inl = inlrg.start; bs.stop.inl = inlrg.stop;
    bs.step.inl = inlrg.step;
    bs.start.crl = crlrg.start; bs.stop.crl = crlrg.stop;
    bs.step.crl = crlrg.step;
    zrg.start = sampling.atIndex( nonnullsamplerg.start );
    zrg.stop = sampling.atIndex( nonnullsamplerg.stop );
    zrg.step = sampling.step;

    // Setup the inl/crl vs coord things via longest inline
    const bool usemin = longestinlstart.inl - mininlbinid.inl
		      > maxinlbinid.inl - longestinlstart.inl;
    Coord c[3];
    BinID b[2];
    c[0] = usemin ? mininlbinidcoord : maxinlbinidcoord;
    b[0] = usemin ? mininlbinid : maxinlbinid;
    c[1] = longestinlstartcoord;
    b[1] = longestinlstart;
    c[2] = longestinlstopcoord;
    int crl = longestinlstop.crl;
    BinID2Coord b2c;
    const char* msg = b2c.set3Pts( c, b, crl );
    if ( msg )
    {
	curmsg = msg;
	return false;
    }

    // Now calculate what the coords would have been on the corners
    crd[0] = b2c.transform( bs.start );
    crd[1] = b2c.transform( bs.stop );
    crd[2] = b2c.transform( BinID(bs.start.inl,bs.stop.crl) );

    return true;
}


void SeisScanner::report( IOPar& iopar ) const
{
    iopar.clear();
    if ( !reader.ioObj() ) { iopar.setName( "No scan executed" ); return; }

    BufferString bs = "Report for "; bs += reader.ioObj()->translator();
    bs += " cube '"; bs += reader.ioObj()->name(); bs += "'\n\n";
    iopar.setName( bs );

    iopar.add( "->", "Sampling info" );
    iopar.set( "Z step", sampling.step );
    iopar.set( "Z start in file", sampling.start );
    iopar.set( "Z stop in file", zRange().stop );
    iopar.set( "Number of samples in file", (int)nrsamples );
    if ( nonnullsamplerg.start != 0 )
	iopar.set( "First non-zero sample", nonnullsamplerg.start );
    if ( nonnullsamplerg.stop != nrsamples-1 )
	iopar.set( "Last non-zero sample", nonnullsamplerg.stop );

    iopar.add( "->", "Global stats" );
    iopar.set( "Number of non-null traces", (int)nrvalidtraces );
    iopar.set( "Number of null traces", (int)nrnulltraces );
    if ( nrtrcsperposn != 1 )
    {
	iopar.set( "Number of traces per position", (int)nrvalidtraces );
	if ( nonstdnrtrcsbid.inl > 0 )
	{
	    iopar.set( "First change in number of traces per position found at",
		       nonstdnrtrcsbid );
	}
    }
    iopar.set( "Number of inlines", (int)nrlines );
    iopar.set( "First inline", inlrg.start );
    iopar.set( "Last inline", inlrg.stop );
    iopar.set( "Step inline", inlrg.step );
    iopar.set( "Gaps in inline numbers", inlgapsfound );
    iopar.set( "First crossline", crlrg.start );
    iopar.set( "Last crossline", crlrg.stop );
    iopar.set( "Step crossline", crlrg.step );
    iopar.set( "Gaps in crossline numbers", inlgapsfound );
    iopar.set( "Lines start at variable crossline numbers", varcrlstart );
    iopar.set( "Lines end at variable crossline numbers", varcrlend );


    iopar.add( "->", "Data values" );
    iopar.set( "Minimum value", valrg.start );
    iopar.set( "Maximum value", valrg.stop );
    if ( invalidsamplebid.inl > 0 )
    {
	iopar.set( "First invalid value found at", invalidsamplebid );
	iopar.set( "First invalid value was sample number", invalidsamplenr );
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


const char* SeisScanner::getClipRgStr( float pct ) const
{
    const float ratio = nrdistribvals / (1 - 100.*pct);
    int idx0 = mNINT(ratio);
    int idx1 = nrdistribvals - idx0 - 1;

    float v1 = distribvals[idx0];
    float v2 = idx0 >= idx1 ? v1 : distribvals[idx1];

    static BufferString ret;
    ret = v1; ret += " - "; ret += v2;
    return ret.buf();
}


int SeisScanner::nextStep()
{
    for ( int itrc=0; itrc<chnksz; itrc++ )
    {
	int res = reader.get( trc.info() );
	if ( res < 1 )
	{
	    curmsg = "Done";
	    if ( res == 0 )
	    {
		curmsg = "Error during read of trace header after ";
		if ( !prevbid.inl )
		    "opening file";
		else
		{
		    curmsg += prevbid.inl; curmsg += "/"; curmsg += prevbid.crl;
		}
	    }
	    wrapUp();
	    return res;
	}
	if ( res > 1 ) continue;

	if ( !reader.get( trc ) )
	{
	    curmsg = "Error during read of trace data at ";
	    curmsg += trc.info().binid.inl; curmsg += "/";
	    curmsg += trc.info().binid.crl;
	    wrapUp();
	    return Executor::ErrorOccurred;
	}

	if ( doValueWork() )
	{
	    if ( first_trace )
		handleFirstTrc();
	    else
		handleTrc();
	    nrvalidtraces++;
	}
    }

    return Executor::MoreToDo;
}


void SeisScanner::wrapUp()
{
    reader.close();
    sort_array( distribvals, nrdistribvals );
}


void SeisScanner::handleFirstTrc()
{
    first_trace = false;
    sampling = trc.info().sampling;
    nrsamples = trc.size(0);
    longestinlstart = longestinlstop = mininlbinid = maxinlbinid
	= curlinestart = curlinestop
	= trc.info().binid;
    longestinlstartcoord = longestinlstopcoord = mininlbinidcoord
	= maxinlbinidcoord = curlinestartcoord = curlinestopcoord
	= trc.info().coord;
    inlrg.start = inlrg.stop = mininlbinid.inl;
    crlrg.start = crlrg.stop = mininlbinid.crl;
    xrg.start = xrg.stop = mininlbinidcoord.x;
    yrg.start = yrg.stop = mininlbinidcoord.y;
    prevbid = mininlbinid;
    nonnullsamplerg.start = trc.size(0);
}


void SeisScanner::handleTrc()
{
    const BinID& curbid = trc.info().binid;
    if ( curbid == prevbid )
    {
	if ( first_position )
	    nrtrcsperposn++;
	else
	{
	    tracesthisposition++;
	    if ( tracesthisposition > nrtrcsperposn )
	    {
		if ( nonstdnrtrcsbid.inl != -999 )
		    nonstdnrtrcsbid = curbid;
	    }
	}
    }
    else
    {
	first_position = false;
	if ( tracesthisposition != nrtrcsperposn )
	    nonstdnrtrcsbid = prevbid;
	else
	    handleBinIDChange();
	tracesthisposition = 1;
    }

    prevbid = curbid;
}


void SeisScanner::handleBinIDChange()
{
    const BinID& curbid = trc.info().binid;
    const Coord& curcoord = trc.info().coord;

    inlrg.include( curbid.inl ); crlrg.include( curbid.crl );
    xrg.include( curcoord.x ); yrg.include( curcoord.y );

    if ( curbid.inl != prevbid.inl )
    {
	nrlines++;
	if ( nrlines == 2 )
	    inlrg.step = prevbid.inl - curbid.inl;
	else if ( prevbid.inl - curbid.inl != inlrg.step )
	{
	    if ( prevbid.inl - curbid.inl < inlrg.step )
		inlrg.step = prevbid.inl - curbid.inl;
	    inlgapsfound = true;
	}

	if ( mininlbinid.inl > curbid.inl )
	    { mininlbinid = curbid; mininlbinidcoord = curcoord; }
	if ( maxinlbinid.inl < curbid.inl )
	    { maxinlbinid = curbid; maxinlbinidcoord = curcoord; }

	if ( curlinestop.crl - curlinestart.crl
		> longestinlstop.crl - longestinlstart.crl )
	{
	    longestinlstart = curlinestart; longestinlstop = curlinestop;
	    longestinlstartcoord = curlinestartcoord;
	    longestinlstopcoord = curlinestopcoord;
	}

	if ( nrlines > 1 )
	{
	    if (   curbid.crl != longestinlstart.crl
		&& curbid.crl != longestinlstop.crl )
		varcrlstart = true;
	    if (   prevbid.crl != longestinlstart.crl
		&& prevbid.crl != longestinlstop.crl )
		varcrlend = true;
	}

	curlinestart = curlinestop = curbid;
	curlinestartcoord = curlinestopcoord = curcoord;
    }
    else
    {
	if ( curlinestart.crl > curbid.crl )
	    { curlinestart.crl = curbid.crl; curlinestartcoord = curcoord; }
	if ( curlinestop.crl < curbid.crl )
	    { curlinestop.crl = curbid.crl; curlinestopcoord = curcoord; }

	if ( !crlrg.step )
	    crlrg.step = prevbid.crl - curbid.crl;
	else if ( prevbid.crl - curbid.crl != crlrg.step )
	{
	    if ( prevbid.crl - curbid.crl < crlrg.step )
		crlrg.step = prevbid.crl - curbid.crl;
	    crlgapsfound = true;
	}
    }
}


bool SeisScanner::doValueWork()
{
    const bool adddistribvals = nrdistribvals < mMaxNrDistribVals;
    float thresh = 1. / (1. + 0.01 * nrvalidtraces);
    const bool selected_trc = !adddistribvals
			   && Stat_getRandom() < 0.01;
    unsigned int selsieve = (unsigned int)(1. + 0.01 * nrvalidtraces);
    if ( selsieve > 10 ) selsieve = 10;
    float sievethresh = 1. / selsieve;

    bool nonnull_seen = false;
    int nullstart = trc.size(0);
    for ( int idx=nullstart-1; idx!=-1; idx-- )
    {
	float val = trc.get(idx,0);
	if ( !mIS_ZERO(val) ) break;
	nullstart = idx;
    }
   if ( nullstart-1 > nonnullsamplerg.stop )
       nonnullsamplerg.stop = nullstart - 1;

    for ( int idx=0; idx<nullstart; idx++ )
    {
	float val = trc.get(idx,0);
	bool iszero = mIS_ZERO(val);
	if ( !nonnull_seen )
	{
	   if ( iszero ) continue;
	   else if ( nonnullsamplerg.start > idx )
	       nonnullsamplerg.start = idx;
	   nonnull_seen = true;
	}

	if ( !isFinite(val) || val > 1e30 || val < -1e30 )
	{
	    if ( invalidsamplenr < 0 )
	    {
		invalidsamplenr = idx;
		invalidsamplebid = trc.info().binid;
	    }
	    continue;
	}

	valrg.include( val );

	if ( nrdistribvals < mMaxNrDistribVals )
	    distribvals[nrdistribvals++] = val;
	else if ( selected_trc && Stat_getRandom() < sievethresh )
	{
	    int arrnr = Stat_getIndex(nrdistribvals);
	    distribvals[ arrnr ] = val;
	}
    }

    if ( !nonnull_seen )
    {
	nrnulltraces++;
	return false;
    }
    return true;
}
