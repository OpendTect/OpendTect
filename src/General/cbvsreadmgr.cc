/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : April 2001
 * FUNCTION : CBVS File pack reading
-*/

static const char* rcsID = "$Id: cbvsreadmgr.cc,v 1.4 2001-05-23 07:54:59 bert Exp $";

#include "cbvsreadmgr.h"
#include "cbvsreader.h"
#include "filegen.h"
#include <fstream.h>


static inline void mkErrMsg( BufferString& errmsg, const char* fname,
			     const char* msg )
{
    errmsg = "'"; errmsg += fname; errmsg += "' ";
    errmsg += msg;
}


CBVSReadMgr::CBVSReadMgr( const char* fnm )
	: CBVSIOMgr(fnm)
	, info_(*new CBVSInfo)
{
    while ( 1 )
    {
	BufferString fname = getFileName( readers_.size() );
	if ( !File_exists((const char*)fname) ) break;

	if ( !addReader(fname) ) return;
	fnames_ += new BufferString( fname );
    }

    if ( !readers_.size() )
	mkErrMsg( errmsg_, basefname_, "does not exist" );
    else
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


bool CBVSReadMgr::addReader( const char* fname )
{
    istream* newstrm = new ifstream( fname );
    if ( !newstrm || !*newstrm )
    {
	mkErrMsg( errmsg_, fname, "cannot be opened" );
	return false;
    }

    CBVSReader* newrdr = new CBVSReader( newstrm );
    if ( newrdr->errMsg() )
    {
	mkErrMsg( errmsg_, fname, newrdr->errMsg() );
	return false;
    }

    readers_ += newrdr;
    return true;
}


void CBVSReadMgr::createInfo()
{
    if ( readers_.size() == 0 ) return;
    info_ = readers_[0]->info();

    for ( int idx=1; idx<readers_.size(); idx++ )
	if ( !handleInfo(readers_[idx],idx) ) return;
}


#define mErrRet(s) { \
    errmsg_ = s; \
    errmsg_ += " for:\n"; \
    errmsg_ += *fnames_[ireader]; \
    errmsg_ += "\ndiffers from first file"; \
    return false; \
}

bool CBVSReadMgr::handleInfo( CBVSReader* rdr, int ireader )
{
    const CBVSInfo& ci = rdr->info();
    if ( ci.nrtrcsperposn != info_.nrtrcsperposn )
	mErrRet("Number of traces per position")
    if ( ci.geom.step.inl != info_.geom.step.inl )
	mErrRet("In-line number step")
    if ( ci.geom.step.crl != info_.geom.step.crl )
	mErrRet("Cross-line number step")

    for ( int icomp=0; icomp<ci.compinfo.size(); icomp++ )
    {
	const BasicComponentInfo& cicompinf = *ci.compinfo[icomp];
	const BasicComponentInfo& compinf = *info_.compinfo[icomp];
	if ( cicompinf.nrsamples != compinf.nrsamples )
	    mErrRet("Number of samples")
	if ( !mIS_ZERO(cicompinf.sd.step-compinf.sd.step) )
	    mErrRet("Sample interval")
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
	    rdrnr = nextRdrNr( rdrnr );
	    if ( rdrnr < 1 ) break;
	}
	ret = readers_[rdrnr]->nextBinID();
    }

    return ret;
}


bool CBVSReadMgr::goTo( const BinID& bid )
{
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
    curnr_ = 0;
    return readers_[curnr_]->toStart();
}


bool CBVSReadMgr::skip( bool fnp )
{
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


bool CBVSReadMgr::fetch( void** d, const Interval<int>* s )
{
    return readers_[curnr_]->fetch( d, s );
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

	istream* strm = new ifstream( (const char*)fname );
	const char* res = CBVSReader::check( *strm );
	delete strm;

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
