/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2008
-*/

#include "seisposindexer.h"
#include "idxable.h"
#include "datainterp.h"
#include "survinfo.h"
#include "od_iostream.h"

static const int cMaxReasonableNrSegs = 100000;


Seis::PosIndexer::PosIndexer( const PosKeyList& pkl, bool doindex, bool excl )
    : pkl_(pkl)
    , strm_(0)
    , int32interp_(0)
    , int64interp_(0)
    , curinl_(-1)
    , excludeunreasonable_(excl)
    , goodinlrg_( SI().inlRange() )
    , goodcrlrg_( SI().crlRange() )
    , iocompressed_(true)
    , is2d_(false), isps_(false) // keep compiler happy; still need to be set
{
    pos_type inlwidth = goodinlrg_.width();
    if ( inlwidth < 1 )
	inlwidth = 100;

    const pos_type inlexpansion = inlwidth*5;
    goodinlrg_.start -= inlexpansion;
    if ( goodinlrg_.start<0 ) goodinlrg_.start = 1;
    goodinlrg_.stop += inlexpansion;

    pos_type crlwidth = goodcrlrg_.width();
    if ( crlwidth<1 )
	crlwidth = 100;

    const pos_type crlexpansion = crlwidth*5;
    goodcrlrg_.start -= crlexpansion;
    if ( goodcrlrg_.start<0 ) goodcrlrg_.start = 1;
    goodcrlrg_.stop += crlexpansion;

    if ( doindex )
	reIndex();
}


Seis::PosIndexer::~PosIndexer()
{
    setEmpty();
    delete strm_;
    delete int32interp_;
    delete int64interp_;
}


void Seis::PosIndexer::setEmpty()
{
    inls_.erase();
    inlfileoffsets_.erase();
    deepErase( crlsets_ );
    deepErase( fileidxsets_ );
    maxfileidx_ = -1;
    curinl_ = -1;
    inlrg_.start = inlrg_.stop = crlrg_.start = crlrg_.stop = 0;
    offsrg_.start = offsrg_.stop = 0;
    nrrejected_ = 0;
}


#define mWrite( val ) strm.addBin( val )

bool Seis::PosIndexer::dumpTo( od_ostream& strm ) const
{
    mWrite( is2d_ );
    mWrite( isps_ );
    mWrite( inlrg_.start );
    mWrite( inlrg_.stop );
    mWrite( crlrg_.start );
    mWrite( crlrg_.stop );
    mWrite( offsrg_.start );
    mWrite( offsrg_.stop );
    mWrite( maxfileidx_ );

    TypeSet<FileOffsType> inlfileoffsets( inls_.size(), 0 );
    FileOffsType posofinlfileoffsets = 0;
    if ( !is2d_ )
    {
	const size_type nrinl = inls_.size();
	mWrite( nrinl );
	strm.addBin( inls_.arr(), sizeof(pos_type) * nrinl );
	posofinlfileoffsets = strm.position();
	// write to occupy file space for now
	strm.addBin( inlfileoffsets.arr(),
		     sizeof(FileOffsType)*inlfileoffsets.size() );
    }

    const size_type nrlines = is2d_ ? 1 : inls_.size();
    if ( crlsets_.size() <  nrlines )
	return false;

    for ( idx_type lineidx=0; lineidx<nrlines; lineidx++ )
    {
	if ( !is2d_ )
	    inlfileoffsets[lineidx] = strm.position();

	const PosSet& crlset = *crlsets_[lineidx];
	const size_type nrtrcs = crlset.size();
	mWrite( nrtrcs );

	const FileIdxSet& fileidxs = *fileidxsets_[lineidx];
	if ( iocompressed_ )
	    dumpLineCompressed( strm, crlset, fileidxs );
	else
	{
	    strm.addBin( crlset.arr(), sizeof(idx_type) * nrtrcs );
	    strm.addBin( fileidxs.arr(), sizeof(FileIdxType) * nrtrcs );
	}
    }

    if ( !is2d_ )
    {
	// Now write the offsets of the inline starts for real
	FileOffsType endofdataoffs = strm.position();
	strm.setWritePosition( posofinlfileoffsets );
	strm.addBin( inlfileoffsets.arr(),
		     sizeof(FileOffsType)*inlfileoffsets.size() );
	strm.setWritePosition( endofdataoffs );
    }

    return !strm.isBad();
}


#define mRead( var, interp, sz ) \
    if ( interp ) \
    { \
	char buf[8]; \
	strm_->getBin( buf, sz ); \
	var = interp->get( buf, 0 ); \
    } \
    else \
	strm_->getBin( var )

