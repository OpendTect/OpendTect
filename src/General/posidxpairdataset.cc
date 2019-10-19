/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Mar 2016
-*/


#include "posidxpairdataset.h"
#include "arrayndimpl.h"
#include "idxable.h"
#include "od_iostream.h"
#include "posinfo.h"
#include "horsubsel.h"
#include "statrand.h"
#include "trckeysampling.h"



#define mHandleMemFull() \
{ \
    setEmpty(); \
    ErrMsg( "Dataset emptied due to full memory." ); \
}
#define mErrRetMemFull() { mHandleMemFull(); return false; }

#define mEmitSPospErrMsg() { pErrMsg( "SPos invalid" ); }

static bool isnull = false;
static bool isnotnull = true;


Pos::IdxPairDataSet::ObjData::ObjData( const ObjData& oth )
    : objs_(oth.objs_)
    , bufsz_(0)
    , buf_(0)
{
    if ( oth.buf_ )
    {
	BufType* newbuf = new BufType[ oth.bufsz_ ];
	// may throw here, then caller should catch
	buf_ = newbuf;
	bufsz_ = oth.bufsz_;
	OD::memCopy( buf_, oth.buf_, bufsz_ );
    }
}


const void* Pos::IdxPairDataSet::ObjData::getObj( bool mandata, idx_type idx,
						    obj_size_type objsz ) const
{
    if ( !mandata )
	return objs_[idx];
    else if ( objsz == 0 )
	return 0;

    const bool* isnullptr = static_cast<const bool*>( objs_[idx] );
    return !isnullptr || !*isnullptr ? 0 : buf_ + objsz*idx;
}


bool Pos::IdxPairDataSet::ObjData::addObjSpace( bool mandata, idx_type idx,
					        obj_size_type objsz )
{
    const idx_type oldnrobjs = objs_.size();
    const bool atend = idx >= oldnrobjs;
    try {
	if ( atend )
	    objs_ += 0;
	else
	    objs_.insertAt( 0, idx );
    } catch ( std::bad_alloc )
	{ return false; }

    if ( !mandata || objsz < 1 )
	return true;

    if ( !manageBufCapacity(objsz) )
	return false;
    else if ( !atend )
    {
	BufType* ptrnewobjpos = buf_ + objsz * (idx+1);
	const BufType* ptroldobjpos = ptrnewobjpos - objsz;
	const BufType* ptrafterlast = buf_ + objsz * objs_.size();
	OD::memMove( ptrnewobjpos, ptroldobjpos, ptrafterlast-ptrnewobjpos );
    }
    return true;
}


void Pos::IdxPairDataSet::ObjData::putObj( bool mandata, idx_type idx,
					   obj_size_type objsz,
					   const void* obj )
{
    if ( !mandata )
	{ objs_.replace( idx, obj ); return; }
    else if ( objsz < 1 )
	return;

    objs_.replace( idx, obj ? &isnotnull : &isnull );
    if ( obj )
	OD::memCopy( buf_+idx*objsz, obj, objsz );
}


void Pos::IdxPairDataSet::ObjData::removeObj( bool mandata, idx_type idx,
					      obj_size_type objsz )
{
    if ( objsz < 1 || !objs_.validIdx(idx) )
	return;
    const int oldnrobjs = objs_.size();
    objs_.removeSingle( idx );
    if ( !mandata )
	return;

    if ( idx < oldnrobjs-1 )
    {
	BufType* ptrobj2rem = buf_ + objsz * idx;
	const BufType* ptrfirstmove = buf_ + objsz * (idx + 1);
	const BufType* ptrafterlastmove = buf_ + objsz * oldnrobjs;
	OD::memMove( ptrobj2rem, ptrfirstmove, ptrafterlastmove-ptrfirstmove );
    }

    manageBufCapacity( objsz );
}


bool Pos::IdxPairDataSet::ObjData::incrObjSize( obj_size_type orgsz,
					obj_size_type newsz, obj_size_type offs,
					const void* initbytes )
{
    if ( newsz < 1 )
	return true;

    BufType* orgbuf = buf_; const buf_size_type orgbufsz = bufsz_;
    buf_ = 0; bufsz_ = 0;
    if ( !manageBufCapacity(newsz) )
	{ buf_ = orgbuf; bufsz_ = orgbufsz; return false; }
    if ( !orgbuf )
	return true;

    if ( offs < 0 )
	{ offs = orgsz + offs + 1; if ( offs < 0 ) offs = 0; }
    if ( offs )
	OD::memCopy( buf_, orgbuf, offs );

    const obj_size_type gapsz = newsz - orgsz;
    obj_size_type offsorg = offs;
    obj_size_type offsnew = offs + gapsz;

    while ( offsorg+orgsz < orgbufsz )
    {
	obj_size_type nrbytes2copy = orgsz;
	if ( offsnew + nrbytes2copy > bufsz_ )
	    nrbytes2copy = bufsz_ - offsnew;
	if ( nrbytes2copy > 0 )
	    OD::memCopy( buf_+offsnew, orgbuf+offsorg, nrbytes2copy );

	if ( initbytes )
	    OD::memCopy( buf_+offsnew-gapsz, initbytes, gapsz );

	offsorg += orgsz; offsnew += newsz;
    }

    if ( orgbuf != buf_ )
	delete [] orgbuf;
    return true;
}


