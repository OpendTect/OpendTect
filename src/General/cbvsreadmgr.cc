/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : April 2001
 * FUNCTION : CBVS File pack reading
-*/

static const char* rcsID = "$Id: cbvsreadmgr.cc,v 1.1 2001-04-18 16:24:05 bert Exp $";

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
	: basefname_(fnm)
	, info_(*new CBVSInfo)
	, curreader_(0)
{
    BufferString fname( basefname_ );
    while ( File_exists((const char*)fname) )
    {
	if ( !addReader(fname) ) return;
	fnames_ += new BufferString( fname );

	int curnr = readers_.size() + 1;
	fname = basefname_;
	char* ptr = strrchr( fname.buf(), '.' );
	BufferString ext;
	if ( ptr ) { ext = ptr; *ptr = '\0'; }
	fname += curnr < 10 ? "0^" : "^";
	fname += ext;
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
	const CBVSComponentInfo& cicompinf = *ci.compinfo[icomp];
	const CBVSComponentInfo& compinf = *info_.compinfo[icomp];
	if ( cicompinf.nrsamples != compinf.nrsamples )
	    mErrRet("Number of samples")
	if ( !mIS_ZERO(cicompinf.sd.step-compinf.sd.step) )
	    mErrRet("Sample interval")
    }

    if ( !ci.geom.fullyrectandreg || !info_.geom.fullyrectandreg )
	return mergeIrreg( ci.geom, ireader );

    int expected_inline = info_.geom.stop.inl + info_.geom.step.inl;
    bool isafter = true;
    if ( ci.geom.start.inl != expected_inline )
    {
	expected_inline = info_.geom.start.inl - info_.geom.step.inl;
	if ( ci.geom.stop.inl == expected_inline )
	    isafter = false;
	else
	    return mergeIrreg( ci.geom, ireader );
    }

    if ( isafter ) info_.geom.stop.inl = ci.geom.stop.inl;
    else	   info_.geom.start.inl = ci.geom.start.inl;

    StepInterval<int> crls( info_.geom.start.crl, info_.geom.stop.crl,
			    info_.geom.step.crl );
    crls.include( ci.geom.start.crl );
    crls.include( ci.geom.stop.crl );
    info_.geom.start.crl = crls.start;
    info_.geom.stop.crl = crls.stop;

    return true;
}


bool CBVSReadMgr::mergeIrreg( const CBVSInfo::SurvGeom& geom, int ireader )
{
    return true;
}
