/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 21-6-1996
-*/


#include "posidxpairvalset.h"
#include "posidxpairvalue.h"

#include "arrayndimpl.h"
#include "binidvalset.h"
#include "hiddenparam.h"
#include "iopar.h"
#include "separstr.h"
#include "idxable.h"
#include "posinfo.h"
#include "posinfo2d.h"
#include "sorting.h"
#include "strmoper.h"
#include "statrand.h"
#include "survgeom.h"
#include "survgeom2d.h"
#include "varlenarray.h"
#include "od_iostream.h"

static const float cMaxDistFromGeom = 1000.f;


static inline void setToUdf( float* arr, int nvals )
{
    for ( int idx=0; idx<nvals; idx++ )
	Values::setUdf( arr[idx] );
}


static int findIndexFor( const TypeSet<int>& nrs, int nr, bool* found = 0 )
{
    int ret = -1;
    bool fnd = nrs.size() ? IdxAble::findPos(nrs.arr(),nrs.size(),nr,-1,ret)
			  : false;
    if ( found ) *found = fnd;
    return ret;
}


Pos::IdxPairValueSet::IdxPairValueSet( int nv, bool ad )
	: nrvals_(nv)
	, allowdup_(ad)
{
}


Pos::IdxPairValueSet::IdxPairValueSet( const IdxPairValueSet& s )
	: nrvals_(0)
{
    *this = s;
}


Pos::IdxPairValueSet::~IdxPairValueSet()
{
    setEmpty();
}


Pos::IdxPairValueSet& Pos::IdxPairValueSet::operator =(
						const IdxPairValueSet& vs )
{
    if ( &vs == this ) return *this;

    setEmpty(); copyStructureFrom( vs );

    for ( IdxType ifst=0; ifst<vs.frsts_.size(); ifst++ )
    {
	frsts_ += vs.frsts_[ifst];
	scndsets_ += new TypeSet<IdxType>( *vs.scndsets_[ifst] );
	valsets_ += new TypeSet<float>( *vs.valsets_[ifst] );
    }

    return *this;
}


void Pos::IdxPairValueSet::setEmpty()
{
    frsts_.erase();
    deepErase( scndsets_ );
    deepErase( valsets_ );
}


bool Pos::IdxPairValueSet::append( const IdxPairValueSet& vs )
{
    SPos pos; IdxPair idxpair;
    if ( nrvals_ <= vs.nrvals_ )
    {
	while ( vs.next(pos,!vs.allowdup_) )
	{
	    vs.get( pos, idxpair );
	    const SPos newpos = add( idxpair, nrvals_ ? vs.getVals( pos ) : 0 );
	    if ( !newpos.isValid() )
		return false;
	}
    }
    else
    {
	mAllocVarLenArr( float, insvals, nrvals_ );
	setToUdf(insvals,nrvals_);
	while ( vs.next(pos,!allowdup_) )
	{
	    vs.get( pos, idxpair );
	    OD::memCopy( insvals, vs.getVals( pos ),
			 vs.nrvals_ * sizeof(float) );
	    const SPos newpos = add( idxpair, insvals );
	    if ( !newpos.isValid() )
		return false;
	}
    }

    return true;
}


void Pos::IdxPairValueSet::remove( const IdxPairValueSet& removepairs )
{
    SPos pos;

    while ( removepairs.next(pos, true ) )
    {
	const IdxPair pair = removepairs.getIdxPair( pos );
	SPos removepos = findOccurrence( pair );

	while ( removepos.j>=0 )
	{
	    remove( removepos );
	    removepos = findOccurrence( pair );
	}
    }
}


void Pos::IdxPairValueSet::randomSubselect( od_int64 maxsz )
{
    const od_int64 orgsz = totalSize();
    if ( orgsz <= maxsz )
	return;
    if ( maxsz < 1 )
	{ setEmpty(); return; }

    mGetIdxArr( od_int64, idxs, orgsz );
    if ( !idxs )
	{ setEmpty(); return; }

    const bool buildnew = ((od_int64)maxsz) < (orgsz / ((od_int64)2));
    Stats::randGen().subselect( idxs, orgsz, maxsz );
    TypeSet<SPos> poss;
    if ( buildnew )
    {
	for ( od_int64 idx=0; idx<maxsz; idx++ )
	    poss += getPos( idxs[idx] );
    }
    else
    {
	for ( od_int64 idx=maxsz; idx<orgsz; idx++ )
	    poss += getPos( idxs[idx] );
    }
    delete [] idxs;

    if ( !buildnew )
	remove( poss );
    else
    {
	IdxPairValueSet newvs( nrvals_, allowdup_ );
	DataRow dr;
	for ( od_int64 idx=0; idx<poss.size(); idx++ )
	{
	    get( poss[mCast(int,idx)], dr );
	    newvs.add( dr );
	}
	*this = newvs;
    }
}


