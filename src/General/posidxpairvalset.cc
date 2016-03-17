/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : 21-6-1996 / Mar 2016
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
#include "survgeom.h"
#include "varlenarray.h"
#include "od_iostream.h"

static const float cMaxDistFromGeom = 1000.f;


static inline void setToUdf( float* arr, int nvals )
{
    for ( int idx=0; idx<nvals; idx++ )
	Values::setUdf( arr[idx] );
}


static void initVals( Pos::IdxPairDataSet& ds, int spos_i, int spos_j )
{
    const Pos::IdxPairDataSet::SPos spos( spos_i, spos_j );
    const int nrvals = (int)(ds.objSize() / sizeof(float));
    float* vals = (float*)ds.getObj( spos );
    if ( vals )
	setToUdf( vals, nrvals );
    else
    {
	TypeSet<float> tsvals( nrvals, mUdf(float) );
	ds.set( spos, tsvals.arr() );
    }
}


Pos::IdxPairValueSet::IdxPairValueSet( int nv, bool ad )
	: data_(nv*sizeof(float),ad,true)
	, nrvals_(nv)
{
}


Pos::IdxPairValueSet& Pos::IdxPairValueSet::operator =(
				    const IdxPairValueSet& oth )
{
    if ( this != &oth )
    {
	data_ = oth.data_;
	const_cast<int&>( nrvals_ ) = oth.nrvals_;
    }
    return *this;
}


void Pos::IdxPairValueSet::copyStructureFrom( const IdxPairValueSet& oth )
{
    data_.copyStructureFrom( oth.data_ );
    const_cast<int&>(nrvals_) = oth.nrvals_;
}


bool Pos::IdxPairValueSet::getFrom( od_istream& strm, GeomID gid )
{
    setEmpty();
    if ( !setNrVals(0) )
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
	    setNrVals( nrvalsfound );
	    nextword = firstval;
	    coordindic = dist < cMaxDistFromGeom ? 1 : 0;
	}

	TrcKey tk;
	if ( coordindic == 1 )
	    tk = survgeom->nearestTrace( coord );
	else
	{
	    tk.setLineNr( (Pos::LineID)(coord.x + 0.5) );
	    tk.setTrcNr( (Pos::TraceID)(coord.y + 0.5) );
	}

	float* vals = getVals( add(tk.binID()) );
	if ( !vals )
	    continue;

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
	const IdxPair ip( getIdxPair(pos) );
	const float* vals = gtVals(pos);
	strm << ip.first << od_tab << ip.second;
	for ( int idx=0; idx<nrvals_; idx++ )
	    strm << od_tab << toString( vals[idx] );
	strm << od_newline;
    }
    strm.flush();
    return strm.isOK();
}


float* Pos::IdxPairValueSet::getVals( const SPos& spos )
{
    return !spos.isValid() ? 0 : gtVals( spos );
}


const float* Pos::IdxPairValueSet::getVals( const SPos& spos ) const
{
    return !spos.isValid() ? 0 : gtVals( spos );
}


float Pos::IdxPairValueSet::getVal( const SPos& spos, int valnr ) const
{
    if ( !spos.isValid() || valnr < 0 || valnr >= nrvals_ )
	return mUdf(float);

    return gtVals(spos)[valnr];
}


Interval<float> Pos::IdxPairValueSet::valRange( int valnr ) const
{
    Interval<float> ret( mUdf(float), mUdf(float) );
    if ( valnr >= nrvals_ || valnr < 0 || isEmpty() )
	return ret;

    SPos pos;
    while ( next(pos) )
    {
	const float val = gtVals(pos)[valnr];
	if ( !mIsUdf(val) )
	    { ret.start = ret.stop = val; break; }
    }
    while ( next(pos) )
    {
	const float val = gtVals(pos)[valnr];
	if ( !mIsUdf(val) )
	    ret.include( val, false );
    }

    return ret;
}


void Pos::IdxPairValueSet::get( const SPos& pos, IdxPair& ip, float* vs,
				 int maxnrvals ) const
{
    if ( maxnrvals < 0 || maxnrvals > nrvals_ )
	maxnrvals = nrvals_;

    ip = getIdxPair( pos );
    if ( !vs || maxnrvals == 0 )
	return;

    if ( pos.isValid() )
	OD::memCopy( vs, gtVals(pos), maxnrvals * sizeof(float) );
    else
	setToUdf( vs, maxnrvals );
}