void Pos::IdxPairDataSet::ObjData::decrObjSize( obj_size_type orgsz,
				    obj_size_type newsz, obj_size_type offs )
{
    BufType* orgbuf = buf_; const buf_size_type orgbufsz = bufsz_;
    buf_ = 0; bufsz_ = 0;
    if ( !manageBufCapacity(newsz) )
	{ buf_ = orgbuf; bufsz_ = orgbufsz; }
    if ( !orgbuf )
	return;

    if ( offs < 0 )
	{ offs = newsz + offs + 1; if ( offs < 0 ) offs = 0; }
    if ( offs )
	OD::memCopy( buf_, orgbuf, offs );

    obj_size_type offsorg = offs + orgsz - newsz;
    obj_size_type offsnew = offs;

    while ( offsnew+newsz < bufsz_ )
    {
	obj_size_type nrbytes2copy = newsz;
	if ( offsorg + nrbytes2copy > orgbufsz )
	    nrbytes2copy = orgbufsz - offsorg;
	if ( nrbytes2copy > 0 )
	    OD::memCopy( buf_+offsnew, orgbuf+offsorg, nrbytes2copy );
	offsorg += orgsz; offsnew += newsz;
    }

    if ( orgbuf != buf_ )
	delete [] orgbuf;
}


bool Pos::IdxPairDataSet::ObjData::manageBufCapacity( obj_size_type objsz )
{
    if ( objsz < 1 )
	{ delete buf_; buf_ = 0; bufsz_ = 0; return true; }

    const size_type needednrobjs = objs_.size();
    size_type curnrobjs = (size_type)(bufsz_ / objsz);
    const bool needmore = needednrobjs > curnrobjs;
    if ( !needmore && needednrobjs > curnrobjs/2 )
	return true;

    size_type newnrobjs = curnrobjs;
    if ( needmore )
    {
	if ( newnrobjs == 0 )
	    newnrobjs = 1;
	while ( newnrobjs < needednrobjs )
	    newnrobjs *= 2;
    }
    else // need less
    {
	if ( newnrobjs == 1 )
	    newnrobjs = 0;
	else
	    while ( newnrobjs > needednrobjs )
		newnrobjs /= 2;
	if ( newnrobjs < needednrobjs )
	    newnrobjs *= 2;
    }

    const buf_size_type newsz = newnrobjs * objsz;
    BufType* orgbuf = buf_; const buf_size_type orgsz = bufsz_;
    bufsz_ = newsz;

    if ( bufsz_ < 1 )
	buf_ = 0;
    else
    {
	try {
	    buf_ = new BufType[ bufsz_ ];
	    if ( orgbuf )
		OD::memCopy( buf_, orgbuf, orgsz < bufsz_ ? orgsz : bufsz_ );
	} catch ( std::bad_alloc )
	    { return false; }
    }

    if ( bufsz_ > orgsz )
	OD::memZero( buf_+orgsz, bufsz_-orgsz );

    delete [] orgbuf;
    return true;
}




Pos::IdxPairDataSet::IdxPairDataSet( obj_size_type objsz, bool alwdup, bool md )
	: objsz_(objsz)
	, allowdup_(alwdup)
	, mandata_(md)
{
}


Pos::IdxPairDataSet::IdxPairDataSet( const Pos::IdxPairDataSet& oth )
	: objsz_(oth.objsz_)
	, mandata_(oth.mandata_)
{
    *this = oth;
}


Pos::IdxPairDataSet::~IdxPairDataSet()
{
    setEmpty();
}


Pos::IdxPairDataSet& Pos::IdxPairDataSet::operator =( const IdxPairDataSet& oth)
{
    if ( &oth == this ) return *this;

    setEmpty();
    copyStructureFrom( oth );

    try {
	for ( idx_type ifst=0; ifst<oth.frsts_.size(); ifst++ )
	{
	    frsts_ += oth.frsts_[ifst];
	    scndsets_ += new IdxSet( *oth.scndsets_[ifst] );
	    objdatas_ += new ObjData( *oth.objdatas_[ifst] );
	}
    } catch ( std::bad_alloc )
	mHandleMemFull()

    return *this;
}


void Pos::IdxPairDataSet::setEmpty()
{
    frsts_.setEmpty();
    deepErase( scndsets_ );
    deepErase( objdatas_ );
}


Pos::IdxPairDataSet::idx_type Pos::IdxPairDataSet::findIndexFor(
		const IdxSet& idxs, pos_type nr, bool* found )
{
    const size_type sz = idxs.size();
    idx_type ret = -1;
    const bool fnd = sz > 0 ? IdxAble::findPos( idxs.arr(), sz, nr, -1, ret )
			    : false;
    if ( found )
	*found = fnd;
    return ret;
}


