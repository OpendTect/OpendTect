/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 25-9-1999
-*/

static const char* rcsID = "$Id$";
#include "prog.h"
#include "batchprog.h"
#include "ioman.h"
#include "ioobj.h"
#include "seiscubeprov.h"
#include "seisbuf.h"
#include "seiswrite.h"
#include "seisread.h"
#include "seistrc.h"
#include "executor.h"
#include "ptrman.h"
#include "separstr.h"
#include "sorting.h"
#include "survinfo.h"
#include "errh.h"
#include "iopar.h"
#include "debug.h"
#include <math.h>


static int xlinestep = 1;
static bool doinls = true;
static int maxnrtrcsfill = 5;
static int nrinlbufs = 1;
static ObjectSet<SeisTrcBuf> fulltbufs;
static std::ostream* streamptr = 0;
#define strm (*streamptr)

#define mBid(tptr) tptr->info().binid
#define mInl(tptr) tptr->info().binid.inl
#define mCrl(tptr) tptr->info().binid.crl

static std::ostream* dbgstrmptr = 0;
#define dbgstrm if ( dbgstrmptr ) *dbgstrmptr << "\tD: "


static bool fillBuf( SeisMSCProvider& prov, SeisTrcBuf& buf, bool& atend )
{
    if ( atend )
    {
	dbgstrm << "atend is true" << std::endl;
	return false;
    }

    buf.deepErase();
    const SeisTrc* trc = prov.get(0,0);
    if ( !trc )
    {
	dbgstrm << "!trc on prov.get(0,0) (first time)" << std::endl;
	return false;
    }
    const int inl = mInl(trc);

    while ( !trc || mInl(trc) == inl )
    {
	buf.add( new SeisTrc(*trc) );
	while ( 1 )
	{
	    SeisMSCProvider::AdvanceState res = prov.advance();
	    if ( res == SeisMSCProvider::Buffering ) continue;
	    if ( res == SeisMSCProvider::EndReached )
	    {
		atend = true;
		dbgstrm << "atend = true (prov)" << std::endl;
		return true;
	    }
	    if ( res == SeisMSCProvider::Error )
	    {
		dbgstrm << "prov error" << std::endl;
		strm << prov.errMsg() << std::endl;
		return false;
	    }
	    break;
	}
	trc = prov.get(0,0);
	if ( !trc )
	{
	    atend = true;
	    dbgstrm << "prov(0,0) is null" << std::endl;
	    return true;
	}
    }

    return true;
}


static SeisTrc* getInterp( const SeisTrc* t1, const SeisTrc* t2, float frac )
{
    SeisTrc* trc = new SeisTrc( *t1 );
    for ( int icomp=0; icomp<trc->data().nrComponents(); icomp++ )
    {
	const int sz = trc->size();
	for ( int isamp=0; isamp<sz; isamp++ )
	{
	    float val1 = t1->get( isamp, icomp );
	    float val2 = t2->get( isamp, icomp );
	    trc->set( isamp, frac*val1 + (1-frac)*val2, icomp );
	}
    }
    return trc;
}


static void insertTrace( SeisTrcBuf& tb, SeisTrc* trc )
{
    const int xl = mCrl(trc);
    for ( int idx=0; idx<tb.size(); idx++ )
    {
	SeisTrc* t = tb.get( idx );
	if ( mCrl(t) == xl )
	{
	    strm << "Bad insert " << mInl(trc) << '/' << xl << std::endl;
	    delete trc; return;
	}
	else if ( mCrl(t) > xl )
	    { tb.insert( trc, idx ); return; }
    }
}


static void interpolateXdir()
{
    SeisTrcBuf& tbuf = *fulltbufs[fulltbufs.size()-1];
    if ( tbuf.size() < 3 ) return;

    const SeisTrc* prevtrc = tbuf.get( 0 );
    bool haveinterpolated = false;
    for ( int idx=1; idx<tbuf.size(); idx++ )
    {
	const SeisTrc* curtrc = tbuf.get( idx );

	int gap = mCrl(curtrc) - mCrl(prevtrc);
	if ( gap > xlinestep && gap <= xlinestep * maxnrtrcsfill )
	{
	    haveinterpolated = true;
	    Interval<int> trcnrs( mCrl(prevtrc), mCrl(curtrc) );
	    for ( int xl=trcnrs.start+xlinestep; xl<trcnrs.stop;
		    	xl += xlinestep )
	    {
		float frac = trcnrs.stop - xl;
		frac /= gap;
		SeisTrc* newtrc = getInterp(prevtrc,curtrc,frac);
		mCrl(newtrc) = xl;
		newtrc->info().coord = SI().transform( newtrc->info().binid );
		insertTrace( tbuf, newtrc );
	    }
	}

	prevtrc = curtrc;
    }
    if ( haveinterpolated )
	{ strm << "Interpolated between Xlines."; strm.flush(); }
}