bool Pos::IdxPairValueSet::getFrom( od_istream& strm, GeomID gid )
{
    setEmpty();
    if ( !setNrVals( 0, false ) )
	return false;

    BufferString line; char valbuf[1024];
    const Survey::Geometry* survgeom = Survey::GM().getGeometry( gid );
    int coordindic = -1;

    while ( strm.getLine( line ) )
    {
	char* firstchar = line.getCStr();
	mSkipBlanks( firstchar );
	if ( *firstchar == '"' )
	{
	    char* ptr = firstOcc( firstchar+1, '"' );
	    if ( !ptr ) continue;
	    firstchar = ptr+1;
	}
	mSkipBlanks( firstchar );
	if ( !*firstchar || (*firstchar != '-' && !iswdigit(*firstchar) ) )
	    continue;

	const char* nextword = getNextWord( firstchar, valbuf );
	Coord coord;
	coord.x = toDouble( valbuf );
	mSkipBlanks( nextword ); if ( !*nextword ) continue;
	nextword = getNextWord( nextword, valbuf );
	coord.y = toInt( valbuf );

	if ( coordindic < 0 )
	{
	    float dist = mUdf(float);
	    if ( survgeom )
		(void)survgeom->nearestTrace( coord, &dist );

	    const char* firstval = nextword;
	    int nrvalsfound = 0;
	    while ( true )
	    {
		mSkipBlanks( nextword ); if ( !*nextword ) break;
		nrvalsfound++;
		nextword = getNextWord( nextword, valbuf );
	    }
	    setNrVals( nrvalsfound, false );
	    nextword = firstval;
	    coordindic = dist < cMaxDistFromGeom ? 1 : 0;
	}

	TrcKey tk;
	if ( coordindic == 1 )
	    tk = survgeom->nearestTrace( coord );
	else
	{
	    tk.lineNr() = (Pos::LineID)(coord.x + 0.5);
	    tk.trcNr() = (Pos::TraceID)(coord.y + 0.5);
	}

	float* vals = getVals( add(tk.pos()) );
	if ( !vals ) continue;
	for ( int idx=0; idx<nrVals(); idx++ )
	{
	    mSkipBlanks( nextword ); if ( !*nextword ) break;
	    nextword = getNextWord( nextword, valbuf );
	    if ( !valbuf[0] ) break;
	    vals[idx] = toFloat(valbuf);
	}
    }

    return !isEmpty();
}


bool Pos::IdxPairValueSet::putTo( od_ostream& strm ) const
{
    SPos pos;
    while ( next(pos) )
    {
	const IdxPair ip( getFrst(pos), getScnd(pos) );
	const float* vals = getVals(pos);
	strm << ip.first << od_tab << ip.second;
	for ( int idx=0; idx<nrvals_; idx++ )
	    strm << od_tab << toString( vals[idx] );
	strm << od_newline;
    }
    strm.flush();
    return strm.isOK();
}


int Pos::IdxPairValueSet::nrSecond( IdxType frst ) const
{
    const IdxType frstidx = frsts_.indexOf( frst );
    return frstidx<0 ? 0 : getScndSet(frstidx).size();
}


Interval<Pos::IdxPairValueSet::IdxType> Pos::IdxPairValueSet::firstRange() const
{
    Interval<IdxType> ret( mUdf(IdxType), mUdf(IdxType) );

    bool first = true;
    for ( IdxType ifrst=0; ifrst<frsts_.size(); ifrst++ )
    {
	if ( first )
	    { ret.start = ret.stop = frsts_[ifrst]; first = false; }
	else
	    ret.include( frsts_[ifrst], false );
    }
    return ret;
}


Interval<Pos::IdxPairValueSet::IdxType> Pos::IdxPairValueSet::secondRange(
							IdxType frst ) const
{
    Interval<IdxType> ret( mUdf(IdxType), mUdf(IdxType) );
    if ( frsts_.isEmpty() ) return ret;

    const int frstidx = frsts_.indexOf( frst );
    if ( frstidx >= 0 )
    {
	const TypeSet<IdxType>& scndset = getScndSet(frstidx);
	const int nrscnd = scndset.size();
	if ( nrscnd>=1 )
	    ret.start = ret.stop = scndset[0];
	if ( nrscnd>1 )
	    ret.include( scndset[nrscnd-1], false );

    }
    else
    {
	bool found = false;
	for ( int idx=0; idx<frsts_.size(); idx++ )
	{
	    const TypeSet<IdxType>& scndset = getScndSet(idx);
	    const int nrscnd = scndset.size();
	    if ( nrscnd>=1 )
	    {
		if ( found )
		    ret.include( scndset[0], false );
		else
		{
		    ret.start = ret.stop = scndset[0];
		    found = true;
		}
	    }

	    if ( nrscnd>1 )
		ret.include( scndset[nrscnd-1], false );
	}
    }

    return ret;
}


Interval<float> Pos::IdxPairValueSet::valRange( int valnr ) const
{
    Interval<float> ret( mUdf(float), mUdf(float) );
    if ( valnr >= nrvals_ || valnr < 0 || isEmpty() )
	return ret;

    SPos pos;
    while ( next(pos) )
    {
	const float val = getVals(pos)[valnr];
	if ( !mIsUdf(val) )
	    { ret.start = ret.stop = val; break; }
    }
    while ( next(pos) )
    {
	const float val = getVals(pos)[valnr];
	if ( !mIsUdf(val) )
	    ret.include( val, false );
    }

    return ret;
}


void Pos::IdxPairValueSet::copyStructureFrom( const IdxPairValueSet& vs )
{
    setEmpty();
    const_cast<int&>(nrvals_) = vs.nrvals_;
    allowdup_ = vs.allowdup_;
}


Pos::IdxPairValueSet::SPos Pos::IdxPairValueSet::findOccurrence(
					const IdxPair& ip, int occ ) const
{
    bool found; int idx = findIndexFor(frsts_,ip.first,&found);
    SPos pos( found ? idx : -1, -1 );
    if ( pos.i >= 0 )
    {
	const TypeSet<IdxType>& scnds = getScndSet(pos);
	idx = findIndexFor(scnds,ip.second,&found);
	pos.j = found ? idx : -1;
	if ( found )
	{
	    pos.j = idx;
	    while ( pos.j && scnds[pos.j-1] == ip.second )
		pos.j--;
	}
    }

    while ( occ > 0 && next(pos) )
	occ--;
    return pos;
}