bool Pos::IdxPairDataSet::setObjSize( obj_size_type newsz, obj_size_type offs,
				      const void* initbytes )
{
    if ( newsz == objsz_ )
	return true;

    if ( newsz < objsz_ )
	decrObjSize( objsz_-newsz, offs );
    else if ( !incrObjSize(newsz-objsz_,offs,initbytes) )
	return false;

    return true;
}


void Pos::IdxPairDataSet::decrObjSize( obj_size_type nrbytes,
					obj_size_type offs )
{
    if ( nrbytes == 0 )
	return;
    else if ( nrbytes < 0 )
	{ incrObjSize( -nrbytes, offs ); return; }
    else if ( mandata_ )
    {
	for ( idx_type ifst=0; ifst<objdatas_.size(); ifst++ )
	    objdatas_[ifst]->decrObjSize( objsz_, objsz_-nrbytes, offs );
    }

    const_cast<obj_size_type&>(objsz_) -= nrbytes;
}


bool Pos::IdxPairDataSet::incrObjSize( obj_size_type nrbytes,
					obj_size_type offs,
					const void* initbytes )
{
    if ( nrbytes == 0 )
	return true;
    else if ( nrbytes < 0 )
	{ decrObjSize( -nrbytes, offs ); return true; }
    else if ( mandata_ )
    {
	for ( idx_type ifst=0; ifst<objdatas_.size(); ifst++ )
	    if ( !objdatas_[ifst]->incrObjSize(objsz_,objsz_+nrbytes,offs,
					       initbytes) )
		mErrRetMemFull()
    }

    const_cast<obj_size_type&>(objsz_) += nrbytes;
    return true;
}


void Pos::IdxPairDataSet::allowDuplicateIdxPairs( bool yn )
{
    if ( !yn )
	removeDuplicateIdxPairs();
    allowdup_ = yn;
}


bool Pos::IdxPairDataSet::append( const IdxPairDataSet& oth )
{
    SPos spos;
    while ( oth.next(spos,!allowdup_) )
    {
	if ( !add( oth.gtIdxPair(spos), oth.gtObj(spos) ).isValid() )
	    mErrRetMemFull()
    }
    return true;
}


void Pos::IdxPairDataSet::remove( const IdxPairDataSet& oth )
{
    SPos othspos;
    while ( oth.next(othspos,true) )
    {
	const IdxPair pair = oth.gtIdxPair( othspos );
	SPos myspos = find( pair );

	while ( myspos.isValid() )
	{
	    remove( myspos );
	    myspos = find( pair );
	}
    }
}


Pos::IdxPairDataSet::size_type Pos::IdxPairDataSet::nrSecond(
						pos_type frst ) const
{
    const idx_type frstidx = frsts_.indexOf( frst );
    return frstidx<0 ? 0 : gtScndSet(frstidx).size();
}


Pos::IdxPairDataSet::size_type Pos::IdxPairDataSet::nrSecondAtIdx(
						idx_type frst ) const
{
    return frsts_.validIdx(frst) ? gtScndSet(frst).size() : 0;
}


Pos::IdxPairDataSet::idx_type Pos::IdxPairDataSet::firstIdx(
						pos_type frstpos ) const
{
    bool found = false;
    auto idx = findIndexFor( frsts_, frstpos, &found );
    return found ? idx : -1;
}


Pos::IdxPairDataSet::idx_type Pos::IdxPairDataSet::secondIdx( idx_type firstidx,
						pos_type scndpos ) const
{
    if ( !frsts_.validIdx(firstidx) )
	return -1;
    bool found = false;
    auto idx = findIndexFor( *scndsets_[firstidx], scndpos, &found );
    return found ? idx : -1;
}


Pos::IdxPairDataSet::pos_type Pos::IdxPairDataSet::firstAtIdx(
						idx_type frst ) const
{
    return frsts_.validIdx(frst) ? frsts_[frst] : mUdf(idx_type);
}


Pos::IdxPair Pos::IdxPairDataSet::positionAtIdxs( idx_type frst,
						  idx_type scnd ) const
{
    if ( frsts_.validIdx(frst) )
    {
	const IdxSet& scnds = gtScndSet( frst );
	if ( scnds.validIdx(scnd) )
	    return IdxPair( frsts_[frst], scnds[scnd] );
    }
    return IdxPair::udf();
}


void Pos::IdxPairDataSet::getSecondsAtIndex( idx_type frst,
					     TypeSet<pos_type>& seconds ) const
{
    seconds.setEmpty();
    if ( !frsts_.validIdx(frst) )
	return;
    const IdxSet& scnds = gtScndSet( frst );
    const auto sz = scnds.size();
    if ( sz < 1 )
	return;

    seconds.add( scnds.get(0) );
    for ( idx_type idx=1; idx<sz; idx++ )
    {
	const auto scnd = scnds.get( idx );
	if ( scnd != scnds.get(idx-1) )
	    seconds.add( scnd );
    }
}


