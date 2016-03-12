/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Mar 2016
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "posidxpairdataset.h"
#include "idxable.h"
#include "posinfo.h"
#include "od_iostream.h"



static TypeSet<Pos::IdxPairDataSet::IdxType>::size_type findIndexFor(
	const TypeSet<Pos::IdxPairDataSet::IdxType>& idxs,
	Pos::IdxPairDataSet::IdxType nr, bool* found = 0 )
{
    const TypeSet<Pos::IdxPairDataSet::IdxType>::size_type sz = idxs.size();
    TypeSet<Pos::IdxPairDataSet::IdxType>::size_type ret = -1;
    const bool fnd = sz > 0 ? IdxAble::findPos( idxs.arr(), sz, nr, -1, ret )
			    : false;
    if ( found )
	*found = fnd;
    return ret;
}


Pos::IdxPairDataSet::IdxPairDataSet( od_int64 objsz, bool alwdup, bool md )
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

    for ( IdxType ifst=0; ifst<oth.frsts_.size(); ifst++ )
    {
	frsts_ += oth.frsts_[ifst];
	scndsets_ += new IdxSet( *oth.scndsets_[ifst] );
	objsets_ += new ObjSet( *oth.objsets_[ifst] );

	if ( mandata_ )
	{
	    ObjSet& newset = *objsets_.last();
	    for ( int idx=0; idx<newset.size(); idx++ )
		newset.replace( idx, getObjCopy(newset[idx]) );
	}
    }

    return *this;
}


void Pos::IdxPairDataSet::setEmpty()
{
    frsts_.erase();
    deepErase( scndsets_ );
    if ( mandata_ )
    {
	for ( int iscnd=0; iscnd<objsets_.size(); iscnd++ )
	{
	    const ObjSet& curset = *objsets_[iscnd];
	    for ( int idx=0; idx<curset.size(); idx++ )
		deleteObj( curset[idx] );
	}
    }
    deepErase( objsets_ );
}


void* Pos::IdxPairDataSet::getObjCopy( const void* org ) const
{
    if ( objsz_ < 1 )
	return 0;

    void* ret = new StorType[ objsz_ ];
    if ( org )
	OD::memCopy( ret, org, objsz_ );
    else
	OD::memZero( ret, objsz_ );

    return ret;
}


bool Pos::IdxPairDataSet::append( const IdxPairDataSet& oth )
{
    SPos pos;
    while ( oth.next(pos,!allowdup_) )
    {
	IdxPair idxpair = oth.getIdxPair( pos );
	const void* othptr = oth.gtObj( pos );
	const SPos newpos = add( idxpair, othptr );
	if ( !newpos.isValid() )
	    return false;
    }
    return true;
}


void Pos::IdxPairDataSet::remove( const IdxPairDataSet& oth )
{
    SPos othpos;
    while ( oth.next(othpos,true) )
    {
	const IdxPair pair = oth.getIdxPair( othpos );
	SPos mypos = find( pair );

	while ( mypos.isValid() )
	{
	    remove( mypos );
	    mypos = find( pair );
	}
    }
}