bool Pos::IdxPairValueSet::next( SPos& pos, bool skip_dup ) const
{
    if ( pos.i < 0 )
    {
	if ( frsts_.size() < 1 ) return false;
	pos.i = pos.j = 0;
	return true;
    }
    else if ( pos.i >= frsts_.size() )
	{ pos.i = pos.j = -1; return false; }
    else if ( pos.j < 0 )
	{ pos.j = 0; return true; }

    const TypeSet<IdxType>& scnds = getScndSet(pos);
    if ( pos.j > scnds.size()-2 )
    {
	pos.j = 0;
	pos.i++;
	if ( pos.i >= frsts_.size() )
	    pos.i = pos.j = -1;
	return pos.i >= 0;
    }

    pos.j++;
    if ( skip_dup && scnds[pos.j] == scnds[pos.j-1] )
	return next( pos, true );

    return true;
}


bool Pos::IdxPairValueSet::prev( SPos& pos, bool skip_dup ) const
{
    if ( !pos.isValid() )
	return false;
    if ( pos.i == 0 && pos.j == 0)
	{ pos.i = pos.j = -1; return false; }

    IdxType curscnd = getScnd(pos);
    if ( pos.j )
	pos.j--;
    else
    {
	pos.i--;
	pos.j = getScndSet(pos).size() - 1;
    }

    if ( !skip_dup ) return true;

    while ( getScnd(pos) == curscnd )
	return prev( pos, true );

    return true;
}


bool Pos::IdxPairValueSet::isValid( const IdxPair& ip ) const
{
    SPos pos = find( ip );
    return pos.isValid()
	&& frsts_.isPresent(ip.first)
	&& getScndSet(pos).size() > pos.j;
}


void Pos::IdxPairValueSet::get( const SPos& pos, IdxPair& ip, float* vs,
			 int maxnrvals ) const
{
    if ( maxnrvals < 0 || maxnrvals > nrvals_ ) maxnrvals = nrvals_;

    if ( !pos.isValid() )
	{ ip.first = ip.second = 0; }
    else
    {
	ip.first = getFrst(pos); ip.second = getScnd(pos);
	if ( vs && maxnrvals )
	{
	    OD::memCopy( vs, getVals(pos), maxnrvals * sizeof(float) );
	    return;
	}
    }

    if ( vs )
	setToUdf(vs,maxnrvals);
}


Pos::IdxPair Pos::IdxPairValueSet::getIdxPair( const SPos& pos ) const
{
    return pos.isValid() ? IdxPair(getFrst(pos),getScnd(pos)) : IdxPair::udf();
}


Pos::IdxPairValueSet::SPos Pos::IdxPairValueSet::getPos( od_int64 glidx ) const
{
    od_int64 firstidx = 0; SPos pos;
    for ( pos.i=0; pos.i<frsts_.size(); pos.i++ )
    {
	const TypeSet<IdxType>& scnds = getScndSet(pos);
	if ( firstidx + scnds.size() > glidx )
	{
	    pos.j = (int)(glidx - firstidx);
	    return pos;
	}
	firstidx += scnds.size();
    }

    return SPos(-1,-1);
}


Pos::IdxPairValueSet::SPos Pos::IdxPairValueSet::add( const Pos::IdxPair& ip,
							const float* arr )
{
    SPos pos( find(ip) );
    if ( pos.i < 0 )
    {
	pos.i = findIndexFor(frsts_,ip.first) + 1;
	if ( pos.i > frsts_.size()-1 )
	{
	    frsts_ += ip.first;
	    scndsets_ += new TypeSet<IdxType>;
	    valsets_ += new TypeSet<float>;
	    pos.i = frsts_.size() - 1;
	}
	else
	{
	    frsts_.insert( pos.i, ip.first );
	    scndsets_.insertAt( new TypeSet<IdxType>, pos.i );
	    valsets_.insertAt( new TypeSet<float>, pos.i );
	}
    }

    if ( pos.j < 0 || allowdup_ )
	addNew( pos, ip.second, arr );

    return pos;
}


void Pos::IdxPairValueSet::addNew( SPos& pos, IdxType scnd, const float* arr )
{
    TypeSet<IdxType>& scnds = getScndSet(pos);

    if ( pos.j < 0 )
	pos.j = findIndexFor(scnds,scnd) + 1;
    else
    {
	pos.j++;
	while ( pos.j < scnds.size() && scnds[pos.j] == scnd )
	    pos.j++;
    }


    TypeSet<float>& vals = getValSet(pos);
    if ( pos.j > scnds.size() - 1 )
    {
	scnds += scnd;
	for ( int idx=0; idx<nrvals_; idx++ )
	    vals += arr ? arr[idx] : mUdf(float);
    }
    else
    {
	scnds.insert( pos.j, scnd );
	for ( int idx=nrvals_-1; idx>=0; idx-- )
	    vals.insert( pos.j*nrvals_, arr ? arr[idx] : mUdf(float) );
    }
}


Pos::IdxPairValueSet::SPos Pos::IdxPairValueSet::add( const DataRow& dr )
{
    if ( dr.size() >= nrvals_ )
	return add( dr, dr.values() );

    DataRow locdr( 0, 0, nrvals_ );
    for ( int idx=0; idx<dr.size(); idx++ )
	locdr.value(idx) = dr.value(idx);
    for ( int idx=dr.size(); idx<nrvals_; idx++ )
	mSetUdf( locdr.value(idx) );
    return add( dr, locdr.values() );
}