Interval<Pos::IdxPairDataSet::pos_type> Pos::IdxPairDataSet::firstRange() const
{
    Interval<pos_type> ret( mUdf(pos_type), mUdf(pos_type) );

    for ( idx_type ifrst=0; ifrst<frsts_.size(); ifrst++ )
    {
	if ( ifrst == 0 )
	    ret.start = ret.stop = frsts_[0];
	else
	    ret.include( frsts_[ifrst], false );
    }
    return ret;
}


Interval<Pos::IdxPairDataSet::pos_type> Pos::IdxPairDataSet::secondRange(
							pos_type frst ) const
{
    Interval<pos_type> ret( mUdf(pos_type), mUdf(pos_type) );
    if ( frsts_.isEmpty() )
	return ret;

    const bool isall = frst < 0;
    const idx_type frstidx = isall ? -1 : frsts_.indexOf( frst );
    if ( frstidx >= 0 )
    {
	const IdxSet& scndset = gtScndSet( frstidx );
	const size_type nrscnd = scndset.size();
	if ( nrscnd > 0 )
	{
	    ret.start = ret.stop = scndset[0];
	    if ( nrscnd > 1 )
		ret.include( scndset[nrscnd-1], false );
	}
    }
    else if ( isall )
    {
	bool anyseenyet = false;
	for ( idx_type idx=0; idx<frsts_.size(); idx++ )
	{
	    const IdxSet& scndset = gtScndSet(idx);
	    const size_type nrscnd = scndset.size();
	    if ( nrscnd > 0 )
	    {
		if ( anyseenyet )
		    ret.include( scndset[0], false );
		else
		{
		    anyseenyet = true;
		    ret.start = ret.stop = scndset[0];
		}

		if ( nrscnd > 1 )
		    ret.include( scndset[nrscnd-1], false );
	    }
	}
    }

    return ret;
}


void Pos::IdxPairDataSet::copyStructureFrom( const IdxPairDataSet& oth )
{
    setEmpty();
    const_cast<obj_size_type&>(objsz_) = oth.objsz_;
    const_cast<bool&>(mandata_) = oth.mandata_;
    allowdup_ = oth.allowdup_;
}


bool Pos::IdxPairDataSet::isValid( SPos spos ) const
{
    if ( !spos.isValid() )
       return false;
    else if ( !frsts_.validIdx(spos.i()) )
	return false;

    return scndsets_[spos.i()]->validIdx( spos.j() );
}


bool Pos::IdxPairDataSet::isValid( const IdxPair& ip ) const
{
    return isValid( find(ip) );
}


Pos::IdxPairDataSet::SPos Pos::IdxPairDataSet::findOccurrence(
					const IdxPair& ip, int occ ) const
{
    bool found; idx_type idx = findIndexFor( frsts_, ip.first(), &found );
    SPos spos( found ? idx : -1, -1 );
    if ( !found )
	return spos;

    if ( spos.i() >= 0 )
    {
	const IdxSet& scnds = gtScndSet( spos );
	idx = findIndexFor( scnds, ip.second(), &found );
	spos.j() = found ? idx : -1;
	if ( found )
	{
	    spos.j() = idx;
	    while ( spos.j() && scnds[spos.j()-1] == ip.second() )
		spos.j()--;
	}
    }

    if ( found && occ )
    {
	while ( occ > 0 && next(spos) )
	    occ--;
    }

    return spos;
}


void Pos::IdxPairDataSet::updNearest( const IdxPair& ip, const SPos& spos,
		    od_int64& mindistsq, SPos& ret ) const
{
    const auto curip = getIdxPair( spos );
    const IdxPair ipdiff( curip.first()-ip.first(),
			  curip.second()-ip.second() );
    od_int64 distsq = ((od_int64)ipdiff.first()) * ipdiff.first();
    distsq += ((od_int64)ipdiff.second()) * ipdiff.second();
    if ( distsq < mindistsq )
	{ ret = spos; mindistsq = distsq; }
}


Pos::IdxPairDataSet::SPos Pos::IdxPairDataSet::findNearest(
					const IdxPair& ip ) const
{
    SPos ret;
    const auto nrfrst = frsts_.size();
    if ( nrfrst < 1 )
	return ret;

    od_int64 mindistsq = mUdf( od_int64 );


    for ( SPos spos(0); spos.i()<nrfrst; spos.i()++ )
    {
	const IdxSet& scnds = gtScndSet( spos );
	const auto lastj = scnds.size() - 1;
	bool found;
	spos.j() = findIndexFor( scnds, ip.second(), &found );
	if ( found )
	    updNearest( ip, spos, mindistsq, ret );
	else if ( spos.j() < 0 )
	    { spos.j() = 0; updNearest( ip, spos, mindistsq, ret ); }
	else if ( spos.j() > lastj )
	    { spos.j() = lastj; updNearest( ip, spos, mindistsq, ret ); }
	else
	{
	    updNearest( ip, spos, mindistsq, ret );
	    if ( spos.j() < lastj )
		{ spos.j()++; updNearest( ip, spos, mindistsq, ret ); }
	}
    }

    return ret;
}


