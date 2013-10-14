/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 21-6-1996
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "posidxpairvalset.h"
#include "posidxpairvalue.h"
#include "iopar.h"
#include "separstr.h"
#include "idxable.h"
#include "posinfo.h"
#include "sorting.h"
#include "strmoper.h"
#include "statrand.h"
#include "varlenarray.h"
#include <iostream>
#include <vector>

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
						const Pos::IdxPairValueSet& vs )
{
    if ( &vs == this ) return *this;

    setEmpty(); copyStructureFrom( vs );

    for ( int iinl=0; iinl<vs.frsts_.size(); iinl++ )
    {
	frsts_ += vs.frsts_[iinl];
	scndsets_ += new TypeSet<int>( *vs.scndsets_[iinl] );
	valsets_ += new TypeSet<float>( *vs.valsets_[iinl] );
    }

    return *this;
}


void Pos::IdxPairValueSet::setEmpty()
{
    frsts_.erase();
    deepErase( scndsets_ );
    deepErase( valsets_ );
}


bool Pos::IdxPairValueSet::append( const Pos::IdxPairValueSet& vs )
{
    Pos pos; IdxPair idxpair;
    if ( nrvals_ <= vs.nrvals_ )
    {
	while ( vs.next(pos,!vs.allowdup_) )
	{
	    vs.get( pos, idxpair );
	    const Pos newpos = add( idxpair, nrvals_ ? vs.getVals( pos ) : 0 );
	    if ( !newpos.valid() )
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
	    memcpy( insvals, vs.getVals( pos ), vs.nrvals_ * sizeof(float) );
	    const Pos newpos = add( idxpair, insvals );
	    if ( !newpos.valid() )
		return false;
	}
    }

    return true;
}


void Pos::IdxPairValueSet::remove( const Pos::IdxPairValueSet& removepairs ) 
{ 
    Pos::IdxPairValueSet::Pos pos; 
	 
    while ( removepairs.next(pos, true ) ) 
    { 
	const IdxPair pair = removepairs.getIdxPair( pos ); 
	Pos::IdxPairValueSet::Pos removepos = findOccurrence( pair ); 
					 
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
    if ( maxsz == 0 )
	{ setEmpty(); return; }

    mGetIdxArr(od_int64,idxs,orgsz);
    if ( !idxs )
	{ setEmpty(); return; }

    const bool buildnew = ((od_int64)maxsz) < (orgsz / ((od_int64)2));
    Stats::randGen().subselect( idxs, orgsz, maxsz );
    TypeSet<Pos> poss;
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
	Pos::IdxPairValueSet newvs( nrvals_, allowdup_ );
	DataRow dr;
	for ( od_int64 idx=0; idx<poss.size(); idx++ )
	{
	    get( poss[mCast(int,idx)], dr );
	    newvs.add( dr );
	}
	*this = newvs;
    }
}