namespace Pos
{
class IdxPairValueSetFromCubeData : public ::ParallelTask
{
public:

typedef Pos::IdxPairValueSet::IdxType IdxType;

IdxPairValueSetFromCubeData( IdxPairValueSet& vs,
			       const PosInfo::CubeData& cubedata )
    : vs_( vs )
    , cubedata_( cubedata )
{
    //Add first pos on each line so all lines are in, thus
    //threadsafe to add things as long as each line is separate
    for ( int idx=0; idx<cubedata.size(); idx++ )
    {
	const PosInfo::LineData& line = *cubedata_[idx];
	const int frst = line.linenr_;
	if ( line.segments_.size() )
	    vs.add( IdxPair(frst,line.segments_[0].start) );
    }
}

od_int64 nrIterations() const { return cubedata_.size(); }

bool doWork( od_int64 start, od_int64 stop, int )
{
    for ( int idx=mCast(int,start); idx<=stop; idx++ )
    {
	const PosInfo::LineData& line = *cubedata_[idx];
	const IdxType frst = line.linenr_;
	for ( int idy=0; idy<line.segments_.size(); idy++ )
	{
	    StepInterval<int> crls = line.segments_[idy];
	    if ( !idy )
		crls.start += crls.step; //We added first scnd in constructor
	    for ( IdxType scnd=crls.start; scnd<=crls.stop; scnd+=crls.step )
		vs_.add( IdxPair(frst,scnd) );
	}
    }

    return true;
}

    IdxPairValueSet&	vs_;
    PosInfo::CubeData	cubedata_;

};

} // namespace Pos


void Pos::IdxPairValueSet::add( const PosInfo::CubeData& cubedata )
{
    Pos::IdxPairValueSetFromCubeData task( *this, cubedata );
    task.execute();
}


void Pos::IdxPairValueSet::set( SPos pos, const float* vals )
{
    if ( !pos.isValid() || !nrvals_ ) return;

    if ( vals )
	OD::memCopy( getVals(pos), vals, nrvals_*sizeof(float) );
    else
	setToUdf( getVals(pos), nrvals_ );
}


int Pos::IdxPairValueSet::nrPos( int frstidx ) const
{
    return frstidx < 0 || frstidx >= frsts_.size() ? 0
		: getScndSet(frstidx).size();
}


od_int64 Pos::IdxPairValueSet::totalSize() const
{
    od_int64 nr = 0;
    for ( int idx=0; idx<frsts_.size(); idx++ )
	nr += getScndSet(idx).size();
    return nr;
}


bool Pos::IdxPairValueSet::hasFirst( IdxType frst ) const
{
    return frsts_.isPresent(frst);
}


bool Pos::IdxPairValueSet::hasSecond( IdxType scnd ) const
{
    for ( int ifrst=0; ifrst<frsts_.size(); ifrst++ )
    {
	const TypeSet<IdxType>& scnds = getScndSet(ifrst);
	for ( int iscnd=0; iscnd<scnds.size(); iscnd++ )
	    if ( scnds[iscnd] == scnd ) return true;
    }
    return false;
}


Pos::IdxPair Pos::IdxPairValueSet::firstIdxPair() const
{
    SPos pos; next(pos);
    IdxPair ip( IdxPair::udf() ); get(pos,ip);
    return ip;
}


void Pos::IdxPairValueSet::remove( const SPos& pos )
{
    if ( pos.i < 0 || pos.i >= frsts_.size() )
	return;

    TypeSet<IdxType>& scnds = getScndSet(pos);
    if ( pos.j < 0 || pos.j >= scnds.size() )
	return;

    scnds.removeSingle( pos.j );
    if ( scnds.size() )
    {
	if ( nrvals_ )
	    getValSet(pos).removeRange( pos.j*nrvals_, (pos.j+1)*nrvals_ - 1 );
    }
    else
    {
	removeLine( pos.i );
    }
}


void Pos::IdxPairValueSet::remove( const TypeSet<SPos>& poss )
{
    if ( poss.size() < 1 )
	return;
    else if ( poss.size() == 1 )
	{ remove( poss[0] ); return; }

    IdxPairValueSet rmvs( 0, false );
    for ( int idx=0; idx<poss.size(); idx++ )
	rmvs.add( IdxPair(poss[idx].i,poss[idx].j) );

    IdxPairValueSet vs( *this );
    setEmpty();

    SPos pos; IdxPair ip;
    while ( vs.next(pos) )
    {
	if ( !rmvs.isValid(IdxPair(pos.i,pos.j)) )
	{
	    vs.get( pos, ip );
	    add( ip, vs.getVals(pos) );
	}
    }
}


void Pos::IdxPairValueSet::removeDuplicateIdxPairs()
{
    if ( isEmpty() ) return;

    SPos pos; next(pos,false);
    IdxPair previp; get( pos, previp );
    TypeSet<SPos> poss;
    IdxPair cur;
    while ( next(pos,false) )
    {
	get( pos, cur );
	if ( previp == cur )
	    poss += pos;
	else
	    previp = cur;
    }
    remove( poss );
}


void Pos::IdxPairValueSet::removeVal( int validx )
{
    if ( validx < 0 || validx >= nrvals_ )
	return;

    if ( nrvals_ == 1 )
    {
	setNrVals( 0, false );
	return;
    }

    for ( int ifrst=0; ifrst<frsts_.size(); ifrst++ )
    {
	TypeSet<float>& vals = getValSet(ifrst);
	TypeSet<IdxType>& scnds = getScndSet(ifrst);
	for ( int iscnd=scnds.size()-1; iscnd>=0; iscnd-- )
	    vals.removeSingle( nrvals_*iscnd+validx );
    }
    const_cast<int&>(nrvals_)--;
}


bool Pos::IdxPairValueSet::insertVal( int validx )
{
    if ( validx < 0 || validx >= nrvals_ )
	return false;

    const int oldnrvals = nrvals_;
    const_cast<int&>( nrvals_ ) = oldnrvals+1;

    for ( int ifrst=0; ifrst<frsts_.size(); ifrst++ )
    {
	const int nrscnd = getScndSet(ifrst).size();
	TypeSet<float>* oldvals = valsets_[ifrst];
	mDeclareAndTryAlloc( TypeSet<float>*, newvals,
			     TypeSet<float>( nrscnd*nrvals_, mUdf(float) ) );
	if ( !newvals )
	    return false;
	valsets_.replace( ifrst, newvals );
	float* oldarr = oldvals->arr();
	float* newarr = newvals->arr();
	for ( int iscnd=0; iscnd<nrscnd; iscnd++ )
	{
	    OD::memCopy( newarr+(iscnd*nrvals_), oldarr+(iscnd*oldnrvals),
		    (validx) * sizeof(float) );
	    OD::memCopy( newarr+(iscnd*nrvals_+validx+1),
		    oldarr+(iscnd*oldnrvals+validx+1),
		    (oldnrvals-validx) * sizeof(float) );
	}

	delete oldvals;
    }

    return true;
}