Pos::IdxPairDataSet::SPos Pos::IdxPairDataSet::findNearestOnFirst(
					pos_type frst, pos_type scnd ) const
{
    const auto nrfrst = frsts_.size();
    if ( nrfrst < 1 )
	return SPos();

    bool found;
    SPos spos;
    spos.i() = findIndexFor( frsts_, frst, &found );
    if ( !found )
	return SPos();

    const IdxSet& scnds = gtScndSet( spos );
    const auto lastj = scnds.size() - 1;
    spos.j() = findIndexFor( scnds, scnd, &found );
    if ( found )
	return spos;

    if ( spos.j() < 0 )
	spos.j() = 0;
    else if ( spos.j() > lastj )
	spos.j() = scnds.size() - 1;
    else if ( spos.j() < lastj )
    {
	auto ip = getIdxPair( spos );
	const auto posdiff1 = std::abs( ip.second() - scnd );
	SPos spos2( spos.i(), spos.j()+1 );
	ip = getIdxPair( spos2 );
	const auto posdiff2 = std::abs( ip.second() - scnd );
	if ( posdiff2 < posdiff1 )
	    spos = spos2;
    }

    return spos;
}


bool Pos::IdxPairDataSet::next( SPos& spos, bool skip_dup ) const
{
    if ( spos.i() < 0 )
    {
	if ( frsts_.size() < 1 )
	    return false;
	spos.i() = spos.j() = 0;
	return true;
    }
    else if ( spos.i() >= frsts_.size() )
	{ spos.i() = spos.j() = -1; return false; }
    else if ( spos.j() < 0 )
	{ spos.j() = 0; return true; }

    const IdxSet& scnds = gtScndSet( spos );
    if ( spos.j() > scnds.size()-2 )
    {
	spos.j() = 0; spos.i()++;
	if ( spos.i() >= frsts_.size() )
	    spos.i() = spos.j() = -1;
	return spos.i() >= 0;
    }

    spos.j()++;
    if ( skip_dup && scnds[spos.j()] == scnds[spos.j()-1] )
	return next( spos, true );

    return true;
}


bool Pos::IdxPairDataSet::prev( SPos& spos, bool skip_dup ) const
{
    if ( spos.j() < 0 )
    {
	spos.i()--;
	if ( spos.i() >= 0 )
	    spos.j() = gtScndSet(spos).size() - 1;
    }
    if ( spos.i() < 0 || spos.j() < 0 )
	return false;
    else if ( spos.i() == 0 && spos.j() == 0 )
	{ spos.i() = spos.j() = -1; return false; }

    pos_type curscnd = gtScnd( spos );
    if ( spos.j() > 0 )
	spos.j()--;
    else
	{ spos.i()--; spos.j() = gtScndSet(spos).size() - 1; }

    if ( !skip_dup )
	return true;

    while ( gtScnd(spos) == curscnd )
	return prev( spos, true );

    return true;
}


const void* Pos::IdxPairDataSet::get( SPos spos, IdxPair& ip ) const
{
    if ( spos.isValid() )
    {
	if ( !isValid(spos) )
	    mEmitSPospErrMsg()
	else
	{
	    ip = gtIdxPair( spos );
	    return gtObj( spos );
	}
    }

    ip.setUdf();
    return 0;
}


Pos::IdxPair Pos::IdxPairDataSet::getIdxPair( SPos spos ) const
{
    if ( spos.isValid() )
    {
	if ( !isValid(spos) )
	    mEmitSPospErrMsg()
	else
	    return gtIdxPair( spos );
    }
    return IdxPair::udf();
}


const void* Pos::IdxPairDataSet::getObj( SPos spos ) const
{
    if ( spos.isValid() )
    {
	if ( isValid(spos) )
	    return gtObj( spos );
	mEmitSPospErrMsg()
    }
    return 0;
}


Pos::IdxPairDataSet::SPos Pos::IdxPairDataSet::getPos(
					glob_idx_type glidx ) const
{
    glob_idx_type firstidx = 0; SPos spos;
    for ( spos.i()=0; spos.i()<frsts_.size(); spos.i()++ )
    {
	const IdxSet& scnds = gtScndSet(spos);
	if ( firstidx + scnds.size() > glidx )
	{
	    spos.j() = (idx_type)(glidx - firstidx);
	    return spos;
	}
	firstidx += scnds.size();
    }

    return SPos(-1,-1);
}


Pos::IdxPairDataSet::SPos Pos::IdxPairDataSet::add( const Pos::IdxPair& ip,
							const void* obj )
{
    SPos spos( findFirst(ip) );
    addEntry( ip, obj, spos );
    return spos;
}


void Pos::IdxPairDataSet::set( SPos spos, const void* obj )
{
    if ( spos.isValid() )
    {
	if ( !isValid(spos) )
	    mEmitSPospErrMsg()
	else
	    putObj( spos, obj );
    }
}


Pos::IdxPairDataSet::SPos Pos::IdxPairDataSet::update( const IdxPair& ip,
							const void* obj )
{
    SPos spos = findFirst( ip );
    if ( !spos.isValid() )
	addEntry( ip, obj, spos );
    else
	set( spos, obj );
    return spos;
}


