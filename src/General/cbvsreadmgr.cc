/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : April 2001
 * FUNCTION : CBVS File pack reading
-*/

static const char* rcsID = "$Id: cbvsreadmgr.cc,v 1.15 2002-07-19 14:47:31 bert Exp $";

#include "cbvsreadmgr.h"
#include "cbvsreader.h"
#include "filegen.h"
#include "strmprov.h"
#include "survinfo.h"

static inline void mkErrMsg( BufferString& errmsg, const char* fname,
			     const char* msg )
{
    errmsg = "'"; errmsg += fname; errmsg += "' ";
    errmsg += msg;
}


CBVSReadMgr::CBVSReadMgr( const char* fnm, const CubeSampling* cs )
	: CBVSIOMgr(fnm)
	, info_(*new CBVSInfo)
	, vertical_(false)
{
    bool foundone = false;

    if ( !fnm || !strcmp(fnm,StreamProvider::sStdIO) )
    {
	addReader( &cin, cs );
	if ( !readers_.size() )
	    errmsg_ = "Standard input contains no relevant data";
	else
	    createInfo();
	return;
    }

    bool alreadyfailed = false;
    for ( int fnr=0; ; fnr++ )
    {
	BufferString fname = getFileName( fnr );
	if ( !File_exists((const char*)fname) )
	    break;

	foundone = true;
	if ( !addReader(fname,cs) )
	{
	    if ( *(const char*)errmsg_ )
		return;
	}
	else
	    fnames_ += new BufferString( fname );
    }

    if ( !readers_.size() )
    {
	if ( foundone )
	    mkErrMsg( errmsg_, basefname_, "contains no relevant data" );
	else
	    mkErrMsg( errmsg_, basefname_, "cannot be opened" );
	return;
    }

    createInfo();
}


CBVSReadMgr::~CBVSReadMgr()
{
    close();
    delete &info_;
    deepErase( readers_ );
}


void CBVSReadMgr::close()
{
    for ( int idx=0; idx<readers_.size(); idx++ )
	readers_[idx]->close();
}


const char* CBVSReadMgr::errMsg_() const
{
    return readers_.size() ? readers_[curnr_]->errMsg() : 0;
}


bool CBVSReadMgr::addReader( const char* fname, const CubeSampling* cs )
{
    StreamData sd = StreamProvider(fname).makeIStream();
    if ( !sd.usable() )
    {
	mkErrMsg( errmsg_, fname, "cannot be opened" );
	sd.close();
	return false;
    }

    return addReader( sd.istrm, cs );
}


bool CBVSReadMgr::addReader( istream* strm, const CubeSampling* cs )
{
    CBVSReader* newrdr = new CBVSReader( strm );
    if ( newrdr->errMsg() )
    {
	errmsg_ = newrdr->errMsg();
	delete newrdr;
	return false;
    }

    if ( cs && !newrdr->info().contributesTo(*cs) )
    {
	delete newrdr;
	return false;
    }

    readers_ += newrdr;
    return true;
}


void CBVSReadMgr::createInfo()
{
    const int sz = readers_.size();
    if ( sz == 0 ) return;
    info_ = readers_[0]->info();
    if ( !info_.geom.step.inl ) // unknown, get from other source
    {
	int rdrnr = 1;
	while ( rdrnr < sz )
	{
	    if ( readers_[rdrnr]->info().geom.step.inl )
	    {
		info_.geom.step.inl = readers_[rdrnr]->info().geom.step.inl;
	        break;
	    }
	}
	if ( !info_.geom.step.inl )
	    info_.geom.step.inl = SI().step(false).inl;
    }

    for ( int idx=1; idx<sz; idx++ )
	if ( !handleInfo(readers_[idx],idx) ) return;
}


#define mErrMsgMk(s) \
    errmsg_ = s; \
    errmsg_ += " found in:\n"; errmsg_ += *fnames_[ireader];

#define mErrRet(s) { \
    mErrMsgMk(s) \
    errmsg_ += "\ndiffers from first file"; \
    return false; \
}

bool CBVSReadMgr::handleInfo( CBVSReader* rdr, int ireader )
{
    if ( !ireader ) return true;

    const CBVSInfo& ci = rdr->info();
    if ( ci.nrtrcsperposn != info_.nrtrcsperposn )
	mErrRet("Number of traces per position")
    if ( !info_.geom.fullyrectandreg )
	const_cast<CBVSInfo&>(ci).geom.step.inl = info_.geom.step.inl;
    else if ( ci.geom.step.inl != info_.geom.step.inl )
	mErrRet("In-line number step")
    if ( ci.geom.step.crl != info_.geom.step.crl )
	mErrRet("Cross-line number step")

    for ( int icomp=0; icomp<ci.compinfo.size(); icomp++ )
    {
	const BasicComponentInfo& cicompinf = *ci.compinfo[icomp];
	BasicComponentInfo& compinf = *info_.compinfo[icomp];
	if ( !mIS_ZERO(cicompinf.sd.step-compinf.sd.step) )
	    mErrRet("Sample interval")
	if ( mIS_ZERO(cicompinf.sd.start-compinf.sd.start) )
	{
	    if ( cicompinf.nrsamples != compinf.nrsamples )
		mErrRet("Number of samples")
	}
	else
	{
	    StepInterval<float> intv = compinf.sd.interval(compinf.nrsamples);
	    intv.stop += compinf.sd.step;
	    if ( !mIS_ZERO(cicompinf.sd.start-intv.stop) )
	    {
		mErrMsgMk("Time range")
		errmsg_ += "\nis unexpected.\nExpected: ";
		errmsg_ += intv.stop; errmsg_ += " s.\nFound: ";
		errmsg_ += cicompinf.sd.start; errmsg_ += " s.";
		return false;
	    }
	    vertical_ = true;
	}
    }

    info_.geom.merge( ci.geom );
    return true;
}