int Pos::IdxPairDataSet::nrSecond( IdxType frst ) const
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
    const int frstidx = isall ? -1 : frsts_.indexOf( frst );
    if ( frstidx >= 0 )
    {
	const IdxSet& scndset = gtScndSet( frstidx );
	const int nrscnd = scndset.size();
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
	for ( int idx=0; idx<frsts_.size(); idx++ )
	{
	    const IdxSet& scndset = gtScndSet(idx);
	    const int nrscnd = scndset.size();
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
    const_cast<od_int64&>(objsz_) = oth.objsz_;
    const_cast<bool&>(mandata_) = oth.mandata_;
    allowdup_ = oth.allowdup_;
}


Pos::IdxPairDataSet::SPos Pos::IdxPairDataSet::findOccurrence(
					const IdxPair& ip, int occ ) const
{
    bool found; int idx = findIndexFor( frsts_, ip.first, &found );
    SPos pos( found ? idx : -1, -1 );
    if ( !found )
	return pos;

    if ( pos.i >= 0 )
    {
	const IdxSet& scnds = gtScndSet( pos );
	idx = findIndexFor( scnds, ip.second, &found );
	pos.j = found ? idx : -1;
	if ( found )
	{
	    pos.j = idx;
	    while ( pos.j && scnds[pos.j-1] == ip.second )
		pos.j--;
	}
    }

    if ( found && occ )
    {
	while ( occ > 0 && next(pos) )
	    occ--;
    }

    return pos;
}


bool Pos::IdxPairDataSet::next( SPos& pos, bool skip_dup ) const
{
    if ( pos.i < 0 )
    {
	if ( frsts_.size() < 1 )
	    return false;
	pos.i = pos.j = 0;
	return true;
    }
    else if ( pos.i >= frsts_.size() )
	{ pos.i = pos.j = -1; return false; }
    else if ( pos.j < 0 )
	{ pos.j = 0; return true; }

    const IdxSet& scnds = gtScndSet( pos );
    if ( pos.j > scnds.size()-2 )
    {
	pos.j = 0; pos.i++;
	if ( pos.i >= frsts_.size() )
	    pos.i = pos.j = -1;
	return pos.i >= 0;
    }

    pos.j++;
    if ( skip_dup && scnds[pos.j] == scnds[pos.j-1] )
	return next( pos, true );

    return true;
}


bool Pos::IdxPairDataSet::prev( SPos& pos, bool skip_dup ) const
{
    if ( pos.j < 0 )
    {
	pos.i--;
	if ( pos.i >= 0 )
	    pos.j = gtScndSet(pos).size() - 1;
    }
    if ( pos.i < 0 || pos.j < 0 )
	return false;
    else if ( pos.i == 0 && pos.j == 0 )
	{ pos.i = pos.j = -1; return false; }

    IdxType curscnd = gtScnd( pos );
    if ( pos.j > 0 )
	pos.j--;
    else
	{ pos.i--; pos.j = gtScndSet(pos).size() - 1; }

    if ( !skip_dup )
	return true;

    while ( gtScnd(pos) == curscnd )
	return prev( pos, true );

    return true;
}


bool Pos::IdxPairDataSet::isValid( const IdxPair& ip ) const
{
    return find( ip ).isValid();
}


Pos::IdxPair Pos::IdxPairDataSet::getIdxPair( const SPos& pos ) const
{
    return pos.isValid() ? IdxPair(gtFrst(pos),gtScnd(pos)) : IdxPair::udf();
}


const void* Pos::IdxPairDataSet::getObj( const SPos& pos ) const
{
    return pos.isValid() ? gtObj(pos) : 0;
}


Pos::IdxPairDataSet::SPos Pos::IdxPairDataSet::getPos( od_int64 glidx ) const
{
    od_int64 firstidx = 0; SPos pos;
    for ( pos.i=0; pos.i<frsts_.size(); pos.i++ )
    {
	const IdxSet& scnds = gtScndSet(pos);
	if ( firstidx + scnds.size() > glidx )
	{
	    pos.j = (int)(glidx - firstidx);
	    return pos;
	}
	firstidx += scnds.size();
    }

    return SPos(-1,-1);
}


Pos::IdxPairDataSet::SPos Pos::IdxPairDataSet::add( const Pos::IdxPair& ip,
							const void* obj )
{
    SPos pos( find(ip) );
    if ( pos.i < 0 )
    {
	pos.i = findIndexFor(frsts_,ip.first) + 1;
	ObjSet* newset = new ObjSet;
	newset->allowNull( true );
	if ( pos.i > frsts_.size()-1 )
	{
	    frsts_ += ip.first;
	    scndsets_ += new IdxSet;
	    objsets_ += newset;
	    pos.i = frsts_.size() - 1;
	}
	else
	{
	    frsts_.insert( pos.i, ip.first );
	    scndsets_.insertAt( new IdxSet, pos.i );
	    objsets_.insertAt( newset, pos.i );
	}
    }

    if ( pos.j < 0 || allowdup_ )
	addNew( pos, ip.second, obj );

    return pos;
}


void Pos::IdxPairDataSet::addNew( SPos& pos, IdxType scnd, const void* obj )
{
    IdxSet& scnds = gtScndSet( pos );
    ObjSet& objs = gtObjSet( pos );

    if ( pos.j < 0 )
	pos.j = findIndexFor(scnds,scnd) + 1;
    else
    {
	pos.j++;
	while ( pos.j < scnds.size() && scnds[pos.j] == scnd )
	    pos.j++;
    }

    const void* objtoadd = mandata_ ? getObjCopy( obj ) : obj;
    if ( pos.j > scnds.size() - 1 )
    {
	scnds += scnd;
	objs += objtoadd;
    }
    else
    {
	scnds.insert( pos.j, scnd );
	objs.insertAt( objtoadd, pos.j );
    }
}


void Pos::IdxPairDataSet::set( SPos pos, const void* obj )
{
    if ( !pos.isValid() )
	return;

    const void* objtoput = mandata_ ? getObjCopy( obj ) : obj;
    retireObj( gtObjSet(pos).replace( pos.j, objtoput ) );
}


namespace Pos
{
class IdxPairDataSetFromCubeData : public ::ParallelTask
{
public:

typedef Pos::IdxPairDataSet::IdxType IdxType;

IdxPairDataSetFromCubeData( IdxPairDataSet& ds,
			       const PosInfo::CubeData& cubedata )
    : ds_( ds )
    , cubedata_( cubedata )
{
    //Add first pos on each line so all lines are in, thus
    //threadsafe to add things as long as each line is separate
    for ( int idx=0; idx<cubedata.size(); idx++ )
    {
	const PosInfo::LineData& line = *cubedata_[idx];
	const int frst = line.linenr_;
	if ( !line.segments_.isEmpty() )
	    ds.add( IdxPair(frst,line.segments_[0].start) );
    }
}

od_int64 nrIterations() const { return cubedata_.size(); }

bool doWork( od_int64 start, od_int64 stop, int )
{
    for ( IdxPair::IdxType idx=(IdxPair::IdxType)start; idx<=stop; idx++ )
    {
	const PosInfo::LineData& line = *cubedata_[idx];
	const IdxType frst = line.linenr_;
	for ( int idy=0; idy<line.segments_.size(); idy++ )
	{
	    StepInterval<int> crls = line.segments_[idy];
	    if ( idy == 0 )
		crls.start += crls.step; //We added first scnd in constructor
	    for ( IdxType scnd=crls.start; scnd<=crls.stop; scnd+=crls.step )
		ds_.add( IdxPair(frst,scnd) );
	}
    }

    return true;
}

    IdxPairDataSet&	ds_;
    PosInfo::CubeData	cubedata_;

};

} // namespace Pos


void Pos::IdxPairDataSet::add( const PosInfo::CubeData& cubedata )
{
    Pos::IdxPairDataSetFromCubeData task( *this, cubedata );
    task.execute();
}


int Pos::IdxPairDataSet::nrPos( int frstidx ) const
{
    return frstidx < 0 || frstidx >= frsts_.size() ? 0
		: gtScndSet(frstidx).size();
}


od_int64 Pos::IdxPairDataSet::totalSize() const
{
    od_int64 nr = 0;
    for ( int idx=0; idx<frsts_.size(); idx++ )
	nr += gtScndSet(idx).size();
    return nr;
}


bool Pos::IdxPairDataSet::hasFirst( IdxType frst ) const
{
    return frsts_.isPresent( frst );
}


bool Pos::IdxPairDataSet::hasSecond( IdxType scnd ) const
{
    for ( int ifrst=0; ifrst<frsts_.size(); ifrst++ )
    {
	const IdxSet& scnds = gtScndSet( ifrst );
	for ( int iscnd=0; iscnd<scnds.size(); iscnd++ )
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


void Pos::IdxPairDataSet::remove( const SPos& pos )
{
    if ( pos.i < 0 || pos.i >= frsts_.size() )
	return;

    IdxSet& scnds = gtScndSet( pos );
    if ( pos.j < 0 || pos.j >= scnds.size() )
	return;

    retireObj( gtObjSet(pos).removeSingle( pos.j ) );
    scnds.removeSingle( pos.j );
    if ( scnds.isEmpty() )
    {
	frsts_.removeSingle( pos.i );
	delete scndsets_.removeSingle( pos.i );
	delete objsets_.removeSingle( pos.i );
    }
}


void Pos::IdxPairDataSet::remove( const TypeSet<SPos>& poss )
{
    if ( poss.size() < 1 )
	return;
    else if ( poss.size() == 1 )
	{ remove( poss[0] ); return; }

    IdxPairDataSet ds( *this );
    setEmpty();
    SPos pos;
    while ( ds.next(pos) )
	if ( !poss.isPresent(pos) )
	    add( ds.getIdxPair(pos), ds.getObj(pos) );
}


void Pos::IdxPairDataSet::removeDuplicateIdxPairs()
{
    if ( isEmpty() )
	return;

    SPos pos; next( pos, false );
    IdxPair previp = getIdxPair( pos );
    TypeSet<SPos> poss;
    while ( next(pos,false) )
    {
	IdxPair curip = getIdxPair( pos );
	if ( previp == curip )
	    poss += pos;
	else
	    previp = curip;
    }

    remove( poss );
}


void Pos::IdxPairDataSet::extend( const Pos::IdxPairDelta& so,
				   const Pos::IdxPairStep& sos )
{
    if ( (!so.first && !so.second) || (!sos.first && !sos.second) )
	return;

    IdxPairDataSet ds( *this );

    const bool kpdup = allowdup_;
    allowdup_ = false;

    SPos pos;
    while ( ds.next(pos) )
    {
	IdxPair ip = ds.getIdxPair( pos );
	const IdxPair centralip( ip );
	for ( int ifrst=-so.first; ifrst<=so.first; ifrst++ )
	{
	    ip.first = centralip.first + ifrst * sos.first;
	    for ( int iscnd=-so.second; iscnd<=so.second; iscnd++ )
	    {
		if ( !ifrst && !iscnd )
		    continue;
		ip.second = centralip.second + iscnd * sos.second;
		add( ip, 0 );
	    }
	}
    }

    allowdup_ = kpdup;
}


void Pos::IdxPairDataSet::remove( const TrcKeySampling& hrg,
				  bool inside )
{
    const StepInterval<IdxType> frstrg = hrg.inlRange();
    const StepInterval<IdxType> scndrg = hrg.crlRange();

    TypeSet<SPos> torem;

    SPos pos;
    while ( next(pos) )
    {
	IdxPair ip = getIdxPair( pos );
	const bool inlinside = frstrg.includes( ip.first, false )
			    && frstrg.snap( ip.first ) == ip.first;
	const bool crlinside = scndrg.includes( ip.second, false )
			    && scndrg.snap( ip.second ) == ip.second;
	if ( (inside && inlinside && crlinside)
	  || (!inside && (!inlinside || !crlinside)) )
	    torem += pos;
    }

    remove( torem );
}


bool Pos::IdxPairDataSet::hasDuplicateIdxPairs() const
{
    IdxPair previp = IdxPair::udf();
    SPos pos;
    while ( next(pos) )
    {
	IdxPair ip = getIdxPair( pos );
	if ( previp == ip )
	    return true;
	previp = ip;
    }

    return false;
}


int Pos::IdxPairDataSet::nrDuplicateIdxPairs() const
{
    int nrdupips = 0;
    SPos pos;
    if ( !next(pos) )
	return 0;

    IdxPair previp = getIdxPair( pos );
    while ( next(pos) )
    {
	IdxPair ip = getIdxPair( pos );
	if ( previp == ip )
	{
	    nrdupips++;
	    while ( next(pos) )
	    {
		previp = ip;
		ip = getIdxPair( pos );
		if ( previp != ip )
		    break;
	    }
	}

	if ( !pos.isValid() )
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