bool Pos::IdxPairValueSet::getFrom( od_istream& strm )
{
    setEmpty();
    if ( !setNrVals( 0, false ) )
	return false;

    char linebuf[4096], valbuf[1024];
    Coord crd; BinID bid;
    bool first_time = true;
    while ( strm.getline( linebuf, 4096 ) )
    {
	char* firstchar = linebuf;
	mSkipBlanks( firstchar );
	if ( *firstchar == '"' )
	{
	    char* ptr = strchr( firstchar+1, '"' );
	    if ( !ptr ) continue;
	    firstchar = ptr+1;
	}
	mSkipBlanks( firstchar );
	if ( !*firstchar || (*firstchar != '-' && !isdigit(*firstchar) ) )
	    continue;

	const char* nextword = getNextWord( firstchar, valbuf );
	bid.inl = toInt( valbuf );
	mSkipBlanks( nextword ); if ( !*nextword ) continue;
	nextword = getNextWord( nextword, valbuf );
	bid.crl = toInt( valbuf );

	if ( first_time )
	{
	    first_time = false;
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
	}

	float* vals = getVals( add(bid) );
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
    Pos pos;
    while ( next(pos) )
    {
	const IdxPair ip( getInl(pos), getCrl(pos) );
	const float* vals = getVals(pos);
	strm << ip.inl() << od_tab << ip.crl();
	char str[255];
	for ( int idx=0; idx<nrvals_; idx++ )
	{
	    getStringFromFloat( 0, vals[idx], str );
	    strm << od_tab << str;
	}
	strm << od_newline;
    }
    strm.flush();
    return strm.good();
}


int Pos::IdxPairValueSet::nrSecond( Pos::IdxPairValueSet::IdxType inl ) const
{
    const int inlidx = frsts_.indexOf( inl );
    return inlidx<0 ? 0 : getCrlSet(inlidx).size();
}


Interval<int> Pos::IdxPairValueSet::firstRange() const
{
    Interval<int> ret( mUdf(int), mUdf(int) );

    bool first = true;
    for ( int iinl=0; iinl<frsts_.size(); iinl++ )
    {
	if ( first )
	    { ret.start = ret.stop = frsts_[iinl]; first = false; }
	else
	    ret.include( frsts_[iinl], false );
    }
    return ret;
}


Interval<int> Pos::IdxPairValueSet::secondRange( int inl ) const
{
    Interval<int> ret( mUdf(int), mUdf(int) );
    if ( frsts_.isEmpty() ) return ret;

    const int inlidx = frsts_.indexOf( inl );
    if ( inlidx >= 0 )
    {
	const TypeSet<int>& crlset = getCrlSet(inlidx);
	const int nrcrl = crlset.size();
	if ( nrcrl>=1 )
	    ret.start = ret.stop = crlset[0];
	if ( nrcrl>1 )
	    ret.include( crlset[nrcrl-1], false );

    }
    else
    {
	bool found = false;
	for ( int idx=0; idx<frsts_.size(); idx++ )
	{
	    const TypeSet<int>& crlset = getCrlSet(idx);
	    const int nrcrl = crlset.size();
	    if ( nrcrl>=1 )
	    {
		if ( found )
		    ret.include( crlset[0], false );
		else
		{
		    ret.start = ret.stop = crlset[0];
		    found = true;
		}
	    }

	    if ( nrcrl>1 )
		ret.include( crlset[nrcrl-1], false );
	}
    }

    return ret;
}


Interval<float> Pos::IdxPairValueSet::valRange( int valnr ) const
{
    Interval<float> ret( mUdf(float), mUdf(float) );
    if ( valnr >= nrvals_ || valnr < 0 || isEmpty() )
	return ret;

    Pos pos;
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


void Pos::IdxPairValueSet::copyStructureFrom( const Pos::IdxPairValueSet& vs )
{
    setEmpty();
    const_cast<int&>(nrvals_) = vs.nrvals_;
    allowdup_ = vs.allowdup_;
}


Pos::IdxPairValueSet::Pos Pos::IdxPairValueSet::findOccurrence(
					const IdxPair& ip, int occ ) const
{
    bool found; int idx = findIndexFor(frsts_,ip.inl(),&found);
    Pos pos( found ? idx : -1, -1 );
    if ( pos.i >= 0 )
    {
	const TypeSet<int>& crls = getCrlSet(pos);
	idx = findIndexFor(crls,ip.crl(),&found);
	pos.j = found ? idx : -1;
	if ( found )
	{
	    pos.j = idx;
	    while ( pos.j && crls[pos.j-1] == ip.crl() )
		pos.j--;
	}
    }

    while ( occ > 0 && next(pos) )
	occ--;
    return pos;
}


bool Pos::IdxPairValueSet::next( Pos::IdxPairValueSet::Pos& pos,
					bool skip_dup ) const
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

    const TypeSet<int>& crls = getCrlSet(pos);
    if ( pos.j > crls.size()-2 )
    {
	pos.j = 0;
	pos.i++;
	if ( pos.i >= frsts_.size() )
	    pos.i = pos.j = -1;
	return pos.i >= 0;
    }

    pos.j++;
    if ( skip_dup && crls[pos.j] == crls[pos.j-1] )
	return next( pos, true );

    return true;
}


bool Pos::IdxPairValueSet::prev( Pos::IdxPairValueSet::Pos& pos,
				 bool skip_dup ) const
{
    if ( !pos.valid() )
	return false;
    if ( pos.i == 0 && pos.j == 0)
	{ pos.i = pos.j = -1; return false; }

    int curcrl = getCrl(pos);
    if ( pos.j )
	pos.j--;
    else
    {
	pos.i--;
	pos.j = getCrlSet(pos).size() - 1;
    }

    if ( !skip_dup ) return true;

    while ( getCrl(pos) == curcrl )
	return prev( pos, true );

    return true;
}


bool Pos::IdxPairValueSet::valid( const IdxPair& ip ) const
{
    Pos pos = findFirst( ip );
    return pos.valid()
	&& frsts_.isPresent(ip.inl())
	&& getCrlSet(pos).size() > pos.j;
}


void Pos::IdxPairValueSet::get( const Pos& pos, IdxPair& ip, float* vs,
       			 int maxnrvals ) const
{
    if ( maxnrvals < 0 || maxnrvals > nrvals_ ) maxnrvals = nrvals_;

    if ( !pos.valid() )
	{ ip.inl() = ip.crl() = 0; }
    else
    {
	ip.inl() = getInl(pos); ip.crl() = getCrl(pos);
	if ( vs && maxnrvals )
	{
	    memcpy( vs, getVals(pos), maxnrvals * sizeof(float) );
	    return;
	}
    }

    if ( vs ) 
	setToUdf(vs,maxnrvals);
}


Pos::IdxPair Pos::IdxPairValueSet::getIdxPair( const Pos& pos ) const
{
    return pos.valid() ? IdxPair(getInl(pos),getCrl(pos)) : IdxPair(0,0);
}


Pos::IdxPairValueSet::Pos Pos::IdxPairValueSet::getPos( od_int64 glidx ) const
{
    od_int64 firstidx = 0; Pos pos;
    for ( pos.i=0; pos.i<frsts_.size(); pos.i++ )
    {
	const TypeSet<int>& crls = getCrlSet(pos);
	if ( firstidx + crls.size() > glidx )
	{
	    pos.j = (int)(glidx - firstidx);
	    return pos;
	}
	firstidx += crls.size();
    }

    return Pos(-1,-1);
}


Pos::IdxPairValueSet::Pos Pos::IdxPairValueSet::add( const Pos::IdxPair& ip,
							const float* arr )
{
    Pos pos( findFirst(ip) );
    if ( pos.i < 0 )
    {
	pos.i = findIndexFor(frsts_,ip.inl()) + 1;
	if ( pos.i > frsts_.size()-1 )
	{
	    frsts_ += ip.inl();
	    scndsets_ += new TypeSet<int>;
	    valsets_ += new TypeSet<float>;
	    pos.i = frsts_.size() - 1;
	}
	else
	{
	    frsts_.insert( pos.i, ip.inl() );
	    scndsets_.insertAt( new TypeSet<int>, pos.i );
	    valsets_.insertAt( new TypeSet<float>, pos.i );
	}
    }

    if ( pos.j < 0 || allowdup_ )
	addNew( pos, ip.crl(), arr );

    return pos;
}


void Pos::IdxPairValueSet::addNew( Pos::IdxPairValueSet::Pos& pos, int crl,
					const float* arr )
{
    TypeSet<int>& crls = getCrlSet(pos);

    if ( pos.j < 0 )
	pos.j = findIndexFor(crls,crl) + 1;
    else
    {
	pos.j++;
	while ( pos.j < crls.size() && crls[pos.j] == crl )
	    pos.j++;
    }


    TypeSet<float>& vals = getValSet(pos);
    if ( pos.j > crls.size() - 1 )
    {
	crls += crl;
	for ( int idx=0; idx<nrvals_; idx++ )
	    vals += arr ? arr[idx] : mUdf(float);
    }
    else
    {
	crls.insert( pos.j, crl );
	for ( int idx=nrvals_-1; idx>=0; idx-- )
	    vals.insert( pos.j*nrvals_, arr ? arr[idx] : mUdf(float) );
    }
}


Pos::IdxPairValueSet::Pos Pos::IdxPairValueSet::add( const DataRow& dr )
{
    if ( dr.size() >= nrvals_ )
	return add( dr, vals.values() );

    DataRow locdr( 0, 0, nrvals_ );
    for ( int idx=0; idx<dr.size(); idx++ )
	locdr.value(idx) = dr.value(idx);
    for ( int idx=dr.size(); idx<nrvals_; idx++ )
	DataRow::setUdf( locdr.value(idx) );
    return add( dr, locdr.values() );
}


namespace Pos
{
class IdxPairValueSetFromCubeData : public ::ParallelTask
{
public:

IdxPairValueSetFromCubeData( Pos::IdxPairValueSet& vs,
			       const PosInfo::CubeData& cubedata )
    : vs_( vs )
    , cubedata_( cubedata )
{
    //Add first pos on each inline so all inlines are in, thus
    //threadsafe to add things as long as each inline is separate
    for ( int idx=0; idx<cubedata.size(); idx++ )
    {
	const PosInfo::LineData& line = *cubedata_[idx];
	const int inl = line.linenr_;
	if ( line.segments_.size() )
	    vs.add( IdxPair(inl,line.segments_[0].start) );
    }
}

od_int64 nrIterations() const { return cubedata_.size(); }

bool doWork( od_int64 start, od_int64 stop, int )
{
    for ( int idx=mCast(int,start); idx<=stop; idx++ )
    {
	const PosInfo::LineData& line = *cubedata_[idx];
	const int inl = line.linenr_;
	for ( int idy=0; idy<line.segments_.size(); idy++ )
	{
	    StepInterval<int> crls = line.segments_[idy];
	    if ( !idy )
		crls.start += crls.step; //We added first crl in constructor
	    for ( int crl=crls.start; crl<=crls.stop; crl+=crls.step )
	    {
		vs_.add( IdxPair(inl,crl) );
	    }
	}
    }

    return true;
}

    IdxPairValueSet&	vs_;
    PosInfo::CubeData	cubedata_;

};


void Pos::IdxPairValueSet::add( const PosInfo::CubeData& cubedata )
{
    Pos::IdxPairValueSetFromCubeData task( *this, cubedata );
    task.execute();
}


void Pos::IdxPairValueSet::set( Pos::IdxPairValueSet::Pos pos,
				const float* vals )
{
    if ( !pos.valid() || !nrvals_ ) return;

    if ( vals )
	memcpy( getVals(pos), vals, nrvals_*sizeof(float) );
    else
	setToUdf( getVals(pos), nrvals_ );
}


int Pos::IdxPairValueSet::nrPos( int inlidx ) const
{
    return inlidx < 0 || inlidx >= frsts_.size() ? 0 : getCrlSet(inlidx).size();
}


od_int64 Pos::IdxPairValueSet::totalSize() const
{
    od_int64 nr = 0;
    for ( int idx=0; idx<frsts_.size(); idx++ )
	nr += getCrlSet(idx).size();
    return nr;
}


bool Pos::IdxPairValueSet::hasFirst( Pos::IdxPairValueSet::IdxType inl ) const
{
    return frsts_.isPresent(inl);
}


bool Pos::IdxPairValueSet::hasSecond( Pos::IdxPairValueSet::IdxType crl ) const
{
    for ( int iinl=0; iinl<frsts_.size(); iinl++ )
    {
	const TypeSet<int>& crls = getCrlSet(iinl);
	for ( int icrl=0; icrl<crls.size(); icrl++ )
	    if ( crls[icrl] == crl ) return true;
    }
    return false;
}


Pos::IdxPair Pos::IdxPairValueSet::firstIdxPair() const
{
    Pos pos; next(pos);
    IdxPair ip( IdxPair::udf() ); get(pos,ip);
    return ip;
}


void Pos::IdxPairValueSet::remove( const Pos& pos )
{
    if ( pos.i < 0 || pos.i >= frsts_.size() )
	return;

    TypeSet<int>& crls = getCrlSet(pos);
    if ( pos.j < 0 || pos.j >= crls.size() )
	return;

    crls.removeSingle( pos.j );
    if ( crls.size() )
    {
	if ( nrvals_ )
	    getValSet(pos).removeRange( pos.j*nrvals_, (pos.j+1)*nrvals_ - 1 );
    }
    else
    {
	removeLine( pos.i );
    }
}


void Pos::IdxPairValueSet::remove(
			const TypeSet<Pos::IdxPairValueSet::Pos>& poss )
{
    if ( poss.size() < 1 )
	return;
    else if ( poss.size() == 1 )
	{ remove( poss[0] ); return; }

    Pos::IdxPairValueSet rmvs( 0, false );
    for ( int idx=0; idx<poss.size(); idx++ )
	rmvs.add( IdxPair(poss[idx].i,poss[idx].j) );

    Pos::IdxPairValueSet vs( *this );
    setEmpty();

    Pos pos; IdxPair ip;
    while ( vs.next(pos) )
    {
	if ( !rmvs.valid(IdxPair(pos.i,pos.j)) )
	{
	    vs.get( pos, ip );
	    add( ip, vs.getVals(pos) );
	}
    }
}


void Pos::IdxPairValueSet::removeDuplicateBids()
{
    if ( isEmpty() ) return;

    Pos pos; next(pos,false);
    IdxPair previp; get( pos, previp );
    TypeSet<Pos::IdxPairValueSet::Pos> poss;
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

    for ( int iinl=0; iinl<frsts_.size(); iinl++ )
    {
	TypeSet<float>& vals = getValSet(iinl);
	TypeSet<int>& crls = getCrlSet(iinl);
	for ( int icrl=crls.size()-1; icrl>=0; icrl-- )
	    vals.removeSingle( nrvals_*icrl+validx );
    }
    const_cast<int&>(nrvals_)--;
}

bool Pos::IdxPairValueSet::insertVal( int validx )
{
    if ( validx < 0 || validx >= nrvals_ )
	return false;
    
    const int oldnrvals = nrvals_;
    const_cast<int&>( nrvals_ ) = oldnrvals+1;

    for ( int iinl=0; iinl<frsts_.size(); iinl++ )
    {
	const int nrcrl = getCrlSet(iinl).size();
	TypeSet<float>* oldvals = valsets_[iinl];
	mDeclareAndTryAlloc( TypeSet<float>*, newvals,
			     TypeSet<float>( nrcrl*nrvals_, mUdf(float) ) );
	if ( !newvals )
	    return false;
	valsets_.replace( iinl, newvals );
	float* oldarr = oldvals->arr();
	float* newarr = newvals->arr();
	for ( int icrl=0; icrl<nrcrl; icrl++ )
	{
	    memcpy( newarr+(icrl*nrvals_), oldarr+(icrl*oldnrvals),
		    (validx) * sizeof(float) );
	    memcpy( newarr+(icrl*nrvals_+validx+1),
		    oldarr+(icrl*oldnrvals+validx+1),
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

    for ( int iinl=0; iinl<frsts_.size(); iinl++ )
    {
	const int nrcrl = getCrlSet(iinl).size();
	if ( nrvals_ == 0 )
	    getValSet(iinl).erase();
	else if ( oldnrvals == 0 )
	    getValSet(iinl).setSize( nrcrl * nrvals_, mUdf(float) );
	else
	{
	    TypeSet<float>* oldvals = valsets_[iinl];
	    mDeclareAndTryAlloc( TypeSet<float>*, newvals,
				 TypeSet<float>( nrcrl*nrvals_, mUdf(float) ) );
	    if ( !newvals )
		return false;
	    valsets_.replace( iinl, newvals );
	    if ( keepdata )
	    {
		float* oldarr = oldvals->arr();
		float* newarr = newvals->arr();
		const int cpsz = (oldnrvals > nrvals_ ? nrvals_ : oldnrvals)
		    		   * sizeof( float );
		for ( int icrl=0; icrl<nrcrl; icrl++ )
		    memcpy( newarr+icrl*nrvals_, oldarr+icrl*oldnrvals, cpsz );
	    }
	    delete oldvals;
	}
    }

    return true;
}


void Pos::IdxPairValueSet::sortDuplicateBids( int valnr, bool asc )
{
    if ( valnr >= nrvals_ || !allowdup_ ) return;

    for ( int iinl=0; iinl<frsts_.size(); iinl++ )
    {
	TypeSet<int>& crls = getCrlSet(iinl);
	TypeSet<float>& vals = getValSet(iinl);
	for ( int icrl=1; icrl<crls.size(); icrl++ )
	{
	    int curcrl = crls[icrl];
	    int firstdup = icrl - 1;
	    if ( crls[icrl-1] == curcrl )
	    {
		for ( int idup=icrl+1; idup<crls.size(); idup++ )
		{
		    if ( crls[idup] == curcrl )
			icrl = idup;
		    else
			break;
		}
		sortPart( crls, vals, valnr, firstdup, icrl, asc );
	    }
	}
    }
}


void Pos::IdxPairValueSet::sortPart( TypeSet<int>& crls, TypeSet<float>& vals,
			      int valnr, int firstidx, int lastidx, bool asc )
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
	    Swap( crls[idx], crls[ idxs[idx] ] );
	    for ( int iv=0; iv<nrvals_; iv++ )
		Swap( vals[ idx*nrvals_ + iv], vals[ idxs[idx]*nrvals_ + iv ] );
	}
    }

    delete [] vs; delete [] idxs;
}


void Pos::IdxPairValueSet::extend( const Pos::IdxPair& so,
				   const Pos::IdxPair& sos )
{
    if ( (!so.inl() && !so.crl()) || (!sos.inl() && !sos.crl()) ) return;

    Pos::IdxPairValueSet vs( *this );

    const bool kpdup = allowdup_;
    allowdup_ = false;

    Pos pos; IdxPair ip;
    float* vals = nrvals_ ? new float [nrvals_] : 0;
    while ( vs.next(pos) )
    {
	vs.get( pos, ip, vals );
	const IdxPair centralip( ip );
	for ( int iinl=-so.inl(); iinl<=so.inl(); iinl++ )
	{
	    ip.inl() = centralip.inl() + iinl * sos.inl();
	    for ( int icrl=-so.crl(); icrl<=so.crl(); icrl++ )
	    {
		if ( !iinl && !icrl )
		    continue;
		ip.crl() = centralip.crl() + icrl * sos.crl();
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
    Pos pos;
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
    TypeSet<Pos> poss; Pos pos;
    while ( next(pos) )
    {
	const float* v = getVals( pos );
	if ( inside == rg.includes(v[valnr],true) )
	    poss += pos;
    }
    remove( poss );
}


void Pos::IdxPairValueSet::remove( const HorSampling& hrg, bool removeinside )
{
    const StepInterval<int> inlrg = hrg.inlRange();
    const StepInterval<int> crlrg = hrg.crlRange();

    for ( int idx=frsts_.size()-1; idx>=0; idx-- )
    {
	const int inl = frsts_[idx];
	bool isin = inlrg.includes(inl,false) && inlrg.snap( inl )==inl;
	if ( isin==removeinside )
	    removeLine( idx );
	else
	{
	    TypeSet<int>& crls = *scndsets_[idx];
	    TypeSet<float>& vals = *valsets_[idx];
	    for ( int idy=crls.size()-1; idy>=0; idy-- )
	    {
		const int crl = crls[idy];
		isin = crlrg.includes(crl,false) && crlrg.snap( crl )==crl;
		if ( isin==removeinside )
		{
		    crls.removeSingle( idy );
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


Pos::IdxPairValueSet::Pos Pos::IdxPairValueSet::add(
			const Pos::IdxPairValueSet::PairVal& pv )
{
    return nrvals_ < 2 ? add(pv,pv.val()) : add(DataRow(pv));
}


Pos::IdxPairValueSet::Pos Pos::IdxPairValueSet::add( const Pos::IdxPair& ip,
								double v )
{ return add( ip, mCast(float,v) ); }


Pos::IdxPairValueSet::Pos Pos::IdxPairValueSet::add( const Pos::IdxPair& ip,
							float v )
{
    if ( nrvals_ < 2 )
	return add( ip, nrvals_ == 1 ? &v : 0 );

    DataRow dr( ip, 1 );
    dr.value(0) = v;
    return add( dr );
}


Pos::IdxPairValueSet::Pos Pos::IdxPairValueSet::add( const Pos::IdxPair& ip,
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


Pos::IdxPairValueSet::Pos Pos::IdxPairValueSet::add( const Pos::IdxPair& ip,
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


void Pos::IdxPairValueSet::get( const Pos& pos, DataRow& dr ) const
{
    dr.setSize( nrvals_ );
    get( pos, dr, dr.values() );
}


void Pos::IdxPairValueSet::get( const Pos& pos, PairVal& pv ) const
{
    if ( nrvals_ < 2 )
	get( pos, pv, &pv.val() );
    else
    {
	DataRow dr; get( pos, dr );
	pv.set( dr ); pv.set( dr.value(0) );
    }
}


void Pos::IdxPairValueSet::get( const Pos::IdxPairValueSet::Pos& pos,
			 Pos::IdxPair& ip, float& v ) const
{
    if ( nrvals_ < 2 )
	get( pos, ip, &v );
    else
    {
	DataRow dr; get( pos, dr );
	ip = dr; v = dr.value(0);
    }
}


void Pos::IdxPairValueSet::get( const Pos::IdxPairValueSet::Pos& pos,
				Pos::IdxPair& ip, float& v1, float& v2 ) const
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


void Pos::IdxPairValueSet::get( const Pos::IdxPairValueSet::Pos& pos,
				Pos::IdxPair& ip, TypeSet<float>& vals,
				int maxnrvals ) const
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


void Pos::IdxPairValueSet::set( const Pos::IdxPairValueSet::Pos& pos, float v )
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


void Pos::IdxPairValueSet::set( const Pos::IdxPairValueSet::Pos& pos,
				float v1, float v2 )
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


void Pos::IdxPairValueSet::set( const Pos::IdxPairValueSet::Pos& pos,
				const TypeSet<float>& v )
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

    for ( int iinl=0; iinl<frsts_.size(); iinl++ )
    {
	fms = ""; fms += frsts_[iinl];
	const TypeSet<int>& crls = getCrlSet(iinl);
	const TypeSet<float>& vals = getValSet(iinl);
	for ( int icrl=0; icrl<crls.size(); icrl++ )
	{
	    fms += crls[icrl];
	    if ( nrvals_ )
	    {
		const float* v = vals.arr() + icrl*nrvals_;
		char str[255];
		for ( int idx=0; idx<nrvals_; idx++ )
		{
		    getStringFromFloat( 0, v[idx], str );
		    fms += str;
		}
	    }
	}
	if ( ky && *ky )
	    { key = ky; key += "."; }
	else
	    key = "";
	key += iinl;
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
	setNrVals( toInt(fms[0]), false );
	allowdup_ = *fms[1] == 'D';
    }

    for ( int iinl=0; ; iinl++ )
    {
	if ( ky && *ky )
	    { key = ky; key += "."; }
	else
	    key = "";
	key += iinl;
	res = iop.find( key );
	if ( !res ) return;
	if ( !*res ) continue;

	fms = res;
	dr.inl() = toInt( fms[0] );
	int nrpos = (fms.size() - 1) / (nrvals_ + 1);
	for ( int icrl=0; icrl<nrpos; icrl++ )
	{
	    int fmsidx = 1 + icrl * (nrvals_ + 1);
	    dr.crl() = toInt( fms[fmsidx] );
	    fmsidx++;
	    for ( int ival=0; ival<nrvals_; ival++ )
		dr.value(ival) = toFloat( fms[fmsidx+ival] );
	    add( dr );
	}
    }
}


bool Pos::IdxPairValueSet::haveDataRow( const DataRow& dr ) const
{
    Pos pos = findFirst( dr );
    bool found = false;
    IdxPair tmpip;
    while ( !found && pos.valid() )
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
    Pos::IdxPairValueSet::Pos pos;
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
    IdxPairValueSet::Pos pos;
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

	if ( !pos.valid() )
	    break;

	previp = ip;
    }

    return nrdupips;
}