static bool incrIdxs( int* idxs, int& workcrl )
{
    const int nrtbufs = fulltbufs.size();
    for ( int idx=0; idx<nrtbufs; idx++ )
    {
	if ( mCrl(fulltbufs[idx]->get(idxs[idx])) == workcrl )
	{
	    if ( fulltbufs[idx]->size() > idxs[idx]+1 )
		idxs[idx]++;
	}
    }

    workcrl += xlinestep;
    while ( 1 )
    {
	bool higherfound = false;
	for ( int idx=0; idx<nrtbufs; idx++ )
	{
	    int bufcrl = mCrl(fulltbufs[idx]->get(idxs[idx]));
	    if ( bufcrl == workcrl )
		return true;
	    else if ( bufcrl > workcrl )
		higherfound = true;
	}
	if ( !higherfound )
	    return false;
	workcrl += xlinestep;
    }

    pFreeFnErrMsg("Can't reach this place","incrIdxs");
    return false;
}


static void interpolateIdir()
{
    const int nrtbufs = fulltbufs.size();
    if ( fulltbufs[nrtbufs-1]->size() == 0 ) return;

    int idxs[nrtbufs];
    for ( int idx=0; idx<nrtbufs; idx++ )
	idxs[idx] = 0;

    int workcrl = mCrl(fulltbufs[0]->get(0));
    for ( int idx=1; idx<nrtbufs; idx++ )
	if ( mCrl(fulltbufs[idx]->get(0)) < workcrl )
	    workcrl = mCrl(fulltbufs[idx]->get(0));

    const int lastbufidx = nrtbufs-1;
    bool haveinterpolated = false;
    do
    {
	const SeisTrc* endtrc = 0;
	if ( mCrl(fulltbufs[lastbufidx]->get(idxs[lastbufidx])) == workcrl )
	{
	    int startidx = -1;
	    for ( int idx=0; idx<lastbufidx; idx++ )
	    {
		if ( mCrl(fulltbufs[idx]->get(idxs[idx])) == workcrl )
		    startidx = idx;
	    }
	    if ( startidx != -1 && startidx < lastbufidx - 1 )
	    {
		haveinterpolated = true;
		const SeisTrc* t0 =fulltbufs[startidx]->get(idxs[startidx]);
		const SeisTrc* t1 =fulltbufs[lastbufidx]->get(idxs[lastbufidx]);
		for ( int ibuf=startidx+1; ibuf<lastbufidx; ibuf++ )
		{
		    SeisTrcBuf& tb = *fulltbufs[ibuf];
		    float frac = lastbufidx - ibuf;
		    frac /= lastbufidx - startidx;
		    SeisTrc* newtrc = getInterp(t0,t1,frac);
		    mInl(newtrc) = mInl(tb.get(0));
		    newtrc->info().coord = SI().transform(newtrc->info().binid);
		    insertTrace( tb, newtrc );
		}
	    }
	}

    }
    while ( incrIdxs(idxs,workcrl) );

    if ( haveinterpolated )
	{ strm << "Interpolated between Inlines."; strm.flush(); }
}


static bool writeBuf( SeisTrcWriter& trcwr, const SeisTrcBuf& tbuf )
{
    const int bufsz = tbuf.size();
    for ( int itrc=0; itrc<bufsz; itrc++ )
    {
	if ( !trcwr.put(*tbuf.get(itrc)) )
	    return false;
	if ( itrc && mCrl(tbuf.get(itrc))-mCrl(tbuf.get(itrc-1)) != xlinestep )
	{
	    strm << "Remaining Gap: " << mCrl(tbuf.get(itrc-1))
		 << " - " << mCrl(tbuf.get(itrc));
	    strm.flush();
	}
    }

    return true;
}


static bool writeFirstBuf( SeisTrcWriter& trcwr )
{
    if ( fulltbufs.size() < 1 )
    {
	dbgstrm << "Not yet 1 buf in fulltbufs" << std::endl;
	return false;
    }
    if ( maxnrtrcsfill > 0 )
	interpolateXdir();

    static bool collecting = true;
    if ( fulltbufs.size() < nrinlbufs && collecting ) return true;
    collecting = false;

    if ( doinls && maxnrtrcsfill > 0 )
	interpolateIdir();

    SeisTrcBuf& tbuf = *fulltbufs[0];
    if ( tbuf.size())
	{ strm << "Writing inl " << mInl(tbuf.get(0)) << " ..."; strm.flush(); }
    else
	dbgstrm << "Empty first buf." << std::endl;
    bool rv = writeBuf( trcwr, tbuf );
    return rv;
}


