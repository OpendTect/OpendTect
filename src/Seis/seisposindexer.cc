/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2008
-*/
static const char* rcsID = "$Id$";

#include "seisposindexer.h"
#include "idxable.h"
#include "datainterp.h"
#include "survinfo.h"

#include "strmoper.h"
#include "strmprov.h"
#include <fstream>

Seis::PosIndexer::PosIndexer( const Seis::PosKeyList& pkl, bool doindex,
       			      bool excludeunreasonable )
    : pkl_(pkl)
    , strm_( 0 )
    , int32interp_( 0 )
    , int64interp_( 0 )
    , curinl_( -1 )
    , excludeunreasonable_( excludeunreasonable )
    , goodinlrg_( SI().inlRange(false) )
    , goodcrlrg_( SI().crlRange(false) )
{
    int inlwidth = goodinlrg_.width();
    if ( inlwidth<1 )
	inlwidth = 100;

    const int inlexpansion = inlwidth*5;
    goodinlrg_.start -= inlexpansion;
    if ( goodinlrg_.start<0 ) goodinlrg_.start = 1;
    goodinlrg_.stop += inlexpansion;

    int crlwidth = goodcrlrg_.width();
    if ( crlwidth<1 )
	crlwidth = 100;

    const int crlexpansion = crlwidth*5;
    goodcrlrg_.start -= crlexpansion;
    if ( goodcrlrg_.start<0 ) goodcrlrg_.start = 1;
    goodcrlrg_.stop += crlexpansion;

    if ( doindex )
	reIndex();
}


Seis::PosIndexer::~PosIndexer()
{
    empty();
    delete strm_;
    delete int32interp_;
    delete int64interp_;
}


void Seis::PosIndexer::empty()
{
    inls_.erase();
    inlfileoffsets_.erase();
    deepErase( crlsets_ );
    deepErase( idxsets_ );
    maxidx_ = -1;
    curinl_ = -1;
    inlrg_.start = inlrg_.stop = crlrg_.start = crlrg_.stop = 0;
    offsrg_.start = offsrg_.stop = 0;
    nrrejected_ = 0;
}

#define mWrite( val ) \
    strm.write( (const char*) &val, sizeof(val))

bool Seis::PosIndexer::dumpTo( std::ostream& strm ) const
{
    mWrite( is2d_ );
    mWrite( isps_ );
    mWrite ( inlrg_.start );
    mWrite ( inlrg_.stop );
    mWrite ( crlrg_.start );
    mWrite ( crlrg_.stop );
    mWrite ( offsrg_.start );
    mWrite ( offsrg_.stop );

    mWrite( maxidx_ );

    TypeSet<od_int64> inloffsets( inls_.size(), 0 );
    od_int64 offsetstart = 0;
    if ( !is2d_ )
    {
	const int nrinl = inls_.size();
	mWrite( nrinl );
	strm.write( (const char*) inls_.arr(), sizeof(int) * nrinl );
	offsetstart = strm.tellp();
	strm.write( (const char*) inloffsets.arr(),
		sizeof(od_int64) * inloffsets.size() );
    }

    const int nrlines = is2d_ ? 1 : inls_.size();

    for ( int lineidx=0; lineidx<nrlines; lineidx++ )
    {
	const TypeSet<int>& crlset = *crlsets_[lineidx];
	const TypeSet<od_int64>& idxset = *idxsets_[lineidx];
	const int nrtrcs = crlset.size();
	if ( !is2d_ ) inloffsets[lineidx] = strm.tellp();
	mWrite( nrtrcs );
	strm.write( (const char*) crlset.arr(), sizeof(int) * nrtrcs );
	strm.write( (const char*) idxset.arr(), sizeof(od_int64) * nrtrcs );
    }

    if ( !is2d_ )
    {
	od_int64 eof = strm.tellp();
	strm.seekp( offsetstart, std::ios::beg );
	strm.write( (const char*) inloffsets.arr(),
		sizeof(od_int64) * inloffsets.size() );
	strm.seekp( eof, std::ios::beg );
    }

    return strm.good();
}

#define mRead( var, interp, sz ) \
    if ( interp ) \
    { \
	char buf[8]; \
	strm_->read( (char*) buf, 8 ); \
	var = interp->get( buf, 0 ); \
    } \
    else \
	strm_->read( (char*) &var, sizeof(var) )