bool Pos::IdxPairValueSet::setNrVals( int newnrvals, bool keepdata )
{
    if ( newnrvals==nrvals_ )
	return true;

    const int oldnrvals = nrvals_;
    const_cast<int&>( nrvals_ ) = newnrvals;

    for ( int ifrst=0; ifrst<frsts_.size(); ifrst++ )
    {
	const int nrscnd = getScndSet(ifrst).size();
	if ( nrvals_ == 0 )
	    getValSet(ifrst).erase();
	else if ( oldnrvals == 0 )
	    getValSet(ifrst).setSize( nrscnd * nrvals_, mUdf(float) );
	else
	{
	    TypeSet<float>* oldvals = valsets_[ifrst];
	    mDeclareAndTryAlloc( TypeSet<float>*, newvals,
				 TypeSet<float>( nrscnd*nrvals_, mUdf(float) ));
	    if ( !newvals )
		return false;
	    valsets_.replace( ifrst, newvals );
	    if ( keepdata )
	    {
		float* oldarr = oldvals->arr();
		float* newarr = newvals->arr();
		const int cpsz = (oldnrvals > nrvals_ ? nrvals_ : oldnrvals)
				   * sizeof( float );
		for ( int iscnd=0; iscnd<nrscnd; iscnd++ )
		    OD::memCopy( newarr+iscnd*nrvals_, oldarr+iscnd*oldnrvals,
				 cpsz );
	    }
	    delete oldvals;
	}
    }

    return true;
}


void Pos::IdxPairValueSet::sortDuplicateIdxPairs( int valnr, bool asc )
{
    if ( valnr >= nrvals_ || !allowdup_ ) return;

    for ( int ifrst=0; ifrst<frsts_.size(); ifrst++ )
    {
	TypeSet<IdxType>& scnds = getScndSet(ifrst);
	TypeSet<float>& vals = getValSet(ifrst);
	for ( int iscnd=1; iscnd<scnds.size(); iscnd++ )
	{
	    IdxType curscnd = scnds[iscnd];
	    IdxType firstdup = iscnd - 1;
	    if ( scnds[iscnd-1] == curscnd )
	    {
		for ( int idup=iscnd+1; idup<scnds.size(); idup++ )
		{
		    if ( scnds[idup] == curscnd )
			iscnd = idup;
		    else
			break;
		}
		sortPart( scnds, vals, valnr, firstdup, iscnd, asc );
	    }
	}
    }
}


void Pos::IdxPairValueSet::sortPart( TypeSet<IdxType>& scnds,
	TypeSet<float>& vals, int valnr, int firstidx, int lastidx, bool asc )
{
    int nridxs = lastidx - firstidx + 1;
    float* vs = new float [ nridxs ];
    int* idxs = new int [ nridxs ];
    for ( int idx=firstidx; idx<=lastidx; idx++ )
    {
	idxs[idx] = idx;
	vs[idx-firstidx] = vals[nrvals_*idx+valnr];
    }
    sort_coupled( vs, idxs, nridxs );

    for ( int idx=0; idx<nridxs; idx++ )
    {
	if ( idxs[idx] != idx )
	{
	    Swap( scnds[idx], scnds[ idxs[idx] ] );
	    for ( int iv=0; iv<nrvals_; iv++ )
		Swap( vals[ idx*nrvals_ + iv], vals[ idxs[idx]*nrvals_ + iv ] );
	}
    }

    delete [] vs; delete [] idxs;
}


void Pos::IdxPairValueSet::extend( const Pos::IdxPairDelta& so,
				   const Pos::IdxPairStep& sos )
{
    if ( (!so.first && !so.second) || (!sos.first && !sos.second) ) return;

    IdxPairValueSet vs( *this );

    const bool kpdup = allowdup_;
    allowdup_ = false;

    SPos pos; IdxPair ip;
    float* vals = nrvals_ ? new float [nrvals_] : 0;
    while ( vs.next(pos) )
    {
	vs.get( pos, ip, vals );
	const IdxPair centralip( ip );
	for ( int ifrst=-so.first; ifrst<=so.first; ifrst++ )
	{
	    ip.first = centralip.first + ifrst * sos.first;
	    for ( int iscnd=-so.second; iscnd<=so.second; iscnd++ )
	    {
		if ( !ifrst && !iscnd )
		    continue;
		ip.second = centralip.second + iscnd * sos.second;
		add( ip, vals );
	    }
	}
    }

    delete [] vals;
    allowdup_ = kpdup;
}


void Pos::IdxPairValueSet::getColumn( int valnr, TypeSet<float>& vals,
				bool incudf ) const
{
    if ( valnr < 0 || valnr >= nrVals() ) return;
    SPos pos;
    while ( next(pos) )
    {
	const float* v = getVals( pos );
	if ( incudf || !mIsUdf(v[valnr]) )
	    vals += v[ valnr ];
    }
}


void Pos::IdxPairValueSet::removeRange( int valnr, const Interval<float>& rg,
				 bool inside )
{
    if ( valnr < 0 || valnr >= nrVals() ) return;
    TypeSet<SPos> poss; SPos pos;
    while ( next(pos) )
    {
	const float* v = getVals( pos );
	if ( inside == rg.includes(v[valnr],true) )
	    poss += pos;
    }
    remove( poss );
}