Pos::IdxPairDataSet::size_type Pos::IdxPairDataSet::nrPos(
						idx_type frstidx ) const
{
    return frstidx < 0 || frstidx >= frsts_.size() ? 0
		: gtScndSet(frstidx).size();
}


Pos::IdxPairDataSet::glob_idx_type Pos::IdxPairDataSet::totalSize() const
{
    glob_idx_type nr = 0;
    for ( idx_type idx=0; idx<frsts_.size(); idx++ )
	nr += gtScndSet(idx).size();
    return nr;
}


bool Pos::IdxPairDataSet::hasFirst( pos_type frst ) const
{
    bool found = false;
    findIndexFor( frsts_, frst, &found );
    return found;
}


bool Pos::IdxPairDataSet::hasSecond( pos_type scnd ) const
{
    for ( idx_type ifrst=0; ifrst<frsts_.size(); ifrst++ )
    {
	const IdxSet& scnds = gtScndSet( ifrst );
	bool found = false;
	findIndexFor( scnds, scnd, &found );
	if ( found )
	    return true;
    }
    return false;
}


Pos::IdxPair Pos::IdxPairDataSet::firstIdxPair() const
{
    return frsts_.isEmpty() ? IdxPair::udf()
			    : IdxPair( frsts_[0], gtScndSet(0)[0] );
}


void Pos::IdxPairDataSet::remove( SPos spos )
{
    if ( spos.i() < 0 || spos.i() >= frsts_.size() )
	return;
    IdxSet& scnds = gtScndSet( spos );
    if ( spos.j() < 0 || spos.j() >= scnds.size() )
	return;

    scnds.removeSingle( spos.j() );
    gtObjData(spos).removeObj( mandata_, spos.j(), objsz_ );

    if ( scnds.isEmpty() )
    {
	frsts_.removeSingle( spos.i() );
	delete scndsets_.removeSingle( spos.i() );
	delete objdatas_.removeSingle( spos.i() );
    }
}


void Pos::IdxPairDataSet::remove( const TypeSet<SPos>& sposs )
{
    if ( sposs.size() < 1 )
	return;
    else if ( sposs.size() == 1 )
	{ remove( sposs[0] ); return; }

    IdxPairDataSet ds( *this );
    setEmpty();
    SPos spos;
    while ( ds.next(spos) )
	if ( !sposs.isPresent(spos) )
	    add( ds.gtIdxPair(spos), ds.getObj(spos) );
}


void Pos::IdxPairDataSet::removeDuplicateIdxPairs()
{
    if ( isEmpty() )
	return;

    SPos spos; next( spos, false );
    IdxPair previp = gtIdxPair( spos );
    TypeSet<SPos> sposs;
    while ( next(spos,false) )
    {
	IdxPair curip = gtIdxPair( spos );
	if ( previp == curip )
	    sposs += spos;
	else
	    previp = curip;
    }

    remove( sposs );
}


void Pos::IdxPairDataSet::addHorPosIfNeeded( const IdxPair& ip,
					     EntryCreatedFn crfn )
{
    SPos spos = find( ip );
    if ( !spos.isValid() )
    {
	spos = add( ip, getObj(SPos(0,0)) ); // just use an existing obj
	if ( !spos.isValid() )
	    { mHandleMemFull(); return; }
	else if ( crfn )
	    crfn( *this, spos.i(), spos.j() );
    }
}


void Pos::IdxPairDataSet::extendHor3D( const Pos::IdxPairDelta& so,
				       const Pos::IdxPairStep& sostep,
				       EntryCreatedFn crfn )
{
    if ( isEmpty()
      || (!so.first() && !so.second())
      || (!sostep.first() && !sostep.second()) )
	return;

    // this implementation is rather complex to make sure big sets will not
    // get bizarre performance. To counter large stepouts and/or large sets,
    // we'll keep track of where data is needed in an Array2D
    // Then afterwards, we'll add a single position there

    auto inlrg = inlRange();
    inlrg.widen( so.inl() * sostep.inl() );
    auto crlrg = crlRange();
    crlrg.widen( so.crl() * sostep.crl() );
    const IdxSubSel2D::pos_steprg_type inldef( inlrg, sostep.inl() );
    const IdxSubSel2D::pos_steprg_type crldef( crlrg, sostep.crl() );
    const CubeHorSubSel subsel( inldef, crldef );
    Array2DImpl<bool> needed( subsel.size(0), subsel.size(1) );
    needed.setAll( false );

    SPos spos;
    while ( next(spos) )
    {
	const auto centralip = gtIdxPair( spos );
	for ( pos_type ifrstoffs=-so.first(); ifrstoffs<=so.first();
		    ifrstoffs++ )
	{
	    const auto first = centralip.first() + ifrstoffs * sostep.first();
	    for ( int iscndoffs=-so.second(); iscndoffs<=so.second();
		    iscndoffs++ )
	    {
		const auto scnd = centralip.second()
				+ iscndoffs * sostep.second();
		const auto rc = subsel.rowCol( first, scnd );
		needed.set( rc.row(), rc.col(), true );
	    }
	}
    }

    ArrRegSubSel2DIterator it( subsel );
    while ( it.next() )
	if ( needed.get(it.idx0_,it.idx1_) )
	    addHorPosIfNeeded( subsel.binID(it.idx0_,it.idx1_), crfn );
}