bool Seis::PosIndexer::readFrom( const char* fnm, od_int64 offset,
	bool readall,
	DataInterpreter<int>* int32interp,
	DataInterpreter<od_int64>* int64interp,
        DataInterpreter<float>* floatinterp )
{
    if ( strm_ )
	delete strm_;

    strm_ = StreamProvider( fnm ).makeIStream().istrm; 
    if ( !strm_->good() )
    {
	delete strm_;
	strm_ = 0;
	return false;
    }

    StrmOper::seek( *strm_, offset, std::ios::beg );
    if ( !strm_->good() )
    {
	delete strm_;
	strm_ = 0;
	return false;
    }

    if ( !readHeader( int32interp, int64interp, floatinterp ) )
    {
	delete strm_;
	strm_ = 0;
	return false;
    }

    if ( !readall )
    {
	delete int32interp_;
	int32interp_ = int32interp
	    ? new DataInterpreter<int>( *int32interp ) : 0;
	delete int64interp_;
	int64interp_ = int64interp
	    ? new DataInterpreter<od_int64>( *int64interp ) : 0;
	return true;
    }

    inlfileoffsets_.erase();

    const int nrlines = is2d_ ? 1 : inls_.size();

    for ( int lineidx=0; lineidx<nrlines; lineidx++ )
    {
	TypeSet<int>* crlset = new TypeSet<int>;
	TypeSet<od_int64>* idxset = new TypeSet<od_int64>;
	if ( !readLine( *crlset, *idxset, int32interp, int64interp ) )
	{
	    delete crlset;
	    delete idxset;
	    delete strm_;
	    strm_ = 0;
	    return false;
	}

	crlsets_ += crlset;
	idxsets_ += idxset;
    }

    const bool res = strm_->good();
    delete strm_;
    strm_ = 0;
    return res;
}


bool Seis::PosIndexer::readLine( TypeSet<int>& crlset,
	TypeSet<od_int64>& idxset,
	DataInterpreter<int>* int32interp,
	DataInterpreter<od_int64>* int64interp ) const
{
    int nrtrcs = DataInterpreter<int>::get( int32interp, *strm_ );
    if ( !strm_->good() )
	return false;

    crlset.setSize( nrtrcs, 0 );
    char* buf;
    int sz;
    ArrPtrMan<char> mybuf = 0;
    if ( int32interp )
    {
	buf = mybuf = new char[sizeof(int)*int32interp->nrBytes()];
	sz = int32interp->nrBytes();
    }
    else
    {
	sz = sizeof(int);
	buf = (char*) crlset.arr();
    }

    strm_->read( buf, sz*nrtrcs );
    if ( !strm_->good() )
	return false;

    if ( int32interp )
    {
	for ( int idx=0; idx<nrtrcs; idx++ )
	    crlset[idx] = int32interp->get( buf, idx );
    }

    idxset.setSize( nrtrcs, 0 );
    if ( int64interp )
    {
	buf = mybuf = new char[sizeof(int)*int64interp->nrBytes()];
	sz = int64interp->nrBytes();
    }
    else
    {
	sz = sizeof(od_int64);
	buf = (char*) idxset.arr();
    }

    strm_->read( buf, sz*nrtrcs );
    if ( !strm_->good() )
	return false;

    if ( int64interp )
    {
	for ( int idx=0; idx<nrtrcs; idx++ )
	    idxset[idx] = int64interp->get( buf, idx );
    }

    return strm_->good();
}


bool Seis::PosIndexer::readHeader( 
	DataInterpreter<int>* int32interp,
	DataInterpreter<od_int64>* int64interp,
        DataInterpreter<float>* floatinterp )
{
    empty();
    strm_->read( (char*) &is2d_, sizeof(is2d_) );
    strm_->read( (char*) &isps_, sizeof(isps_) );

    mRead ( inlrg_.start, int32interp, 4 );
    mRead ( inlrg_.stop, int32interp, 4 );
    mRead ( crlrg_.start, int32interp, 4 );
    mRead ( crlrg_.stop, int32interp, 4 );
    mRead ( offsrg_.start, floatinterp, 4 );
    mRead ( offsrg_.stop, floatinterp, 4 );

    mRead ( maxidx_, int64interp, 8 );

    if ( !strm_->good() )
	return false;

    if ( !is2d_ )
    {
	int nrinl;
	mRead( nrinl, int32interp, 4 );
	if ( !strm_->good() )
	    return false;

	char* buf;
	int sz;
	ArrPtrMan<char> mybuf = 0;
	inls_.setSize( nrinl, 0 );
	if ( int32interp )
	{
	    buf = mybuf = new char[sizeof(int)*int32interp->nrBytes()];
	    sz = int32interp->nrBytes();
	}
	else
	{
	    sz = sizeof(int);
	    buf = (char*) inls_.arr();
	}

	strm_->read( buf, sz*nrinl );
	if ( !strm_->good() )
	    return false;

	if ( int32interp )
	{
	    for ( int idx=0; idx<nrinl; idx++ )
		inls_[idx] = int32interp->get( buf, idx );
	}

	inlfileoffsets_.setSize( nrinl, 0 );
	if ( int64interp )
	{
	    buf = mybuf = new char[sizeof(int)*int64interp->nrBytes()];
	    sz = int64interp->nrBytes();
	}
	else
	{
	    sz = sizeof(od_int64);
	    buf = (char*) inlfileoffsets_.arr();
	}

	strm_->read( buf, sz*nrinl );
	if ( !strm_->good() )
	    return false;

	if ( int64interp )
	{
	    for ( int idx=0; idx<nrinl; idx++ )
		inlfileoffsets_[idx] = int64interp->get( buf, idx );
	}
    }

    return strm_->good();
}


   
inline static int getIndex( const TypeSet<int>& nrs, int nr, bool& present )
{
    int ret;
    present = IdxAble::findPos( nrs.arr(), nrs.size(), nr, -1, ret );
    return ret;
}