void Pos::IdxPairValueSet::remove( const TrcKeySampling& hrg,
				   bool removeinside )
{
    const StepInterval<IdxType> frstrg = hrg.inlRange();
    const StepInterval<IdxType> scndrg = hrg.crlRange();

    for ( int idx=frsts_.size()-1; idx>=0; idx-- )
    {
	const int frst = frsts_[idx];
	bool isin = frstrg.includes(frst,false) && frstrg.snap( frst )== frst;
	if ( isin==removeinside )
	    removeLine( idx );
	else
	{
	    TypeSet<IdxType>& scnds = *scndsets_[idx];
	    TypeSet<float>& vals = *valsets_[idx];
	    for ( int idy=scnds.size()-1; idy>=0; idy-- )
	    {
		const IdxType scnd = scnds[idy];
		isin = scndrg.includes(scnd,false) && scndrg.snap( scnd )==scnd;
		if ( isin==removeinside )
		{
		    scnds.removeSingle( idy );
		    vals.removeRange( idy*nrvals_, idy*nrvals_+nrvals_-1 );
		}
	    }
	}
    }
}


void Pos::IdxPairValueSet::removeLine( int idx )
{
    frsts_.removeSingle( idx );
    delete scndsets_.removeSingle( idx );
    delete valsets_.removeSingle( idx );
}


Pos::IdxPairValueSet::SPos Pos::IdxPairValueSet::add( const PairVal& pv )
{
    return nrvals_ < 2 ? add(pv,pv.val()) : add(DataRow(pv));
}


Pos::IdxPairValueSet::SPos Pos::IdxPairValueSet::add( const Pos::IdxPair& ip,
								double v )
{ return add( ip, mCast(float,v) ); }


Pos::IdxPairValueSet::SPos Pos::IdxPairValueSet::add( const Pos::IdxPair& ip,
							float v )
{
    if ( nrvals_ < 2 )
	return add( ip, nrvals_ == 1 ? &v : 0 );

    DataRow dr( ip, 1 );
    dr.value(0) = v;
    return add( dr );
}


Pos::IdxPairValueSet::SPos Pos::IdxPairValueSet::add( const Pos::IdxPair& ip,
							float v1, float v2 )
{
    if ( nrvals_ == 0 )
	return add( ip );
    else if ( nrvals_ < 3 )
    {
	float v[2]; v[0] = v1; v[1] = v2;
	return add( ip, v );
    }

    DataRow dr( ip, 2 );
    dr.value(0) = v1; dr.value(1) = v2;
    return add( dr );
}


Pos::IdxPairValueSet::SPos Pos::IdxPairValueSet::add( const Pos::IdxPair& ip,
				       const TypeSet<float>& vals )
{
    if ( vals.isEmpty() )
	return add( ip );
    else if ( vals.size() >= nrvals_ )
	return add( ip, vals.arr() );

    DataRow dr( ip, vals.size() );
    dr.setVals( vals.arr() );
    return add( dr );
}


void Pos::IdxPairValueSet::get( const SPos& pos, DataRow& dr ) const
{
    dr.setSize( nrvals_ );
    get( pos, dr, dr.values() );
}


void Pos::IdxPairValueSet::get( const SPos& pos, PairVal& pv ) const
{
    if ( nrvals_ < 2 )
	get( pos, pv, &pv.val() );
    else
    {
	DataRow dr; get( pos, dr );
	pv.set( dr ); pv.set( dr.value(0) );
    }
}


void Pos::IdxPairValueSet::get( const SPos& pos, Pos::IdxPair& ip,
					float& v ) const
{
    if ( nrvals_ < 2 )
	get( pos, ip, &v );
    else
    {
	DataRow dr; get( pos, dr );
	ip = dr; v = dr.value(0);
    }
}


void Pos::IdxPairValueSet::get( const SPos& pos, Pos::IdxPair& ip,
				float& v1, float& v2 ) const
{
    if ( nrvals_ < 3 )
    {
	float v[2]; get( pos, ip, v );
	v1 = v[0]; v2 = v[1];
    }
    else
    {
	DataRow dr; get( pos, dr );
	ip = dr; v1 = dr.value(0); v2 = dr.value(1);
    }
}


void Pos::IdxPairValueSet::get( const SPos& pos, Pos::IdxPair& ip,
				TypeSet<float>& vals, int maxnrvals ) const
{
    if ( maxnrvals < 0 || maxnrvals > nrvals_ ) maxnrvals = nrvals_;

    if ( vals.size() != maxnrvals )
    {
	vals.erase();
	for ( int idx=0; idx<maxnrvals; idx++ )
	    vals += mUdf(float);
    }
    get( pos, ip, vals.arr(), maxnrvals );
}


void Pos::IdxPairValueSet::set( const SPos& pos, float v )
{
    if ( nrvals_ < 1 ) return;

    if ( nrvals_ == 1 )
	set( pos, &v );
    else
    {
	TypeSet<float> vals( nrvals_, mUdf(float) );
	vals[0] = v; set( pos, vals );
    }
}


void Pos::IdxPairValueSet::set( const SPos& pos, float v1, float v2 )
{
    if ( nrvals_ < 1 ) return;

    if ( nrvals_ == 2 )
    {
	float v[2]; v[0] = v1; v[1] = v2;
	set( pos, v );
    }
    else
    {
	TypeSet<float> vals( nrvals_, mUdf(float) );
	vals[0] = v1; if ( nrvals_ > 1 ) vals[1] = v2; set( pos, vals );
    }
}


void Pos::IdxPairValueSet::set( const SPos& pos, const TypeSet<float>& v )
{
    if ( nrvals_ < 1 ) return;

    if ( nrvals_ <= v.size() )
	set( pos, v.arr() );
    else
    {
	TypeSet<float> vals( nrvals_, mUdf(float) );
	for ( int idx=0; idx<v.size(); idx++ )
	    vals[idx] = v[idx];

	set( pos, vals.arr() );
    }
}