Pos::IdxPairValueSet::SPos Pos::IdxPairValueSet::add( const Pos::IdxPair& ip,
						      const float* arr )
{
    SPos ret;
    if ( arr )
	ret = data_.add( ip, arr );
    else
    {
	TypeSet<float> vals( nrvals_, mUdf(float) );
	ret = data_.add( ip, vals.arr() );
    }
    return ret;
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


void Pos::IdxPairValueSet::add( const PosInfo::CubeData& cubedata )
{
    data_.add( cubedata, initVals );
}


void Pos::IdxPairValueSet::set( SPos spos, const float* vals )
{
    if ( !spos.isValid() || nrvals_ < 1 )
	return;

    data_.set( spos, vals );
    if ( !vals )
	setToUdf( gtVals(spos), nrvals_ );
}


void Pos::IdxPairValueSet::removeVal( int validx )
{
    if ( validx < 0 || validx >= nrvals_ )
	return;
    else if ( nrvals_ == 1 )
	{ setNrVals( 0 ); return; }

    data_.decrObjSize( sizeof(float), validx*sizeof(float) );
    const_cast<int&>(nrvals_)--;
}


bool Pos::IdxPairValueSet::insertVal( int validx )
{
    if ( validx < 0 || validx >= nrvals_ )
	return false;

    const_cast<int&>(nrvals_)++;
    const float udf = mUdf(float);
    return data_.incrObjSize( sizeof(float), validx*sizeof(float), &udf );
}


bool Pos::IdxPairValueSet::setNrVals( int newnrvals )
{
    const int nrdiff = newnrvals - nrvals_;

    if ( nrdiff < 0 )
	data_.decrObjSize( (-nrdiff)*sizeof(float) );
    else if ( nrdiff > 0 && !data_.incrObjSize(nrdiff*sizeof(float)) )
	return false;

    const_cast<int&>(nrvals_) = newnrvals;
    return true;
}


void Pos::IdxPairValueSet::extend( const Pos::IdxPairDelta& so,
				   const Pos::IdxPairStep& sos )
{
    data_.extend( so, sos, initVals );
}


void Pos::IdxPairValueSet::getColumn( int valnr, TypeSet<float>& vals,
				bool incudf ) const
{
    if ( valnr < 0 || valnr >= nrVals() ) return;
    SPos pos;
    while ( next(pos) )
    {
	const float* v = gtVals( pos );
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
	const float* v = gtVals( pos );
	if ( inside == rg.includes(v[valnr],true) )
	    poss += pos;
    }
    remove( poss );
}


Pos::IdxPairValueSet::SPos Pos::IdxPairValueSet::add( const PairVal& pv )
{
    return nrvals_ < 2 ? add(pv,pv.val()) : add(DataRow(pv));
}


Pos::IdxPairValueSet::SPos Pos::IdxPairValueSet::add( const Pos::IdxPair& ip,
								double v )
{
    return add( ip, mCast(float,v) );
}


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
    fms += nrvals_; fms += allowsDuplicateIdxPairs() ? "D" : "N";
    BufferString key; if ( ky && *ky ) { key = ky; key += ".Setup"; }
    iop.set( key, fms );

    SPos spos;
    IdxType prevfirst = mUdf( IdxType );
    while( next(spos) )
    {
	const IdxPair ip = getIdxPair( spos );

	if ( ip.first != prevfirst )
	{
	    if ( !mIsUdf(prevfirst) )
	    {
		if ( ky && *ky )
		    { key = ky; key += "."; }
		else
		    key.setEmpty();
		key += prevfirst;
		iop.set( key, fms );
	    }
	    fms.setEmpty(); fms += ip.first; prevfirst = ip.first;
	}

	fms += ip.second;
	if ( nrvals_ > 0 )
	{
	    const float* v = gtVals( spos );
	    for ( int idx=0; idx<nrvals_; idx++ )
		fms += v[idx];
	}
    }
}


void Pos::IdxPairValueSet::usePar( const IOPar& iop, const char* iopky )
{
    FileMultiString fms;
    BufferString key;
    const bool haveiopky = iopky && *iopky;
    if ( haveiopky )
	key.set( iopky ).add( ".Setup" );
    const char* res = iop.find( key );
    if ( res && *res )
    {
	setEmpty();
	fms = res;
	setNrVals( fms.getIValue(0) );
	allowDuplicateIdxPairs( *fms[1] == 'D' );
    }

    DataRow dr( 0, 0, nrvals_ );
    for ( int ifrst=0; ; ifrst++ )
    {
	if ( haveiopky )
	    key.set( iopky ).add( "." );
	else
	    key.setEmpty();
	key += ifrst;
	res = iop.find( key );
	if ( !res )
	    return;
	if ( !*res )
	    continue;

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