int Seis::PosIndexer::getFirstIdxs( const BinID& bid,
				    int& inlidx, int& crlidx )
{
    if ( inls_.isEmpty() )
	return -1;

    bool pres = true;
    inlidx = is2d_ ? 0 : getIndex( inls_, bid.inl, pres );
    if ( !pres )
	{ crlidx = -1; return -1; }


    const TypeSet<int>* crlsetptr = 0;

    if ( strm_ )
    {
	if ( curinl_!=bid.inl )
	{
	    StrmOper::seek( *strm_, inlfileoffsets_[inlidx], std::ios::beg );
	    if ( !readLine(curcrlset_,curidxset_,int32interp_,int64interp_ ) )
		return -1;
	    curinl_ = bid.inl;
	}

	crlsetptr = &curcrlset_;
    }
    else
	crlsetptr = crlsets_[inlidx];

    crlidx = getIndex( *crlsetptr, bid.crl, pres );
    if ( !pres )
	return -2;

    return 0;
}


void Seis::PosIndexer::getCrls( int inl, TypeSet<int>& crls ) const
{
    Threads::MutexLocker lock( lock_ );

    if ( inls_.isEmpty() )
	return;

    bool pres = true;
    int inlidx = is2d_ ? 0 : getIndex( inls_, inl, pres );
    if ( !pres )
	return;

    const TypeSet<int>* crlsetptr = 0;

    if ( strm_ )
    {
	if ( curinl_!=inl )
	{
	    StrmOper::seek( *strm_, inlfileoffsets_[inlidx], std::ios::beg );
	    if ( !readLine(const_cast<Seis::PosIndexer*>(this)->curcrlset_,
			   const_cast<Seis::PosIndexer*>(this)->curidxset_,
			   const_cast<Seis::PosIndexer*>(this)->int32interp_,
			   const_cast<Seis::PosIndexer*>(this)->int64interp_ ) )
		return;
	    const_cast<Seis::PosIndexer*>(this)->curinl_ = inl;
	}

	crls = curcrlset_;
    }
    else
	crls = *crlsets_[inlidx];
}


od_int64 Seis::PosIndexer::findFirst( const BinID& bid ) const
{
    Threads::MutexLocker lock( lock_ );
    int inlidx, crlidx;
    const int res =
	const_cast<Seis::PosIndexer*>(this)->getFirstIdxs( bid, inlidx, crlidx);
    if ( res < 0 ) return res;

    if ( strm_ )
	return curidxset_[crlidx];

    return (*idxsets_[inlidx])[crlidx];
}


od_int64 Seis::PosIndexer::findFirst( int trcnr ) const
{
    return findFirst( BinID(1,trcnr) );
}


od_int64 Seis::PosIndexer::findFirst( const Seis::PosKey& pk, bool wo ) const
{
    Threads::MutexLocker lock( lock_ );
    int inlidx, crlidx;
    const int res =
	const_cast<Seis::PosIndexer*>(this)->getFirstIdxs( pk.binID(),
							  inlidx, crlidx );
    if ( res < 0 ) return res;

    od_int64 ret = strm_ ? curidxset_[crlidx] : (*idxsets_[inlidx])[crlidx];
    if ( !wo ) return ret;

    for ( ; ret<=maxidx_; ret++ )
    {
	const PosKey curpk( pkl_.key(ret) );
	if ( curpk.isUndef() )
	    continue;
	else if ( curpk.binID() != pk.binID() )
	    break;
	else if ( curpk.hasOffset(pk.offset()) )
	    return ret;
    }

    return -3;
}