#   define mRet(yn) { strm_ = 0; return yn; }


bool Seis::PosIndexer::readFrom( const char* fnm, od_stream_Pos offset,
	bool readall, Int32Interpreter* int32interp,
	Int64Interpreter* int64interp, FloatInterpreter* floatinterp )
{
    if ( strm_ )
	{ pErrMsg("strm_ not null"); delete strm_; strm_ = 0; }

    od_istream* strm = new od_istream( fnm );
    if ( !strm->isOK() )
	mRet( false )
    strm->setReadPosition( offset );
    if ( !strm->isOK() )
	mRet( false )

    strm_ = strm;
    if ( !readHeader( int32interp, int64interp, floatinterp ) )
	mRet( false )

    if ( !is2d_ && !readall )
    {
	delete int32interp_;
	int32interp_ = int32interp
	    ? new Int32Interpreter( *int32interp ) : 0;
	delete int64interp_;
	int64interp_ = int64interp
	    ? new Int64Interpreter( *int64interp ) : 0;
	return true;
    }

    inlfileoffsets_.erase();
    const size_type nrlines = is2d_ ? 1 : inls_.size();

    for ( idx_type lineidx=0; lineidx<nrlines; lineidx++ )
    {
	PosSet* crlset = new PosSet;
	FileIdxSet* fileidxs = new FileIdxSet;
	if ( !readLine( *crlset, *fileidxs, int32interp, int64interp ) )
	{
	    delete crlset; delete fileidxs;
	    mRet( false )
	}

	crlsets_ += crlset;
	fileidxsets_ += fileidxs;
    }

    const bool res = !strm_->isBad();
    mRet( res )
}


bool Seis::PosIndexer::readLine( PosSet& crlset, FileIdxSet& fileidxs,
	Int32Interpreter* int32interp, Int64Interpreter* int64interp ) const
{
    const size_type nrtrcs = Int32Interpreter::get( int32interp, *strm_ );
    if ( strm_->isBad() )
	return false;
    else if ( nrtrcs < 1 )
	return true;

    crlset.setSize( nrtrcs, 0 );
    fileidxs.setSize( nrtrcs, 0 );

    if ( iocompressed_ )
	return readLineCompressed( crlset, fileidxs );

    char* buf; int bytesz;
    ArrPtrMan<char> mybuf = 0;
    if ( int32interp )
    {
	buf = mybuf = new char[sizeof(int)*int32interp->nrBytes()];
	bytesz = int32interp->nrBytes();
    }
    else
    {
	buf = (char*)crlset.arr();
	bytesz = sizeof(size_type);
    }

    strm_->getBin( buf, bytesz*nrtrcs );
    if ( strm_->isBad() )
	return false;

    if ( int32interp )
    {
	for ( idx_type idx=0; idx<nrtrcs; idx++ )
	    crlset[idx] = int32interp->get( buf, idx );
    }

    if ( int64interp )
    {
	buf = mybuf = new char[sizeof(idx_type)*int64interp->nrBytes()];
	bytesz = int64interp->nrBytes();
    }
    else
    {
	buf = (char*)fileidxs.arr();
	bytesz = sizeof( FileIdxType );
    }

    strm_->getBin( buf, bytesz*nrtrcs );
    if ( strm_->isBad() )
	return false;

    if ( int64interp )
    {
	for ( idx_type idx=0; idx<nrtrcs; idx++ )
	    fileidxs[idx] = int64interp->get( buf, idx );
    }

    return !strm_->isBad();
}