void Pos::IdxPairDataSet::extendHor2D( pos_type so, EntryCreatedFn crfn )
{
    if ( isEmpty() || so < 1 )
	return;

    for ( auto ifirst=0; ifirst<frsts_.size(); ifirst++ )
    {
	const auto frst = frsts_[ifirst];
	const GeomID geomid( frst );
	const ::LineHorSubSel subsel( geomid );
	if ( subsel.isEmpty() )
	    continue;

	Array1DImpl<bool> needed( subsel.size() );
	needed.setAll( false );

	const auto sostep = subsel.trcNrRange().step;
	const auto& scnds = gtScndSet( ifirst );
	for ( auto iscnd=0; iscnd<scnds.size(); iscnd++ )
	{
	    const auto centralnr = scnds[iscnd];
	    for ( int ioffs=-so; ioffs<=so; ioffs++ )
	    {
		const auto arridx = subsel.idx4Pos( centralnr + ioffs*sostep );
		if ( needed.validIdx(arridx) )
		    needed.set( arridx, true );
	    }
	}

	const auto arrsz = needed.size();
	for ( auto idx=0; idx<arrsz; idx++ )
	{
	    if ( needed.get(idx) )
	    {
		const IdxPair ip( frst, subsel.pos4Idx(idx) );
		addHorPosIfNeeded( ip, crfn );
	    }
	}
    }

}


namespace Pos
{
class IdxPairDataSetFromLineCollData : public ::ParallelTask
{
public:

typedef Pos::IdxPairDataSet::pos_type pos_type;
typedef Pos::IdxPairDataSet::idx_type idx_type;
mUseType( PosInfo, LineCollData );

IdxPairDataSetFromLineCollData( IdxPairDataSet& ds,
			    const LineCollData& lcd,
			    EntryCreatedFn crfn )
    : ds_( ds )
    , lcd_( lcd )
    , crfn_( crfn )
{
    // Add first spos on each line so all lines are in, thus
    // threadsafe to add things as long as each line is separate
    for ( idx_type idx=0; idx<lcd.size(); idx++ )
    {
	const auto& ld = *lcd_[idx];
	const idx_type frst = ld.linenr_;
	if ( !ld.segments_.isEmpty() )
	    ds.add( IdxPair(frst,ld.segments_[0].start) );
    }
}

od_int64 nrIterations() const { return lcd_.size(); }

bool doWork( od_int64 start, od_int64 stop, int )
{
    for ( IdxPair::pos_type idx=(IdxPair::pos_type)start; idx<=stop; idx++ )
    {
	const auto& ld = *lcd_[idx];
	const pos_type frst = ld.linenr_;
	for ( int idy=0; idy<ld.segments_.size(); idy++ )
	{
	    StepInterval<pos_type> crls = ld.segments_[idy];
	    if ( idy == 0 )
		crls.start += crls.step; //We added first scnd in constructor
	    for ( pos_type scnd=crls.start; scnd<=crls.stop; scnd+=crls.step )
	    {
		IdxPair ip( frst, scnd );
		IdxPairDataSet::SPos spos = ds_.find( ip );
		if ( spos.isValid() )
		    continue;
		spos = ds_.add( ip );
		if ( !spos.isValid() )
		    return false;
		else if ( crfn_ )
		    crfn_( ds_, spos.i(), spos.j() );
	    }
	}
    }

    return true;
}

    IdxPairDataSet&	ds_;
    const LineCollData&	lcd_;
    EntryCreatedFn	crfn_;

};

} // namespace Pos


void Pos::IdxPairDataSet::add( const PosInfo::LineCollData& lcd,
			       EntryCreatedFn crfn )
{
    Pos::IdxPairDataSetFromLineCollData task( *this, lcd, crfn );
    if ( !task.execute() )
	mHandleMemFull()
}


void Pos::IdxPairDataSet::randomSubselect( glob_idx_type maxsz )
{
    const glob_idx_type orgsz = totalSize();
    if ( orgsz <= maxsz )
	return;
    if ( maxsz < 1 )
	{ setEmpty(); return; }

    mGetIdxArr( glob_idx_type, idxs, orgsz );
    if ( !idxs )
	{ setEmpty(); return; }

    const bool buildnew = ((glob_idx_type)maxsz) < (orgsz / ((glob_idx_type)2));
    Stats::randGen().subselect( idxs, orgsz, maxsz );
    TypeSet<SPos> sposs;
    if ( buildnew )
    {
	for ( glob_idx_type idx=0; idx<maxsz; idx++ )
	    sposs += getPos( idxs[idx] );
    }
    else
    {
	for ( glob_idx_type idx=maxsz; idx<orgsz; idx++ )
	    sposs += getPos( idxs[idx] );
    }
    delete [] idxs;

    if ( !buildnew )
	remove( sposs );
    else
    {
	IdxPairDataSet newds( objsz_, allowdup_, mandata_ );
	IdxPair ip;
	for ( glob_idx_type idx=0; idx<sposs.size(); idx++ )
	{
	    const void* data = get( sposs[mCast(int,idx)], ip );
	    SPos newspos = newds.add( ip, data );
	    if ( !newspos.isValid() )
		{ mHandleMemFull() return; }
	}
	*this = newds;
    }
}