void Pos::IdxPairValueSet::fillPar( IOPar& iop, const char* ky ) const
{
    FileMultiString fms;
    fms += nrvals_; fms += allowdup_ ? "D" : "N";
    BufferString key; if ( ky && *ky ) { key = ky; key += ".Setup"; }
    iop.set( key, fms );

    for ( int ifrst=0; ifrst<frsts_.size(); ifrst++ )
    {
	fms = ""; fms += frsts_[ifrst];
	const TypeSet<IdxType>& scnds = getScndSet(ifrst);
	const TypeSet<float>& vals = getValSet(ifrst);
	for ( int iscnd=0; iscnd<scnds.size(); iscnd++ )
	{
	    fms += scnds[iscnd];
	    if ( nrvals_ )
	    {
		const float* v = vals.arr() + iscnd*nrvals_;
		for ( int idx=0; idx<nrvals_; idx++ )
		    fms += v[idx];
	    }
	}
	if ( ky && *ky )
	    { key = ky; key += "."; }
	else
	    key = "";
	key += ifrst;
	iop.set( key, fms );
    }
}


void Pos::IdxPairValueSet::usePar( const IOPar& iop, const char* ky )
{
    DataRow dr( 0, 0, nrvals_ );
    FileMultiString fms;
    BufferString key; if ( ky && *ky ) { key = ky; key += ".Setup"; }
    const char* res = iop.find( key );
    if ( res && *res )
    {
	setEmpty();
	fms = res;
	setNrVals( fms.getIValue(0), false );
	allowdup_ = *fms[1] == 'D';
    }

    for ( int ifrst=0; ; ifrst++ )
    {
	if ( ky && *ky )
	    { key = ky; key += "."; }
	else
	    key = "";
	key += ifrst;
	res = iop.find( key );
	if ( !res ) return;
	if ( !*res ) continue;

	fms = res;
	dr.first = fms.getIValue( 0 );
	int nrpos = (fms.size() - 1) / (nrvals_ + 1);
	for ( int iscnd=0; iscnd<nrpos; iscnd++ )
	{
	    int fmsidx = 1 + iscnd * (nrvals_ + 1);
	    dr.second = fms.getIValue( fmsidx );
	    fmsidx++;
	    for ( int ival=0; ival<nrvals_; ival++ )
		dr.value(ival) = fms.getFValue( fmsidx+ival );
	    add( dr );
	}
    }
}


bool Pos::IdxPairValueSet::haveDataRow( const DataRow& dr ) const
{
    SPos pos = find( dr );
    bool found = false;
    IdxPair tmpip;
    while ( !found && pos.isValid() )
    {
	if ( getIdxPair(pos) != dr )
	    break;

	TypeSet<float> valofset;
	get( pos, tmpip, valofset );
	if ( valofset.size() == dr.size() )
	{
	    bool diff = false;
	    for ( int idx=0; idx<valofset.size(); idx++ )
	    {
		if ( dr.value(idx) != valofset[idx] )
		    diff = true;
	    }
	    found = !diff;
	}
	next( pos );
    }

    return found;
}


bool Pos::IdxPairValueSet::hasDuplicateIdxPairs() const
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


int Pos::IdxPairValueSet::nrDuplicateIdxPairs() const
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


int Pos::IdxPairValueSet::firstIdx( int pos ) const
{
    bool found = false;
    const int idx = findIndexFor( frsts_, pos, &found );
    return found ? idx : -1;
}


int Pos::IdxPairValueSet::secondIdx( int firstidx, int pos ) const
{
    if ( !frsts_.validIdx(firstidx) )
	return -1;
    bool found = false;
    const int idx = findIndexFor( *scndsets_[firstidx], pos, &found );
    return found ? idx : -1;
}


int Pos::IdxPairValueSet::firstAtIdx( int firstidx ) const
{
    return frsts_.validIdx(firstidx) ? frsts_[firstidx] : mUdf(int);
}


void Pos::IdxPairValueSet::getSecondsAtIdx( int firstidx,
					    TypeSet<int>& seconds ) const
{
    seconds.setEmpty();
    if ( !frsts_.validIdx(firstidx) )
	return;

    const TypeSet<int>& scnds = *scndsets_[firstidx];
    const int sz = scnds.size();
    if ( sz < 1 )
	return;

    seconds.add( scnds.get(0) );
    for ( int idx=1; idx<sz; idx++ )
    {
	const int scnd = scnds.get( idx );
	if ( scnd != scnds.get(idx-1) )
	    seconds.add( scnd );
    }
}


void Pos::IdxPairValueSet::updNearest( const IdxPair& ip, const SPos& spos,
		    od_int64& mindistsq, SPos& ret ) const
{
    const auto curip = getIdxPair( spos );
    const IdxPair ipdiff( curip.first-ip.first,
			  curip.second-ip.second );
    od_int64 distsq = ((od_int64)ipdiff.first) * ipdiff.first;
    distsq += ((od_int64)ipdiff.second) * ipdiff.second;
    if ( distsq < mindistsq )
	{ ret = spos; mindistsq = distsq; }
}


Pos::IdxPairValueSet::SPos Pos::IdxPairValueSet::findNearest(
					const IdxPair& ip ) const
{
    SPos ret;
    const auto nrfrst = frsts_.size();
    if ( nrfrst < 1 )
	return ret;

    od_int64 mindistsq = mUdf( od_int64 );


    for ( SPos spos(0); spos.i<nrfrst; spos.i++ )
    {
	const TypeSet<int>& scnds = *scndsets_[spos.i];
	const auto lastj = scnds.size() - 1;
	bool found;
	spos.j = findIndexFor( scnds, ip.second, &found );
	if ( found )
	    updNearest( ip, spos, mindistsq, ret );
	else if ( spos.j < 0 )
	    { spos.j = 0; updNearest( ip, spos, mindistsq, ret ); }
	else if ( spos.j > lastj )
	    { spos.j = lastj; updNearest( ip, spos, mindistsq, ret ); }
	else
	{
	    updNearest( ip, spos, mindistsq, ret );
	    if ( spos.j < lastj )
		{ spos.j++; updNearest( ip, spos, mindistsq, ret ); }
	}
    }

    return ret;
}