od_int64 Seis::PosIndexer::findOcc( const Seis::PosKey& pk, int occ ) const
{
    Threads::MutexLocker lock( lock_ );
    int inlidx, crlidx;
    const int res =
	const_cast<Seis::PosIndexer*>(this)->getFirstIdxs( pk.binID(),
							  inlidx, crlidx );
    if ( res < 0 ) return res;

    od_int64 ret = strm_ ? curidxset_[crlidx] : (*idxsets_[inlidx])[crlidx];
    if ( occ < 1 ) return ret;

    for ( ret++; ret<=maxidx_; ret++ )
    {
	const PosKey curpk( pkl_.key(ret) );
	if ( curpk.isUndef() )
	    continue;
	else if ( curpk.binID() != pk.binID() )
	    break;

	occ--;
	if ( occ == 0 )
	    return ret;
    }

    return -1;
}


TypeSet<od_int64> Seis::PosIndexer::findAll( const Seis::PosKey& pk ) const
{
    Threads::MutexLocker lock( lock_ );
    TypeSet<od_int64> retidxs;
    int inlidx, crlidx;
    int res =
	const_cast<Seis::PosIndexer*>(this)->getFirstIdxs( pk.binID(),
							  inlidx, crlidx );
    if ( res < 0 ) return retidxs;

    od_int64 idx = strm_ ? curidxset_[crlidx] : (*idxsets_[inlidx])[crlidx];
    retidxs += idx;

    for ( ; idx<=maxidx_; idx++ )
    {
	const PosKey curpk( pkl_.key(idx) );
	if ( curpk.isUndef() )
	    continue;
	else if ( curpk.binID() != pk.binID() )
	    break;
	retidxs += idx;
    }

    return retidxs;
}


void Seis::PosIndexer::reIndex()
{
    empty();

    const od_int64 sz = pkl_.size();
    for ( od_int64 idx=0; idx<sz; idx++ )
	add( pkl_.key( idx ), idx );
}


bool Seis::PosIndexer::isReasonable( const BinID& bid ) const
{
    if ( is2d_ )
	return true;

    if ( !goodinlrg_.includes( bid.inl,false ) )
	return false;

    if ( !goodcrlrg_.includes( bid.crl,false ) )
	return false;

    return true;
}


void Seis::PosIndexer::add( const Seis::PosKey& pk, od_int64 posidx )
{
    if ( pk.isUndef() )
    {
	nrrejected_++;
	return;
    }

    if ( crlsets_.isEmpty() )
    {
	is2d_ = Seis::is2D( pk.geomType() );
	isps_ = Seis::isPS( pk.geomType() );

	inlrg_.start = inlrg_.stop = is2d_ ? 1 : pk.inLine();
	crlrg_.start = crlrg_.stop = pk.xLine();
	offsrg_.start = offsrg_.stop = isps_ ? pk.offset() : 0;
    }

    if ( excludeunreasonable_ && !isReasonable( pk.binID() ) )
    {
	nrrejected_++;
	return;
    }

    maxidx_ = posidx;
    if ( isps_ ) offsrg_.include( pk.offset() );

    bool ispresent = !inls_.isEmpty();
    int inlidx = is2d_ ? 0 : getIndex( inls_, pk.inLine(), ispresent );
    if ( !ispresent )
    {
	if ( inlidx >= inls_.size() - 1 )
	{
	    inls_ += pk.inLine();
	    crlsets_ += new TypeSet<int>;
	    idxsets_ += new TypeSet<od_int64>;
	    inlidx = inls_.size() - 1;
	}
	else
	{
	    inlidx++;
	    inls_.insert( inlidx, pk.inLine() );
	    crlsets_.insertAt( new TypeSet<int>, inlidx );
	    idxsets_.insertAt( new TypeSet<od_int64>, inlidx );
	}

	if ( !is2d_ ) inlrg_.include( pk.inLine() );
    }

    TypeSet<int>& crls = *crlsets_[inlidx];
    TypeSet<od_int64>& idxs = *idxsets_[inlidx];
    int crlidx = getIndex( crls, pk.xLine(), ispresent );
    if ( ispresent )
	return;

    crlrg_.include( pk.xLine() );

    if ( crlidx >= crls.size()-1 )
    {
	crls += pk.xLine();
	idxs += posidx;
    }
    else
    {
	crlidx++;
	crls.insert( crlidx, pk.xLine() );
	idxs.insert( crlidx, posidx );
    }
}