void Seis::PosIndexer::dumpLineCompressed( od_ostream& strm,
		    const PosSet& crlset, const FileIdxSet& fileidxs ) const
{
    const size_type nrtrcs = crlset.size();
    if ( nrtrcs < 1 )
	return;

    StepInterval<pos_type> crlseg; StepInterval<FileIdxType> fiseg;
    crlseg.start = crlset[0]; fiseg.start = fileidxs[0];
    if ( nrtrcs < 2 ) // one crl: can be crl-sorted, keep it short
	{ strm.addBin( crlseg.start ).addBin( fiseg.start ); return; }

    crlseg.stop = crlset[1]; fiseg.stop = fileidxs[1];
    crlseg.step = crlseg.stop - crlseg.start;
    fiseg.step = fiseg.stop - fiseg.start;

    TypeSet< StepInterval<pos_type> > crlsegs;
    TypeSet< StepInterval<FileIdxType> > fisegs;
    for ( idx_type itrc=2; itrc<nrtrcs; itrc++ )
    {
	const pos_type crl = crlset[itrc];
	const FileIdxType fidx = fileidxs[itrc];
	const pos_type predcrl = crlseg.stop + crlseg.step;
	const FileIdxType predfidx = fiseg.stop + fiseg.step;
	if ( crl == predcrl && fidx == predfidx )
	    { crlseg.stop = crl; fiseg.stop = fidx; }
	else
	{
	    crlsegs += crlseg; fisegs += fiseg;

	    crlseg.step = crl - crlseg.stop;
	    fiseg.step = fidx - fiseg.stop;
	    crlseg.start = crlseg.stop = crl;
	    fiseg.start = fiseg.stop = fidx;
	}
    }
    crlsegs += crlseg; fisegs += fiseg;

    const size_type nrsegs = crlsegs.size();
    mWrite( nrsegs );
    for ( idx_type iseg=0; iseg<nrsegs; iseg++ )
    {
	crlseg = crlsegs[iseg]; fiseg = fisegs[iseg];
	strm.addBin( crlseg.start ).addBin( crlseg.stop ).addBin( crlseg.step )
	    .addBin( fiseg.start ).addBin( fiseg.stop ).addBin( fiseg.step );
    }
}


bool Seis::PosIndexer::readLineCompressed( PosSet& crlset,
					   FileIdxSet& fileidxs ) const
{
    const size_type nrtrcs = crlset.size(); // already set
    StepInterval<pos_type> crlseg; StepInterval<FileIdxType> fiseg;
    if ( nrtrcs < 2 )
    {
	if ( nrtrcs > 0 )
	{
	    // single crl; special case ...
	    strm_->getBin( crlseg.start ).getBin( fiseg.start );
	    crlset[0] = crlseg.start;
	    fileidxs[0] = fiseg.start;
	}
    }
    else
    {
	crlset.erase(); fileidxs.erase();
	size_type nrsegs = 0;
	strm_->getBin( nrsegs );
	if ( nrsegs > cMaxReasonableNrSegs )
	{
	    ErrMsg( BufferString("Found ",nrsegs," trace segments.") );
	    return false;
	}
	if ( nrsegs > 0 && strm_->isOK() )
	{
	    for ( idx_type iseg=0; iseg<nrsegs; iseg++ )
	    {
		strm_->getBin( crlseg.start ).getBin( crlseg.stop )
		      .getBin( crlseg.step ).getBin( fiseg.start )
		      .getBin( fiseg.stop ).getBin( fiseg.step );

		if ( (crlseg.step > 0 && crlseg.start > crlseg.stop)
		  || (crlseg.step < 0 && crlseg.start < crlseg.stop) )
		    { pErrMsg("Huh"); continue; }

		while ( crlseg.start <= crlseg.stop )
		{
		    crlset += crlseg.start; fileidxs += fiseg.start;
		    if ( crlseg.step == 0 )
			break;
		    crlseg.start += crlseg.step; fiseg.start += fiseg.step;
		}
	    }
	}
    }
    return !strm_->isBad();
}


bool Seis::PosIndexer::readHeader( Int32Interpreter* int32interp,
	Int64Interpreter* int64interp, FloatInterpreter* floatinterp )
{
    setEmpty();
    strm_->getBin( const_cast<bool&>(is2d_) );
    strm_->getBin( const_cast<bool&>(isps_) );

    mRead ( inlrg_.start, int32interp, 4 );
    mRead ( inlrg_.stop, int32interp, 4 );
    mRead ( crlrg_.start, int32interp, 4 );
    mRead ( crlrg_.stop, int32interp, 4 );
    mRead ( offsrg_.start, floatinterp, 4 );
    mRead ( offsrg_.stop, floatinterp, 4 );

    mRead ( maxfileidx_, int64interp, 8 );

    if ( strm_->isBad() )
	return false;

    if ( !is2d_ )
    {
	size_type nrinl;
	mRead( nrinl, int32interp, 4 );
	if ( strm_->isBad() )
	    return false;

	char* buf;
	int bytesz;
	ArrPtrMan<char> mybuf = 0;
	inls_.setSize( nrinl, 0 );
	if ( int32interp )
	{
	    buf = mybuf = new char[sizeof(int)*int32interp->nrBytes()];
	    bytesz = int32interp->nrBytes();
	}
	else
	{
	    bytesz = sizeof(size_type);
	    buf = (char*) inls_.arr();
	}

	strm_->getBin( buf, bytesz*nrinl );
	if ( strm_->isBad() )
	    return false;

	if ( int32interp )
	{
	    for ( idx_type idx=0; idx<nrinl; idx++ )
		inls_[idx] = int32interp->get( buf, idx );
	}

	inlfileoffsets_.setSize( nrinl, 0 );
	if ( int64interp )
	{
	    buf = mybuf = new char[sizeof(idx_type)*int64interp->nrBytes()];
	    bytesz = int64interp->nrBytes();
	}
	else
	{
	    bytesz = sizeof( FileIdxType );
	    buf = (char*)inlfileoffsets_.arr();
	}

	strm_->getBin( buf, bytesz*nrinl );
	if ( strm_->isBad() )
	    return false;

	if ( int64interp )
	{
	    for ( idx_type idx=0; idx<nrinl; idx++ )
		inlfileoffsets_[idx] = int64interp->get( buf, idx );
	}
    }

    return !strm_->isBad();
}