Pos::IdxPairValueSet::SPos Pos::IdxPairValueSet::findNearestOnFirst(
					int frst, int scnd ) const
{
    const auto nrfrst = frsts_.size();
    if ( nrfrst < 1 )
	return SPos();

    bool found;
    SPos spos;
    spos.i = findIndexFor( frsts_, frst, &found );
    if ( !found )
	return SPos();

    const TypeSet<int>& scnds = *scndsets_[spos.i];
    const auto lastj = scnds.size() - 1;
    spos.j = findIndexFor( scnds, scnd, &found );
    if ( found )
	return spos;

    if ( spos.j < 0 )
	spos.j = 0;
    else if ( spos.j > lastj )
	spos.j = scnds.size() - 1;
    else if ( spos.j < lastj )
    {
	auto ip = getIdxPair( spos );
	const auto posdiff1 = std::abs( ip.second - scnd );
	SPos spos2( spos.i, spos.j+1 );
	ip = getIdxPair( spos2 );
	const auto posdiff2 = std::abs( ip.second - scnd );
	if ( posdiff2 < posdiff1 )
	    spos = spos2;
    }

    return spos;
}


static HiddenParam<BinIDValueSet,char> is2ds(0);

BinIDValueSet::~BinIDValueSet()
{
    is2ds.removeParam( this );
}


BinIDValueSet& BinIDValueSet::operator =( const BinIDValueSet& oth )
{
    Pos::IdxPairValueSet::operator=( oth );
    setIs2D( oth.is2D() );
    return *this;
}


void BinIDValueSet::setStepout( int trcstepout, int trcstep )
{
    (void)trcstep;
    if ( isEmpty() || trcstepout<0 )
	return;

    for ( int ifirst=0; ifirst<frsts_.size(); ifirst++ )
    {
	const int frst = frsts_[ifirst];
	const Pos::GeomID geomid( frst );
	const Survey::Geometry2D& geom2d = Survey::GM().get2D( geomid );
	if  ( geom2d.isEmpty() )
	    continue;

	Array1DImpl<bool> needed( geom2d.size() );
	needed.setAll( false );

	const PosInfo::Line2DData& data = geom2d.data();
	const int sostep = data.trcNrRange().step;
	const TypeSet<int>& scnds = getScndSet( ifirst );
	for ( int iscnd=0; iscnd<scnds.size(); iscnd++ )
	{
	    const int centralnr = scnds[iscnd];
	    for ( int ioffs=-trcstepout; ioffs<=trcstepout; ioffs++ )
	    {
		const int arridx = data.indexOf( centralnr+ioffs*sostep );
		if ( needed.info().validPos(arridx) )
		    needed.set( arridx, true );
	    }
	}

	const od_uint64 arrsz = needed.totalSize();
	for ( od_uint64 idx=0; idx<arrsz; idx++ )
	{
	    if ( needed.get(idx) )
	    {
		const Pos::IdxPair ip( frst, data.positions()[idx].nr_ );
		addHorPosIfNeeded( ip );
	    }
	}
    }
}


void BinIDValueSet::setStepout( const IdxPair& stepout, const IdxPair& step )
{
    if ( isEmpty() ||
	 (!stepout.first && !stepout.second) ||
	 (!step.first && !step.second) )
	return;

    Interval<int> inlrg = inlRange();
    inlrg.widen( stepout.first * step.first );
    Interval<int> crlrg = crlRange();
    crlrg.widen( stepout.second * step.second );

    const StepInterval<int> inldef( inlrg.start, inlrg.stop, step.first );
    const StepInterval<int> crldef( crlrg.start, crlrg.stop, step.second );
    TrcKeySampling subsel;
    subsel.set( inldef, crldef );
    Array2DImpl<bool> needed( subsel.nrInl(), subsel.nrCrl() );
    needed.setAll( false );

    SPos spos;
    while ( next(spos) )
    {
	const Pos::IdxPair centralip = getIdxPair( spos );
	for ( int ifrstoffs=-stepout.first; ifrstoffs<=stepout.first;
					    ifrstoffs++ )
	{
	    const int first = centralip.first + ifrstoffs * step.first;
	    for ( int iscndoffs=-stepout.second; iscndoffs<=stepout.second;
		    iscndoffs++ )
	    {
		const int scnd = centralip.second
				+ iscndoffs * step.second;
		needed.set( subsel.inlIdx(first), subsel.crlIdx(scnd), true );
	    }
	}
    }

    TrcKeySamplingIterator it( subsel );
    TrcKey tk;
    while ( it.next(tk) )
    {
	const od_int64 curidx = subsel.globalIdx( tk );
	if ( needed.getData()[curidx] )
	    addHorPosIfNeeded( tk.binID() );
    };
}


void BinIDValueSet::addHorPosIfNeeded( const Pos::IdxPair& ip )
{
    SPos spos = find( ip );
    if ( !spos.isValid() )
    {
	spos = add( ip ); // just use an existing obj
	if ( !spos.isValid() )
	    { setEmpty(); return; }
    }
}


void BinIDValueSet::init()
{ is2ds.setParam( this, false ); }

void BinIDValueSet::setIs2D( bool yn )
{ is2ds.setParam( this, yn ); }

bool BinIDValueSet::is2D() const
{ return is2ds.getParam(this); }