int CBVSReadMgr::nextRdrNr( int rdrnr ) const
{
    rdrnr++;
    if ( rdrnr >= readers_.size() ) rdrnr = 0;
    if ( rdrnr == curnr_ ) rdrnr = -1;
    return rdrnr;
}


BinID CBVSReadMgr::nextBinID() const
{
    int rdrnr = curnr_;
    BinID ret = readers_[rdrnr]->nextBinID();
    while ( 1 )
    {
	if ( ret != BinID(0,0) )
	    return ret;
	else
	{
	    rdrnr++;
	    if ( rdrnr >= readers_.size() )
		return ret;
	}
	ret = readers_[rdrnr]->nextBinID();
    }

    return ret;
}


bool CBVSReadMgr::goTo( const BinID& bid )
{
    if ( vertical_ )
    {
	for ( int idx=0; idx<readers_.size(); idx++ )
	{
	    if ( !readers_[idx]->goTo(bid) )
		return false;
	}
	return true;
    }

    int rdrnr = curnr_;

    while ( !readers_[rdrnr]->goTo( bid ) )
    {
	rdrnr = nextRdrNr( rdrnr );
	if ( rdrnr < 0 ) return false;
    }

    curnr_ = rdrnr;
    return true;
}


bool CBVSReadMgr::toNext()
{
    if ( vertical_ )
    {
	for ( int idx=0; idx<readers_.size(); idx++ )
	{
	    if ( !readers_[idx]->toNext() )
		return false;
	}
	return true;
    }

    if ( !readers_[curnr_]->toNext() )
    {
	if ( curnr_ == readers_.size()-1 ) return false;
	curnr_++;
	return readers_[curnr_]->toStart();
    }

    return true;
}


bool CBVSReadMgr::toStart()
{
    if ( vertical_ )
    {
	for ( int idx=0; idx<readers_.size(); idx++ )
	{
	    if ( !readers_[idx]->toStart() )
		return false;
	}
	return true;
    }

    curnr_ = 0;
    return readers_[curnr_]->toStart();
}


bool CBVSReadMgr::skip( bool fnp )
{
    if ( vertical_ )
    {
	for ( int idx=0; idx<readers_.size(); idx++ )
	{
	    if ( !readers_[idx]->skip(fnp) )
		return false;
	}
	return true;
    }

    if ( !readers_[curnr_]->skip(fnp) )
    {
	if ( curnr_ == readers_.size()-1 ) return false;
	curnr_++;
	return readers_[curnr_]->toStart();
    }

    return true;
}


bool CBVSReadMgr::getHInfo( CBVSInfo::ExplicitData& ed )
{
    return readers_[curnr_]->getHInfo( ed );
}


bool CBVSReadMgr::fetch( void** d, const bool* c,
			 const Interval<int>* selsamps )
{
    if ( !vertical_ )
	return readers_[curnr_]->fetch( d, c, selsamps );

    const BasicComponentInfo& ci = *info_.compinfo[0];
    int nb = (int)ci.datachar.nrBytes();
    int ioffs = 0;
    Interval<int> samps( 0, 0 );
    int sampoffs = 0;
    for ( int idx=0; idx<readers_.size(); idx++ )
    {
	int nrsampsavailable = readers_[idx]->info().compinfo[0]->nrsamples;
	samps.stop += nrsampsavailable - 1;
	/*
	bool done = selsamps && samps.stop >= selsamps->stop;
	if ( done ) samps.stop = selsamps->stop;

	if ( !selsamps
	  || selsamps->stop >= samps.start || selsamps->start <= samps.stop )
	{
	    if ( !readers_[idx]->fetch(fnp) )
		return false;
	}
	if ( done ) break;
	*/

	samps.start = samps.stop + 1;
    }

    return true;
}


int CBVSReadMgr::nrComponents() const
{
    return readers_[curnr_]->nrComponents();
}


const BinID& CBVSReadMgr::binID() const
{
    return readers_[curnr_]->binID();
}


const char* CBVSReadMgr::check( const char* basefname )
{
    static BufferString ret;

    int curnr=0;
    for ( ; ; curnr++ )
    {
	BufferString fname = getFileName( basefname, curnr );
	if ( !File_exists((const char*)fname) ) break;

	StreamData sd = StreamProvider(fname).makeIStream();
	const char* res = CBVSReader::check( *sd.istrm );

	if ( res && *res )
	{
	    ret = "'"; ret += fname; ret += "': ";
	    ret += res;
	    return (const char*)ret;
	}
    }

    if ( curnr == 0 )
    {
	ret = "'"; ret += basefname; ret += "' does not exist";
	return (const char*)ret;
    }

    return 0;
}