inline static Seis::PosIndexer::idx_type
	getIndex( const Seis::PosIndexer::PosSet& nrs,
		  Seis::PosIndexer::pos_type nr, bool& present )
{
    Seis::PosIndexer::idx_type ret;
    present = IdxAble::findPos( nrs.arr(), nrs.size(), nr, -1, ret );
    return ret;
}



Seis::PosIndexer::idx_type Seis::PosIndexer::getFirstIdxs( const BinID& bid,
				idx_type& inlidx, idx_type& crlidx ) const
{
    if ( !is2d_ && inls_.isEmpty() )
	return -1;

    bool pres = true;
    inlidx = is2d_ ? 0 : getIndex( inls_, bid.inl(), pres );
    if ( !pres )
	{ crlidx = -1; return -1; }

    const PosSet* crlsetptr = 0;
    if ( !strm_ )
	crlsetptr = crlsets_[inlidx];
    else
    {
	if ( curinl_ != bid.inl() )
	{
	    strm_->setReadPosition( inlfileoffsets_[inlidx] );
	    if ( !readLine(curcrlset_,curfileidxs_,
			   int32interp_,int64interp_ ) )
		return -1;
	    curinl_ = bid.inl();
	}

	crlsetptr = &curcrlset_;
    }

    crlidx = getIndex( *crlsetptr, bid.crl(), pres );
    if ( !pres )
	return -2;

    return 0;
}


void Seis::PosIndexer::getCrls( pos_type inl, PosSet& crls ) const
{
    Threads::Locker lckr( lock_ );
    if ( inls_.isEmpty() )
	return;

    bool pres = true;
    const idx_type inlidx = is2d_ ? 0 : getIndex( inls_, inl, pres );
    if ( !pres )
	return;

    if ( strm_ )
    {
	if ( curinl_ != inl )
	{
	    strm_->setReadPosition( inlfileoffsets_[inlidx] );
	    if ( !readLine(curcrlset_,curfileidxs_,
			   const_cast<PosIndexer*>(this)->int32interp_,
			   const_cast<PosIndexer*>(this)->int64interp_ ) )
		return;
	    curinl_ = inl;
	}
	crls = curcrlset_;
    }
    else
	crls = *crlsets_[inlidx];
}


Seis::PosIndexer::FileIdxType Seis::PosIndexer::findFirst(
						const BinID& bid ) const
{
    Threads::Locker lckr( lock_ );
    idx_type inlidx, crlidx;
    const idx_type res = getFirstIdxs( bid, inlidx, crlidx);
    if ( res < 0 )
	return res;

    if ( strm_ )
	return curfileidxs_[crlidx];

    return (*fileidxsets_[inlidx])[crlidx];
}


Seis::PosIndexer::FileIdxType Seis::PosIndexer::findFirst(
						pos_type trcnr ) const
{
    return findFirst( BinID(1,trcnr) );
}


Seis::PosIndexer::FileIdxType Seis::PosIndexer::findFirst(
				const PosKey& pk, bool useoffs ) const
{
    Threads::Locker lckr( lock_ );
    idx_type inlidx, crlidx;
    const idx_type res = getFirstIdxs( pk.binID(), inlidx, crlidx );
    if ( res < 0 )
	return res;

    FileIdxType ret = strm_ ? curfileidxs_[crlidx]
			     : (*fileidxsets_[inlidx])[crlidx];
    if ( !useoffs )
	return ret;

    for ( ; ret<=maxfileidx_; ret++ )
    {
	PosKey curpk;
	if ( !pkl_.key( ret, curpk ) )
	    return -1;

	if ( curpk.isUndef() )
	    continue;
	else if ( curpk.binID() != pk.binID() )
	    break;
	else if ( curpk.hasOffset(pk.offset()) )
	    return ret;
    }

    return -3;
}