void Pos::IdxPairDataSet::remove( const TrcKeySampling& hrg,
				  bool inside )
{
    const StepInterval<pos_type> frstrg = hrg.inlRange();
    const StepInterval<pos_type> scndrg = hrg.crlRange();

    TypeSet<SPos> torem;

    SPos spos;
    while ( next(spos) )
    {
	IdxPair ip = gtIdxPair( spos );
	const bool inlinside = frstrg.includes( ip.first(), false )
			    && frstrg.snap( ip.first() ) == ip.first();
	const bool crlinside = scndrg.includes( ip.second(), false )
			    && scndrg.snap( ip.second() ) == ip.second();
	if ( (inside && inlinside && crlinside)
	  || (!inside && (!inlinside || !crlinside)) )
	    torem += spos;
    }

    remove( torem );
}


bool Pos::IdxPairDataSet::hasDuplicateIdxPairs() const
{
    IdxPair previp = IdxPair::udf();
    SPos spos;
    while ( next(spos) )
    {
	IdxPair ip = gtIdxPair( spos );
	if ( previp == ip )
	    return true;
	previp = ip;
    }

    return false;
}


Pos::IdxPairDataSet::size_type Pos::IdxPairDataSet::nrDuplicateIdxPairs() const
{
    size_type nrdupips = 0;
    SPos spos;
    if ( !next(spos) )
	return 0;

    IdxPair previp = gtIdxPair( spos );
    while ( next(spos) )
    {
	IdxPair ip = gtIdxPair( spos );
	if ( previp == ip )
	{
	    nrdupips++;
	    while ( next(spos) )
	    {
		previp = ip;
		ip = gtIdxPair( spos );
		if ( previp != ip )
		    break;
	    }
	}

	if ( !spos.isValid() )
	    break;

	previp = ip;
    }

    return nrdupips;
}


bool Pos::IdxPairDataSet::dump( od_ostream& strm, bool binary ) const
{
    //TODO
    return false;
}


bool Pos::IdxPairDataSet::slurp( od_istream& strm, bool binary )
{
    //TODO
    return false;
}


const void* Pos::IdxPairDataSet::gtObj( const SPos& spos ) const
{
    return gtObjData(spos).getObj( mandata_, spos.j(), objsz_ );
}


void Pos::IdxPairDataSet::putObj( const SPos& spos, const void* obj )
{
    if ( objdatas_.size() <= spos.i() )
    {
	pErrMsg("putObj shuld not have to alloc its own objdata");
	try { objdatas_ += new ObjData; }
	catch ( std::bad_alloc )
	    { mHandleMemFull(); return; }
    }

    gtObjData(spos).putObj( mandata_, spos.j(), objsz_, obj );
}


bool Pos::IdxPairDataSet::addObj( SPos& spos, pos_type scnd, const void* obj )
{
    IdxSet& scnds = gtScndSet( spos );

    if ( spos.j() < 0 )
	spos.j() = findIndexFor(scnds,scnd) + 1;
    else
    {
	spos.j()++;
	while ( spos.j() < scnds.size() && scnds[spos.j()] == scnd )
	    spos.j()++;
    }

    const bool atend = spos.j() > scnds.size() - 1;
    try {
	if ( atend )
	    scnds += scnd;
	else
	    scnds.insert( spos.j(), scnd );
    } catch ( std::bad_alloc )
	mErrRetMemFull()

    if ( !gtObjData(spos).addObjSpace(mandata_,spos.j(),objsz_) )
	mErrRetMemFull()

    putObj( spos, obj );
    return true;
}


void Pos::IdxPairDataSet::addEntry( const Pos::IdxPair& ip, const void* obj,
				    SPos& spos )
{
    if ( spos.i() < 0 )
    {
	spos.i() = findIndexFor(frsts_,ip.first()) + 1;
	try {
	    ObjData* newdata = new ObjData;
	    if ( spos.i() > frsts_.size()-1 )
	    {
		frsts_ += ip.first();
		scndsets_ += new IdxSet;
		objdatas_ += newdata;
		spos.i() = frsts_.size() - 1;
	    }
	    else
	    {
		frsts_.insert( spos.i(), ip.first() );
		scndsets_.insertAt( new IdxSet, spos.i() );
		objdatas_.insertAt( newdata, spos.i() );
	    }
	} catch ( std::bad_alloc )
	    { mHandleMemFull(); spos.reset(); return; }
    }

    if ( (spos.j() < 0 || allowdup_) && !addObj( spos, ip.second(), obj ) )
	spos.reset();
}