static void addBufs( ObjectSet<SeisTrcBuf>& bufs )
{
    int nrtbufs = bufs.size();
    int idxs[nrtbufs]; int starttrcs[nrtbufs];
    ObjectSet<SeisTrcBuf> usebufs;
    for ( int idx=0; idx<bufs.size(); idx++ )
    {
	SeisTrcBuf& tbuf = *bufs[idx];
	if ( tbuf.size() < 1 ) continue;
	usebufs += &tbuf;
	if ( tbuf.size() > 1 && mCrl(tbuf.get(0)) > mCrl(tbuf.get(1)) )
	    tbuf.revert();
	idxs[idx] = idx;
	starttrcs[idx] = mCrl(tbuf.get(0));
    }
    if ( usebufs.size() > 1 )
	sort_coupled( starttrcs, idxs, nrtbufs );

    if ( fulltbufs.size() >= nrinlbufs )
    {
	fulltbufs[0]->deepErase();
	delete fulltbufs[0];
	fulltbufs.remove( 0 );
    }

    SeisTrcBuf& tbuftofill = *new SeisTrcBuf( false );
    fulltbufs += &tbuftofill;
    for ( int idx=0; idx<usebufs.size(); idx++ )
    {
	SeisTrcBuf& subbuf = *usebufs[ idxs[idx] ];
	int lastprevcrl = tbuftofill.size()
	    		? mCrl(tbuftofill.get(tbuftofill.size()-1)) : -1;
	const int bufsz = subbuf.size();
	for ( int itrc=0; itrc<bufsz; itrc++ )
	{
	    SeisTrc* trc = subbuf.remove(0);
	    if ( mCrl(trc)<= lastprevcrl )
		delete trc;
	    else
		tbuftofill.add( trc );
	}
    }
	dbgstrm << "Now have " << fulltbufs.size() << " bufs. Last size is: "
		<< fulltbufs[ fulltbufs.size()-1 ]->size() << std::endl;
}


bool BatchProgram::go( std::ostream& strm_ )
{
    streamptr = &strm_;
    if ( DBG::isOn() )
	dbgstrmptr = &std::cerr;

    xlinestep = SI().crlStep();
    pars().get( "Xline step", xlinestep );
    pars().get( "Max Nr Traces", maxnrtrcsfill );
    pars().getYN( "Interpolate inlines", doinls );
    if ( doinls )
	nrinlbufs = maxnrtrcsfill;

    ObjectSet<SeisTrcBuf> bufs;
    ObjectSet<SeisMSCProvider> provs;
    for ( int idx=0; ; idx++ )
    {
	BufferString iopkey( "Input." );
	iopkey += idx;
	PtrMan<IOPar> iop = pars().subselect( iopkey );
	const char* res = iop ? iop->find( "ID" ) : 0;
	if ( !res || !*res )
	    { if ( idx ) break; continue; }
	SeisMSCProvider* prov = new SeisMSCProvider( MultiID(res) );
	prov->reader().usePar( *iop );
	SeisMSCProvider::AdvanceState advst = prov->advance();
	while ( advst == SeisMSCProvider::Buffering )
	    advst = prov->advance();
	if ( advst == SeisMSCProvider::EndReached )
	{
	    strm << "No valid data in " << IOM().nameOf(res) << std::endl;
	    return false;
	}
	if ( advst == SeisMSCProvider::Error )
	{
	    strm << prov->errMsg() << std::endl;
	    return false;
	}

	provs += prov;
	bufs += new SeisTrcBuf( false );
    }
    int nrprovs = provs.size();
    if ( nrprovs == 0 )
    {
	strm << "No valid input seismic data found" << std::endl;
	return false;
    }
	dbgstrm << "Nr providers: " << nrprovs << std::endl;

    const char* res = pars()["Output.ID"];
    IOObj* ioobj = IOM().get( res );
    if ( !ioobj )
    {
	strm << "Cannot find seismic data with ID: " << res << std::endl;
	return false;
    }
	dbgstrm << "Output to: " << ioobj->name() << std::endl;

    SeisTrcWriter trcwr( ioobj );
    if ( trcwr.errMsg() && *trcwr.errMsg() )
    {
	strm << trcwr.errMsg() << std::endl;
	return false;
    }
	dbgstrm << "TrcWriter initialised" << std::endl;

    bool atend[nrprovs];
    for ( int idx=0; idx<nrprovs; idx++ )
	atend[idx] = false;

    while ( nrprovs )
    {
	const SeisTrc* trc = provs[0]->get(0,0);
	if ( trc )
	    strm << "Reading inl " << mInl(trc) << " ...";
	strm.flush();

	for ( int provnr=0; provnr<nrprovs; provnr++ )
	{
	    SeisTrcBuf* buf = bufs[provnr];
	    SeisMSCProvider* prov = provs[provnr];
	    if ( !fillBuf(*prov,*buf,atend[provnr]) )
	    {
		dbgstrm << "fillBuf returned false" << std::endl;
		delete prov; buf->deepErase(); delete buf;
		provs -= prov; bufs -= buf;
		nrprovs--;
	    }
	}
	if ( !nrprovs ) break;

	addBufs(bufs);
	if ( !writeFirstBuf(trcwr) )
	{
	    dbgstrm << "writeFirstBuf returned false" << std::endl;
	    strm << trcwr.errMsg() << std::endl;
	    return false;
	}
	strm << std::endl;
    }

    delete fulltbufs[0]; fulltbufs.remove( 0 );
    while ( fulltbufs.size() )
    {
	writeFirstBuf( trcwr );
	delete fulltbufs[0]; fulltbufs.remove( 0 );
    }

    return true;
}
