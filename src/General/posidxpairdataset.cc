/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "posidxpairdataset.h"
#include "idxable.h"
#include "posinfo.h"
#include "statrand.h"
#include "od_iostream.h"



#define mHandleMemFull() \
{ \
    setEmpty(); \
    ErrMsg( "Dataset emptied due to full memory." ); \
}
#define mErrRetMemFull() { mHandleMemFull(); return false; }

static bool isnull = false;
static bool isnotnull = true;


Pos::IdxPairDataSet::ObjData::ObjData( const ObjData& oth )
    : objs_(oth.objs_)
    , bufsz_(oth.bufsz_)
    , buf_(0)
{
    if ( oth.buf_ )
    {
	buf_ = new BufType[ bufsz_ ];
	OD::memCopy( buf_, oth.buf_, bufsz_ );
    }
}


const void* Pos::IdxPairDataSet::ObjData::getObj( bool mandata, ArrIdxType idx,
						    ObjSzType objsz ) const
{
    if ( !mandata )
	return objs_[idx];
    else if ( objsz == 0 )
	return 0;

    const bool* isnullptr = static_cast<const bool*>( objs_[idx] );
    return !isnullptr || !*isnullptr ? 0 : buf_ + objsz*idx;
}


bool Pos::IdxPairDataSet::ObjData::addObjSpace( bool mandata, ArrIdxType idx,
					        ObjSzType objsz )
{
    const ArrIdxType oldnrobjs = objs_.size();
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


void Pos::IdxPairDataSet::ObjData::putObj( bool mandata, ArrIdxType idx,
					   ObjSzType objsz, const void* obj )
{
    if ( !mandata )
	{ objs_.replace( idx, obj ); return; }
    else if ( objsz < 1 )
	return;

    objs_.replace( idx, obj ? &isnotnull : &isnull );
    if ( obj )
	OD::memCopy( buf_+idx*objsz, obj, objsz );
}


void Pos::IdxPairDataSet::ObjData::removeObj( bool mandata, ArrIdxType idx,
					      ObjSzType objsz )
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


bool Pos::IdxPairDataSet::ObjData::incrObjSize( ObjSzType orgsz,
					ObjSzType newsz, ObjSzType offs,
					const void* initbytes )
{
    if ( newsz < 1 )
	return true;

    BufType* orgbuf = buf_; const BufSzType orgbufsz = bufsz_;
    buf_ = 0; bufsz_ = 0;
    if ( !manageBufCapacity(newsz) )
	{ buf_ = orgbuf; bufsz_ = orgbufsz; return false; }
    if ( !orgbuf )
	return true;

    if ( offs < 0 )
	{ offs = orgsz + offs + 1; if ( offs < 0 ) offs = 0; }
    if ( offs )
	OD::memCopy( buf_, orgbuf, offs );

    const ObjSzType gapsz = newsz - orgsz;
    ObjSzType offsorg = offs;
    ObjSzType offsnew = offs + gapsz;

    while ( offsnew < bufsz_+gapsz )
    {
	ObjSzType nrbytes2copy = newsz;
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


void Pos::IdxPairDataSet::ObjData::decrObjSize( ObjSzType orgsz,
					ObjSzType newsz, ObjSzType offs )
{
    BufType* orgbuf = buf_; const BufSzType orgbufsz = bufsz_;
    buf_ = 0; bufsz_ = 0;
    if ( !manageBufCapacity(newsz) )
	{ buf_ = orgbuf; bufsz_ = orgbufsz; }
    if ( !orgbuf )
	return;

    if ( offs < 0 )
	{ offs = newsz + offs + 1; if ( offs < 0 ) offs = 0; }
    if ( offs )
	OD::memCopy( buf_, orgbuf, offs );

    const ObjSzType gapsz = orgsz - newsz;
    ObjSzType offsorg = offs + gapsz;
    ObjSzType offsnew = offs;

    while ( offsorg < orgbufsz )
    {
	ObjSzType nrbytes2copy = orgsz;
	if ( offsorg + nrbytes2copy > orgbufsz )
	    nrbytes2copy = orgbufsz - offsorg;
	if ( nrbytes2copy > 0 )
	    OD::memCopy( buf_+offsnew, orgbuf+offsorg, nrbytes2copy );
	offsorg += orgsz; offsnew += newsz;
    }

    if ( orgbuf != buf_ )
	delete [] orgbuf;
}


bool Pos::IdxPairDataSet::ObjData::manageBufCapacity( ObjSzType objsz )
{
    if ( objsz < 1 )
	{ delete buf_; buf_ = 0; bufsz_ = 0; return true; }

    const ArrIdxType needednrobjs = objs_.size();
    ArrIdxType curnrobjs = (ArrIdxType)(bufsz_ / objsz);
    const bool needmore = needednrobjs > curnrobjs;
    if ( !needmore && needednrobjs > curnrobjs/2 )
	return true;

    ArrIdxType newnrobjs = curnrobjs;
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

    const BufSzType newsz = newnrobjs * objsz;
    BufType* orgbuf = buf_; const BufSzType orgsz = bufsz_;
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




Pos::IdxPairDataSet::IdxPairDataSet( ObjSzType objsz, bool alwdup, bool md )
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
	for ( IdxType ifst=0; ifst<oth.frsts_.size(); ifst++ )
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


Pos::IdxPairDataSet::ArrIdxType Pos::IdxPairDataSet::findIndexFor(
		const IdxSet& idxs, IdxType nr, bool* found )
{
    const ArrIdxType sz = idxs.size();
    ArrIdxType ret = -1;
    const bool fnd = sz > 0 ? IdxAble::findPos( idxs.arr(), sz, nr, -1, ret )
			    : false;
    if ( found )
	*found = fnd;
    return ret;
}


bool Pos::IdxPairDataSet::setObjSize( ObjSzType newsz, ObjSzType offs,
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


void Pos::IdxPairDataSet::decrObjSize( ObjSzType nrbytes, ObjSzType offs )
{
    if ( nrbytes == 0 )
	return;
    else if ( nrbytes < 0 )
	{ incrObjSize( -nrbytes, offs ); return; }
    else if ( mandata_ )
    {
	for ( IdxType ifst=0; ifst<objdatas_.size(); ifst++ )
	    objdatas_[ifst]->decrObjSize( objsz_, objsz_-nrbytes, offs );
    }

    const_cast<ObjSzType&>(objsz_) -= nrbytes;
}


bool Pos::IdxPairDataSet::incrObjSize( ObjSzType nrbytes, ObjSzType offs,
					const void* initbytes )
{
    if ( nrbytes == 0 )
	return true;
    else if ( nrbytes < 0 )
	{ decrObjSize( -nrbytes, offs ); return true; }
    else if ( mandata_ )
    {
	for ( IdxType ifst=0; ifst<objdatas_.size(); ifst++ )
	    if ( !objdatas_[ifst]->incrObjSize(objsz_,objsz_+nrbytes,offs,
					       initbytes) )
		mErrRetMemFull()
    }

    const_cast<ObjSzType&>(objsz_) += nrbytes;
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


Pos::IdxPairDataSet::ArrIdxType Pos::IdxPairDataSet::nrSecond(
						    IdxType frst ) const
{
    const IdxType frstidx = frsts_.indexOf( frst );
    return frstidx<0 ? 0 : gtScndSet(frstidx).size();
}


Interval<Pos::IdxPairDataSet::IdxType> Pos::IdxPairDataSet::firstRange() const
{
    Interval<IdxType> ret( mUdf(IdxType), mUdf(IdxType) );

    for ( IdxType ifrst=0; ifrst<frsts_.size(); ifrst++ )
    {
	if ( ifrst == 0 )
	    ret.start = ret.stop = frsts_[0];
	else
	    ret.include( frsts_[ifrst], false );
    }
    return ret;
}


Interval<Pos::IdxPairDataSet::IdxType> Pos::IdxPairDataSet::secondRange(
							IdxType frst ) const
{
    Interval<IdxType> ret( mUdf(IdxType), mUdf(IdxType) );
    if ( frsts_.isEmpty() )
	return ret;

    const bool isall = frst < 0;
    const ArrIdxType frstidx = isall ? -1 : frsts_.indexOf( frst );
    if ( frstidx >= 0 )
    {
	const IdxSet& scndset = gtScndSet( frstidx );
	const ArrIdxType nrscnd = scndset.size();
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
	for ( ArrIdxType idx=0; idx<frsts_.size(); idx++ )
	{
	    const IdxSet& scndset = gtScndSet(idx);
	    const ArrIdxType nrscnd = scndset.size();
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
    const_cast<ObjSzType&>(objsz_) = oth.objsz_;
    const_cast<bool&>(mandata_) = oth.mandata_;
    allowdup_ = oth.allowdup_;
}


bool Pos::IdxPairDataSet::isValid( const IdxPair& ip ) const
{
    return find( ip ).isValid();
}


Pos::IdxPairDataSet::SPos Pos::IdxPairDataSet::findOccurrence(
					const IdxPair& ip, int occ ) const
{
    bool found; ArrIdxType idx = findIndexFor( frsts_, ip.first, &found );
    SPos spos( found ? idx : -1, -1 );
    if ( !found )
	return spos;

    if ( spos.i >= 0 )
    {
	const IdxSet& scnds = gtScndSet( spos );
	idx = findIndexFor( scnds, ip.second, &found );
	spos.j = found ? idx : -1;
	if ( found )
	{
	    spos.j = idx;
	    while ( spos.j && scnds[spos.j-1] == ip.second )
		spos.j--;
	}
    }

    if ( found && occ )
    {
	while ( occ > 0 && next(spos) )
	    occ--;
    }

    return spos;
}


bool Pos::IdxPairDataSet::next( SPos& spos, bool skip_dup ) const
{
    if ( spos.i < 0 )
    {
	if ( frsts_.size() < 1 )
	    return false;
	spos.i = spos.j = 0;
	return true;
    }
    else if ( spos.i >= frsts_.size() )
	{ spos.i = spos.j = -1; return false; }
    else if ( spos.j < 0 )
	{ spos.j = 0; return true; }

    const IdxSet& scnds = gtScndSet( spos );
    if ( spos.j > scnds.size()-2 )
    {
	spos.j = 0; spos.i++;
	if ( spos.i >= frsts_.size() )
	    spos.i = spos.j = -1;
	return spos.i >= 0;
    }

    spos.j++;
    if ( skip_dup && scnds[spos.j] == scnds[spos.j-1] )
	return next( spos, true );

    return true;
}


bool Pos::IdxPairDataSet::prev( SPos& spos, bool skip_dup ) const
{
    if ( spos.j < 0 )
    {
	spos.i--;
	if ( spos.i >= 0 )
	    spos.j = gtScndSet(spos).size() - 1;
    }
    if ( spos.i < 0 || spos.j < 0 )
	return false;
    else if ( spos.i == 0 && spos.j == 0 )
	{ spos.i = spos.j = -1; return false; }

    IdxType curscnd = gtScnd( spos );
    if ( spos.j > 0 )
	spos.j--;
    else
	{ spos.i--; spos.j = gtScndSet(spos).size() - 1; }

    if ( !skip_dup )
	return true;

    while ( gtScnd(spos) == curscnd )
	return prev( spos, true );

    return true;
}


const void* Pos::IdxPairDataSet::get( SPos spos, IdxPair& ip ) const
{
    if ( !spos.isValid() )
	{ ip.setUdf(); return 0; }
    ip = gtIdxPair( spos );
    return gtObj( spos );
}


Pos::IdxPair Pos::IdxPairDataSet::getIdxPair( SPos spos ) const
{
    return spos.isValid() ? gtIdxPair(spos) : IdxPair::udf();
}


const void* Pos::IdxPairDataSet::getObj( SPos spos ) const
{
    return spos.isValid() ? gtObj(spos) : 0;
}


Pos::IdxPairDataSet::SPos Pos::IdxPairDataSet::getPos( GlobIdxType glidx ) const
{
    GlobIdxType firstidx = 0; SPos spos;
    for ( spos.i=0; spos.i<frsts_.size(); spos.i++ )
    {
	const IdxSet& scnds = gtScndSet(spos);
	if ( firstidx + scnds.size() > glidx )
	{
	    spos.j = (ArrIdxType)(glidx - firstidx);
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
	putObj( spos, obj );
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


Pos::IdxPairDataSet::ArrIdxType Pos::IdxPairDataSet::nrPos(
						ArrIdxType frstidx ) const
{
    return frstidx < 0 || frstidx >= frsts_.size() ? 0
		: gtScndSet(frstidx).size();
}


Pos::IdxPairDataSet::GlobIdxType Pos::IdxPairDataSet::totalSize() const
{
    GlobIdxType nr = 0;
    for ( ArrIdxType idx=0; idx<frsts_.size(); idx++ )
	nr += gtScndSet(idx).size();
    return nr;
}


bool Pos::IdxPairDataSet::hasFirst( IdxType frst ) const
{
    return frsts_.isPresent( frst );
}


bool Pos::IdxPairDataSet::hasSecond( IdxType scnd ) const
{
    for ( ArrIdxType ifrst=0; ifrst<frsts_.size(); ifrst++ )
    {
	const IdxSet& scnds = gtScndSet( ifrst );
	for ( ArrIdxType iscnd=0; iscnd<scnds.size(); iscnd++ )
	    if ( scnds[iscnd] == scnd )
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
    if ( spos.i < 0 || spos.i >= frsts_.size() )
	return;
    IdxSet& scnds = gtScndSet( spos );
    if ( spos.j < 0 || spos.j >= scnds.size() )
	return;

    scnds.removeSingle( spos.j );
    gtObjData(spos).removeObj( mandata_, spos.j, objsz_ );

    if ( scnds.isEmpty() )
    {
	frsts_.removeSingle( spos.i );
	delete scndsets_.removeSingle( spos.i );
	delete objdatas_.removeSingle( spos.i );
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


void Pos::IdxPairDataSet::extend( const Pos::IdxPairDelta& so,
				  const Pos::IdxPairStep& sostep,
				  EntryCreatedFn crfn )
{
    if ( (!so.first && !so.second) || (!sostep.first && !sostep.second) )
	return;

    IdxPairDataSet ds( *this );

    const bool kpdup = allowdup_;
    allowdup_ = false;

    SPos spos;
    while ( ds.next(spos) )
    {
	IdxPair ip = ds.gtIdxPair( spos );
	const IdxPair centralip( ip );
	for ( int ifrstoffs=-so.first; ifrstoffs<=so.first; ifrstoffs++ )
	{
	    ip.first = centralip.first + ifrstoffs * sostep.first;
	    for ( int iscndoffs=-so.second; iscndoffs<=so.second; iscndoffs++ )
	    {
		if ( !ifrstoffs && !iscndoffs )
		    continue;

		ip.second = centralip.second + iscndoffs * sostep.second;
		SPos newspos = ds.find( ip );
		if ( newspos.isValid() )
		    continue;
		newspos = ds.add( ip );
		if ( !newspos.isValid() )
		    { mHandleMemFull(); return; }

		if ( crfn )
		    crfn( *this, newspos.i, newspos.j );
	    }
	}
    }

    allowdup_ = kpdup;
}


namespace Pos
{
class IdxPairDataSetFromCubeData : public ::ParallelTask
{
public:

typedef Pos::IdxPairDataSet::IdxType IdxType;
typedef Pos::IdxPairDataSet::ArrIdxType ArrIdxType;

IdxPairDataSetFromCubeData( IdxPairDataSet& ds,
			    const PosInfo::CubeData& cubedata,
			    EntryCreatedFn crfn )
    : ds_( ds )
    , cubedata_( cubedata )
    , crfn_( crfn )
{
    // Add first spos on each line so all lines are in, thus
    // threadsafe to add things as long as each line is separate
    for ( ArrIdxType idx=0; idx<cubedata.size(); idx++ )
    {
	const PosInfo::LineData& line = *cubedata_[idx];
	const ArrIdxType frst = line.linenr_;
	if ( !line.segments_.isEmpty() )
	    ds.add( IdxPair(frst,line.segments_[0].start) );
    }
}

od_int64 nrIterations() const override { return cubedata_.size(); }

bool doWork( od_int64 start, od_int64 stop, int ) override
{
    for ( IdxPair::IdxType idx=(IdxPair::IdxType)start; idx<=stop; idx++ )
    {
	const PosInfo::LineData& line = *cubedata_[idx];
	const IdxType frst = line.linenr_;
	for ( int idy=0; idy<line.segments_.size(); idy++ )
	{
	    StepInterval<IdxType> crls = line.segments_[idy];
	    if ( idy == 0 )
		crls.start += crls.step; //We added first scnd in constructor
	    for ( IdxType scnd=crls.start; scnd<=crls.stop; scnd+=crls.step )
	    {
		IdxPair ip( frst, scnd );
		IdxPairDataSet::SPos spos = ds_.find( ip );
		if ( spos.isValid() )
		    continue;
		spos = ds_.add( ip );
		if ( !spos.isValid() )
		    return false;
		else if ( crfn_ )
		    crfn_( ds_, spos.i, spos.j );
	    }
	}
    }

    return true;
}

    IdxPairDataSet&	ds_;
    PosInfo::CubeData	cubedata_;
    EntryCreatedFn	crfn_;

};

} // namespace Pos


void Pos::IdxPairDataSet::add( const PosInfo::CubeData& cubedata,
			       EntryCreatedFn crfn )
{
    Pos::IdxPairDataSetFromCubeData task( *this, cubedata, crfn );
    if ( !task.execute() )
	mHandleMemFull()
}


void Pos::IdxPairDataSet::randomSubselect( GlobIdxType maxsz )
{
    const GlobIdxType orgsz = totalSize();
    if ( orgsz <= maxsz )
	return;
    if ( maxsz < 1 )
	{ setEmpty(); return; }

    mGetIdxArr( GlobIdxType, idxs, orgsz );
    if ( !idxs )
	{ setEmpty(); return; }

    const bool buildnew = ((GlobIdxType)maxsz) < (orgsz / ((GlobIdxType)2));
    static Stats::RandGen gen;
    gen.subselect( idxs, orgsz, maxsz );
    TypeSet<SPos> sposs;
    if ( buildnew )
    {
	for ( GlobIdxType idx=0; idx<maxsz; idx++ )
	    sposs += getPos( idxs[idx] );
    }
    else
    {
	for ( GlobIdxType idx=maxsz; idx<orgsz; idx++ )
	    sposs += getPos( idxs[idx] );
    }

    delete [] idxs;

    if ( !buildnew )
	remove( sposs );
    else
    {
	IdxPairDataSet newds( objsz_, allowdup_, mandata_ );
	IdxPair ip;
	for ( GlobIdxType idx=0; idx<sposs.size(); idx++ )
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
    const StepInterval<IdxType> frstrg = hrg.inlRange();
    const StepInterval<IdxType> scndrg = hrg.crlRange();

    TypeSet<SPos> torem;

    SPos spos;
    while ( next(spos) )
    {
	IdxPair ip = gtIdxPair( spos );
	const bool inlinside = frstrg.includes( ip.first, false )
			    && frstrg.snap( ip.first ) == ip.first;
	const bool crlinside = scndrg.includes( ip.second, false )
			    && scndrg.snap( ip.second ) == ip.second;
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


Pos::IdxPairDataSet::ArrIdxType Pos::IdxPairDataSet::nrDuplicateIdxPairs() const
{
    ArrIdxType nrdupips = 0;
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
    return gtObjData(spos).getObj( mandata_, spos.j, objsz_ );
}


void Pos::IdxPairDataSet::putObj( const SPos& spos, const void* obj )
{
    if ( objdatas_.size() <= spos.i )
    {
	pErrMsg("putObj shuld not have to alloc its own objdata");
	try { objdatas_ += new ObjData; }
	catch ( std::bad_alloc )
	    { mHandleMemFull(); return; }
    }

    gtObjData(spos).putObj( mandata_, spos.j, objsz_, obj );
}


bool Pos::IdxPairDataSet::addObj( SPos& spos, IdxType scnd, const void* obj )
{
    IdxSet& scnds = gtScndSet( spos );

    if ( spos.j < 0 )
	spos.j = findIndexFor(scnds,scnd) + 1;
    else
    {
	spos.j++;
	while ( spos.j < scnds.size() && scnds[spos.j] == scnd )
	    spos.j++;
    }

    const bool atend = spos.j > scnds.size() - 1;
    try {
	if ( atend )
	    scnds += scnd;
	else
	    scnds.insert( spos.j, scnd );
    } catch ( std::bad_alloc )
	mErrRetMemFull()

    if ( !gtObjData(spos).addObjSpace(mandata_,spos.j,objsz_) )
	mErrRetMemFull()

    putObj( spos, obj );
    return true;
}


void Pos::IdxPairDataSet::addEntry( const Pos::IdxPair& ip, const void* obj,
				    SPos& spos )
{
    if ( spos.i < 0 )
    {
	spos.i = findIndexFor(frsts_,ip.first) + 1;
	try {
	    ObjData* newdata = new ObjData;
	    if ( spos.i > frsts_.size()-1 )
	    {
		frsts_ += ip.first;
		scndsets_ += new IdxSet;
		objdatas_ += newdata;
		spos.i = frsts_.size() - 1;
	    }
	    else
	    {
		frsts_.insert( spos.i, ip.first );
		scndsets_.insertAt( new IdxSet, spos.i );
		objdatas_.insertAt( newdata, spos.i );
	    }
	} catch ( std::bad_alloc )
	    { mHandleMemFull(); spos.reset(); return; }
    }

    if ( (spos.j < 0 || allowdup_) && !addObj( spos, ip.second, obj ) )
	spos.reset();
}