Seis::PosIndexer::FileIdxType Seis::PosIndexer::findOcc( const PosKey& pk,
							 int occ ) const
{
    Threads::Locker lckr( lock_ );
    idx_type inlidx, crlidx;
    const idx_type res = getFirstIdxs( pk.binID(), inlidx, crlidx );
    if ( res < 0 )
	return res;

    FileIdxType ret = strm_ ? curfileidxs_[crlidx]
			     : (*fileidxsets_[inlidx])[crlidx];
    if ( occ < 1 )
	return ret;

    PosKey curpk;
    for ( ret++; ret<=maxfileidx_; ret++ )
    {
	if ( !pkl_.key( ret, curpk ) )
	    return -1;

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


Seis::PosIndexer::FileIdxSet Seis::PosIndexer::findAll( const PosKey& pk ) const
{
    Threads::Locker lckr( lock_ );
    FileIdxSet retfileidxs;
    idx_type inlidx, crlidx;
    idx_type res = getFirstIdxs( pk.binID(), inlidx, crlidx );
    if ( res < 0 )
	return retfileidxs;

    FileIdxType fileidx = strm_ ? curfileidxs_[crlidx]
				: (*fileidxsets_[inlidx])[crlidx];
    retfileidxs += fileidx;

    PosKey curpk;
    for ( ; fileidx<=maxfileidx_; fileidx++ )
    {
	if ( !pkl_.key(fileidx,curpk) )
	    break;

	if ( curpk.isUndef() )
	    continue;
	else if ( curpk.binID() != pk.binID() )
	    break;
	retfileidxs += fileidx;
    }

    return retfileidxs;
}


void Seis::PosIndexer::reIndex()
{
    setEmpty();

    const FileIdxType sz = pkl_.size();
    PosKey curpk;
    for ( FileIdxType idx=0; idx<sz; idx++ )
    {
	if ( !pkl_.key( idx, curpk ) )
	    return;

	add( curpk, idx );
    }
}


bool Seis::PosIndexer::isReasonable( const BinID& bid ) const
{
    if ( is2d_ )
	return true;

    if ( !goodinlrg_.includes( bid.inl(),false ) )
	return false;

    if ( !goodcrlrg_.includes( bid.crl(),false ) )
	return false;

    return true;
}


void Seis::PosIndexer::add( const PosKey& pk, FileIdxType fileidx )
{
    if ( pk.isUndef() )
	{ nrrejected_++; return; }

    if ( crlsets_.isEmpty() )
    {
	const_cast<bool&>(is2d_) = is2D( pk.geomType() );
	const_cast<bool&>(isps_) = isPS( pk.geomType() );

	inlrg_.start = inlrg_.stop = is2d_ ? 1 : pk.inLine();
	crlrg_.start = crlrg_.stop = pk.xLine();
	offsrg_.start = offsrg_.stop = isps_ ? pk.offset() : 0;
    }

    if ( excludeunreasonable_ && !isReasonable( pk.binID() ) )
    {
	nrrejected_++;
	return;
    }

    maxfileidx_ = fileidx;
    if ( isps_ ) offsrg_.include( pk.offset() );

    bool ispresent = !inls_.isEmpty();
    idx_type inlidx = is2d_ ? 0 : getIndex( inls_, pk.inLine(), ispresent );
    if ( !ispresent )
    {
	if ( inlidx >= inls_.size() - 1 )
	{
	    inls_ += pk.inLine();
	    crlsets_ += new PosSet;
	    fileidxsets_ += new FileIdxSet;
	    inlidx = inls_.size() - 1;
	}
	else
	{
	    inlidx++;
	    inls_.insert( inlidx, pk.inLine() );
	    crlsets_.insertAt( new PosSet, inlidx );
	    fileidxsets_.insertAt( new FileIdxSet, inlidx );
	}

	if ( !is2d_ ) inlrg_.include( pk.inLine() );
    }

    PosSet& crls = *crlsets_[inlidx];
    FileIdxSet& fileidxs = *fileidxsets_[inlidx];
    idx_type crlidx = getIndex( crls, pk.xLine(), ispresent );
    if ( ispresent )
	return;

    crlrg_.include( pk.xLine() );

    if ( crlidx >= crls.size()-1 )
    {
	crls += pk.xLine();
	fileidxs += fileidx;
    }
    else
    {
	crlidx++;
	crls.insert( crlidx, pk.xLine() );
	fileidxs.insert( crlidx, fileidx );
    }
}
