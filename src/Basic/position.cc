/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-6-1996
-*/

static const char* rcsID = "$Id: position.cc,v 1.36 2004-07-16 15:35:25 bert Exp $";

#include "binidvalset.h"
#include "iopar.h"
#include "separstr.h"
#include "finding.h"
#include "sorting.h"
#include <math.h>
#include <string.h>


float BinIDValue::compareepsilon = 1e-4;
float BinIDValues::udf = mUndefValue;
#define mSetUdf(arr,nvals) \
    for ( int idx=0; idx<nvals; idx++ ) \
	arr[idx] = mUndefValue


double Coord::distance( const Coord& coord ) const
{
    double diffx = coord.x - x;
    double diffy = coord.y - y;
    return diffx || diffy ? sqrt( diffx*diffx + diffy*diffy ) : 0;
}


void Coord::fill( char* str ) const
{
    if ( !str ) return;
    strcpy( str, "(" ); strcat( str, getStringFromDouble(0,x) );
    strcat( str, "," ); strcat( str, getStringFromDouble(0,y) );
    strcat( str, ")" );
}


bool Coord::use( const char* str )
{
    if ( !str || !*str ) return false;
    static char buf[80];

    strcpy( buf, *str == '(' ? str+1 : str );
    char* ptr = strchr( buf, ',' );
    if ( !ptr ) return false;

    *ptr++ = '\0';
    const int len = strlen(ptr);
    if ( len && ptr[len-1] == ')' ) ptr[len-1] = '\0';

    x = atof( buf );
    y = atof( ptr );
    return true;
}


bool getDirectionStr( const Coord& coord, BufferString& res )
{
    if ( mIsZero(coord.x,mDefEps) && mIsZero(coord.y,mDefEps) )
	return false;

    const double len = sqrt(coord.x*coord.x+coord.y*coord.y);
    const double x = coord.x/len;
    const double y = coord.y/len;

    res = "";
    if ( y>0.5 )
	res += "north";
    else if ( y<-0.5 )
	res += "south";

    if ( x>0.5 )
	res += "east";
    else if ( x<-0.5 )
	res += "west";

    return true;
}


double Coord3::abs() const
{
    return sqrt( x*x + y*y + z*z );
}


void Coord3::fill(char* str, const char* start,
		     const char* space, const char* end) const
{
    strcpy( str, start );
    strcat( str, getStringFromDouble(0,x) ); strcat(str,space);
    strcat( str, getStringFromDouble(0,y) ); strcat(str,space);
    strcat( str, getStringFromDouble(0,z) ); strcat(str,space);
    strcat( str, end );
}


bool Coord3::use(const char* str)
{
    if ( !str ) return false;
    const char* endptr=str+strlen(str);

    while ( !isdigit(*str) && *str!='+' && *str!='-' && str!=endptr )
	str++;

    char* numendptr;
    x = strtod( str, &numendptr );
    if ( str==numendptr ) return false;

    str = numendptr;
    while ( !isdigit(*str) && *str!='+' && *str!='-' && str!=endptr )
	str++;
    y = strtod( str, &numendptr );
    if ( str==numendptr ) return false;

    str = numendptr;
    while ( !isdigit(*str) && *str!='+' && *str!='-' && str!=endptr )
	str++;
    z = strtod( str, &numendptr );
    if ( str==numendptr ) return false;

    return true;
}


double Coord3::distance( const Coord3& b ) const
{
    double dx = x-b.x, dy = y-b.y, dz = z-b.z;
    return sqrt( dx*dx + dy*dy + dz*dz );
}


void BinID::fill( char* str ) const
{
    if ( !str ) return;
    sprintf( str, "%d/%d", inl, crl );
}


bool BinID::use( const char* str )
{
    if ( !str || !*str ) return false;

    static char buf[80];
    strcpy( buf, str );
    char* ptr = strchr( buf, '/' );
    if ( !ptr ) return false;
    *ptr++ = '\0';
    inl = atoi( buf );
    crl = atoi( ptr );
    return true;
}


BinID BinID::operator+( const BinID& bi ) const
{
    BinID res = *this; 
    res.inl += bi.inl; res.crl += bi.crl;
    return res; 
}


BinID BinID::operator-( const BinID& bi ) const
{
    BinID res = *this; 
    res.inl -= bi.inl; res.crl -= bi.crl;
    return res; 
}


BinIDValue::BinIDValue( const BinIDValues& bvs, int nr )
    	: binid(bvs.binid)
    	, value(bvs.value(nr))
{
}


BinIDValues& BinIDValues::operator =( const BinIDValues& bvs )
{
    if ( &bvs != this )
    {
	binid = bvs.binid;
	setSize( bvs.sz );
	if ( vals )
	    memcpy( vals, bvs.vals, sz * sizeof(float) );
    }
    return *this;
}


bool BinIDValues::operator ==( const BinIDValues& bvs ) const
{
    if ( binid != bvs.binid || sz != bvs.sz )
	return false;

    for ( int idx=0; idx<sz; idx++ )
	if ( !mIsEqual(vals[idx],bvs.vals[idx],BinIDValue::compareepsilon) )
	    return false;

    return true;
}


void BinIDValues::setSize( int newsz, bool kpvals )
{
    if ( newsz == sz ) return;

    if ( newsz < 1 )
	{ delete [] vals; vals = 0; sz = 0; }
    else if ( !kpvals )
    {
	delete [] vals; vals = new float [newsz];
	sz = vals ? newsz : 0;
	return;
    }
    else
    {
	float* oldvals = vals;
	vals = new float [newsz];
	int oldsz = sz;
	sz = vals ? newsz : 0;
	if ( sz )
	    memcpy( vals, oldvals, (oldsz > sz ? sz : oldsz) * sizeof(float) );
	delete [] oldvals;
    }
}


void BinIDValues::setVals( const float* vs )
{
    if ( sz ) memcpy( vals, vs, sz * sizeof(float) );
}


static int findIndexFor( const TypeSet<int>& nrs, int nr, bool* found = 0 )
{
    int ret;
    bool fnd = findPos( nrs.arr(), nrs.size(), nr, -1, ret );
    if ( found ) *found = fnd;
    return ret;
}


BinIDValueSet::BinIDValueSet( int nv, bool ad )
	: nrvals(nv)
	, allowdup(ad)
{
}


BinIDValueSet::BinIDValueSet( const BinIDValueSet& s )
    	: nrvals(0)
{
    *this = s;
}


BinIDValueSet::~BinIDValueSet()
{
    empty();
}


BinIDValueSet& BinIDValueSet::operator =( const BinIDValueSet& bvs )
{
    if ( &bvs == this ) return *this;

    copyStructureFrom( bvs );

    for ( int iinl=0; iinl<bvs.inls.size(); iinl++ )
    {
	const TypeSet<float*>& bvsvals = *bvs.valsets[iinl];

	inls += bvs.inls[iinl];
	crlsets += new TypeSet<int>( *bvs.crlsets[iinl] );
	valsets += new TypeSet<float*>( bvsvals );

	TypeSet<float*>& vals = *valsets[iinl];
	for ( int icrl=0; icrl<bvsvals.size(); icrl++ )
	{
	    vals[icrl] = new float [nrvals];
	    memcpy( vals[icrl], bvsvals[icrl], nrvals * sizeof(float) );
	}
    }

    return *this;
}


void BinIDValueSet::empty()
{
    for ( int iinl=0; iinl<inls.size(); iinl++ )
    {
	TypeSet<float*>& vals = *valsets[iinl];
	for ( int icrl=0; icrl<vals.size(); icrl++ )
	    delete [] vals[icrl];
    }
    inls.erase();
    deepErase( crlsets );
    deepErase( valsets );
}


void BinIDValueSet::append( const BinIDValueSet& bvs )
{
    Pos pos; BinIDValues bivs;
    while ( bvs.next(pos,!bvs.allowdup) )
    {
	bvs.get( pos, bivs );
	add( bivs );
    }
}


Interval<int> BinIDValueSet::inlRange() const
{
    Interval<int> ret( mUndefIntVal, -mUndefIntVal );
    for ( int iinl=0; iinl<inls.size(); iinl++ )
	ret.include( inls[iinl] );
    return ret;
}


Interval<int> BinIDValueSet::crlRange() const
{
    Interval<int> ret( mUndefIntVal, -mUndefIntVal );
    if ( !inls.size() ) return ret;

    Pos pos; BinID bid;
    while ( next(pos) )
    {
	get( pos, bid );
	ret.include( bid.crl );
    }

    return ret;
}


Interval<float> BinIDValueSet::valRange( int valnr ) const
{
    Interval<float> ret( mUndefValue, -mUndefValue );
    if ( valnr >= nrvals || valnr < 0 || isEmpty() )
	return ret;

    Pos pos; const float* vals;
    while ( next(pos) )
	ret.include( ((*valsets[pos.i])[pos.j])[valnr], false );

    return ret;
}


void BinIDValueSet::copyStructureFrom( const BinIDValueSet& bvs )
{
    empty();
    const_cast<int&>(nrvals) = bvs.nrvals;
    allowdup = bvs.allowdup;
}


BinIDValueSet::Pos BinIDValueSet::findFirst( const BinID& bid ) const
{
    bool found; int idx = findIndexFor(inls,bid.inl,&found);
    Pos pos( found ? idx : -1, -1 );
    if ( pos.i >= 0 )
    {
	TypeSet<int>& crls = *crlsets[pos.i];
	idx = findIndexFor(crls,bid.crl,&found);
	pos.j = found ? idx : -1;
	if ( found )
	{
	    pos.j = idx;
	    while ( pos.j && crls[pos.j-1] == bid.crl )
		pos.j--;
	}
    }

    return pos;
}


bool BinIDValueSet::next( BinIDValueSet::Pos& pos, bool skip_dup ) const
{
    if ( pos.i < 0 )
    {
	if ( inls.size() < 1 ) return false;
	pos.i = pos.j = 0;
	return true;
    }
    else if ( pos.i >= inls.size() )
	{ pos.i = pos.j = -1; return false; }
    else if ( pos.j < 0 )
    	{ pos.j = 0; return true; }

    TypeSet<int>& crls = *crlsets[pos.i];
    if ( pos.j > crls.size()-2 )
    {
	pos.j = 0;
	pos.i++;
	if ( pos.i >= inls.size() )
	    pos.i = pos.j = -1;
	return pos.i >= 0;
    }

    pos.j++;
    if ( skip_dup && crls[pos.j] == crls[pos.j-1] )
	return next( pos, true );

    return true;
}


bool BinIDValueSet::prev( BinIDValueSet::Pos& pos, bool skip_dup ) const
{
    if ( !pos.valid() )
	return false;
    if ( pos.i == 0 && pos.j == 0)
	{ pos.i = pos.j = -1; return false; }

    int curcrl = (*crlsets[pos.i])[pos.j];
    if ( pos.j )
	pos.j--;
    else
    {
	pos.i--;
	pos.j = crlsets[pos.i]->size() - 1;
    }

    if ( !skip_dup ) return true;

    while ( (*crlsets[pos.i])[pos.j] == curcrl )
	return prev( pos, true );

    return true;
}


bool BinIDValueSet::valid( const Pos& pos ) const
{
    return pos.valid()
	&& inls.indexOf(pos.i) >= 0
	&& crlsets[pos.i]->size() > pos.j;
}


void BinIDValueSet::get( const Pos& pos, BinID& bid, float* vs ) const
{
    if ( !pos.valid() )
	{ bid.inl = bid.crl = 0; }
    else
    {
	bid.inl = inls[pos.i];
	bid.crl = (*crlsets[pos.i])[pos.j];
	if ( vs && nrvals )
	{
	    memcpy( vs, (*valsets[pos.i])[pos.j], nrvals * sizeof(float) );
	    return;
	}
    }

    if ( vs ) 
	mSetUdf(vs,mUndefValue);
}


float* BinIDValueSet::gtVals( const Pos& pos ) const
{
    const float* res = pos.valid() && nrvals ? (*valsets[pos.i])[pos.j] : 0;
    return const_cast<float*>( res );
}


BinIDValueSet::Pos BinIDValueSet::getPos( int glidx ) const
{
    int firstidx = 0; Pos pos;
    for ( pos.i=0; pos.i<inls.size(); pos.i++ )
    {
	TypeSet<int>& crls = *crlsets[pos.i];
	if ( firstidx + crls.size() > glidx )
	{
	    pos.j = glidx - firstidx;
	    return pos;
	}
	firstidx += crls.size();
    }

    return Pos(-1,-1);
}


BinIDValueSet::Pos BinIDValueSet::add( const BinID& bid, const float* arr )
{
    Pos pos( findFirst(bid) );
    if ( pos.i < 0 )
    {
	pos.i = findIndexFor(inls,bid.inl) + 1;
	if ( pos.i > inls.size()-1 )
	{
	    inls += bid.inl;
	    crlsets += new TypeSet<int>;
	    valsets += new TypeSet<float*>;
	    pos.i = inls.size() - 1;
	}
	else
	{
	    inls.insert( pos.i, bid.inl );
	    crlsets.insertAt( new TypeSet<int>, pos.i );
	    valsets.insertAt( new TypeSet<float*>, pos.i );
	}
    }

    if ( pos.j < 0 || allowdup )
	addNew( pos, bid.crl, arr );

    return pos;
}


void BinIDValueSet::addNew( BinIDValueSet::Pos& pos, int crl, const float* arr )
{
    TypeSet<int>& crls = *crlsets[pos.i];
    TypeSet<float*>& vals = *valsets[pos.i];

    float* newvals = 0;
    if ( nrvals )
    {
	newvals = new float [nrvals];
	if ( arr )
	    memcpy( newvals, arr, nrvals * sizeof(float) );
	else
	    mSetUdf( newvals, nrvals );
    }

    if ( pos.j < 0 )
	pos.j = findIndexFor(crls,crl) + 1;
    else
    {
	pos.j++;
	while ( pos.j < crls.size() && crls[pos.j] == crl )
	    pos.j++;
    }

    if ( pos.j > crls.size() - 1 )
    {
	crls += crl;
	if ( newvals ) vals += newvals;
    }
    else
    {
	crls.insert( pos.j, crl );
	if ( newvals ) vals.insert( pos.j,  newvals );
    }
}


BinIDValueSet::Pos BinIDValueSet::add( const BinIDValues& bivs )
{
    if ( bivs.size() >= nrvals )
	return add( bivs.binid, bivs.values() );
    BinIDValues locbivs( 0, 0, nrvals );
    for ( int idx=0; idx<bivs.size(); idx++ )
	locbivs.value(idx) = bivs.value(idx);
    for ( int idx=bivs.size(); idx<nrvals; idx++ )
	locbivs.value(idx) = mUndefValue;
    return add( bivs.binid, locbivs.values() );
}


void BinIDValueSet::set( BinIDValueSet::Pos pos, const float* vals )
{
    if ( !pos.valid() || !nrvals ) return;

    if ( vals )
	memcpy( (*valsets[pos.i])[pos.j], vals, nrvals*sizeof(float) );
    else
	mSetUdf( (*valsets[pos.i])[pos.j], nrvals );
}


int BinIDValueSet::nrPos( int inlidx ) const
{
    return inlidx < 0 || inlidx >= inls.size() ? 0 : crlsets[inlidx]->size();
}


int BinIDValueSet::totalSize() const
{
    int nr = 0;
    for ( int idx=0; idx<inls.size(); idx++ )
	nr += crlsets[idx]->size();
    return nr;
}


bool BinIDValueSet::hasInl( int inl ) const
{
    return inls.indexOf(inl) >= 0;
}


bool BinIDValueSet::hasCrl( int crl ) const
{
    for ( int iinl=0; iinl<inls.size(); iinl++ )
    {
	TypeSet<int>& crls = *crlsets[iinl];
	for ( int icrl=0; icrl<crls.size(); icrl++ )
	    if ( crls[icrl] == crl ) return true;
    }
    return false;
}


BinID BinIDValueSet::firstPos() const
{
    Pos pos; next(pos);
    BinID bid; get(pos,bid);
    return bid;
}


void BinIDValueSet::remove( const Pos& pos )
{
    if ( pos.i < 0 || pos.i >= inls.size() ) return;
    TypeSet<int>& crls = *crlsets[pos.i];
    TypeSet<float*>& vals = *valsets[pos.i];
    if ( pos.j < 0 || pos.j >= crls.size() ) return;
    crls.remove( pos.j );
    delete [] vals[pos.j];
    vals.remove( pos.j );
}


void BinIDValueSet::removeDuplicateBids()
{
    Pos pos; next(pos,false);
    BinID prev; get( pos, prev );
    bool donext = true;
    BinID cur;
    while ( !donext || next(pos,false) )
    {
	get( pos, cur );
	if ( prev == cur )
	    { remove(pos); donext = false; }
	else
	    prev = cur;
    }
}


void BinIDValueSet::setNrVals( int newnrvals, bool kp_data )
{
    if ( newnrvals == nrvals ) return;

    const int orgnrvals = nrvals;
    const_cast<int&>( nrvals ) = newnrvals;

    if ( !nrvals )
    {
	for ( int iinl=0; iinl<inls.size(); iinl++ )
	{
	    TypeSet<float*>& vals = *valsets[iinl];
	    const int sz = crlsets[iinl]->size();
	    for ( int icrl=0; icrl<sz; icrl++ )
		delete vals[icrl];
	    vals.erase();
	}
	return;
    }

    if ( !orgnrvals )
    {
	for ( int iinl=0; iinl<inls.size(); iinl++ )
	{
	    TypeSet<float*>& vals = *valsets[iinl];
	    vals.erase(); // shouldn't be necessary
	    const int sz = crlsets[iinl]->size();
	    for ( int icrl=0; icrl<sz; icrl++ )
	    {
		float* newvs = new float [ nrvals ];
		mSetUdf( newvs, nrvals );
		vals += newvs;
	    }
	}
	return;
    }

    const int transfsz = !kp_data ? 0
			: (orgnrvals > nrvals ? nrvals : orgnrvals);
    for ( int iinl=0; iinl<inls.size(); iinl++ )
    {
	float** vals = valsets[iinl]->arr();
	const int sz = crlsets[iinl]->size();
	for ( int icrl=0; icrl<sz; icrl++ )
	{
	    float* orgdata = vals[icrl];
	    vals[icrl] = new float [nrvals ];
	    if ( transfsz )
		memcpy( vals[icrl], orgdata, transfsz * sizeof(float) );
	    delete orgdata;
	    for ( int idx=transfsz; idx<nrvals; idx++ )
		vals[icrl][idx] = mUndefValue;
	}
    }
}


void BinIDValueSet::sortDuplicateBids( int valnr, bool asc )
{
    if ( valnr >= nrvals || !allowdup ) return;

    for ( int iinl=0; iinl<inls.size(); iinl++ )
    {
	TypeSet<int>& crls = *crlsets[iinl];
	TypeSet<float*>& vals = *valsets[iinl];
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


void BinIDValueSet::sortPart( TypeSet<int>& crls, TypeSet<float*>& vals,
			      int valnr, int firstidx, int lastidx, bool asc )
{
    int nridxs = lastidx - firstidx + 1;
    float* vs = new float [ nridxs ];
    int* idxs = new int [ nridxs ];
    for ( int idx=firstidx; idx<=lastidx; idx++ )
    {
	idxs[idx] = idx;
	vs[idx-firstidx] = vals[idx][valnr];
    }
    sort_coupled( vs, idxs, nridxs );

    float** newvals = new float* [ nridxs ];
    for ( int idx=0; idx<nridxs; idx++ )
    {
	int sortidx = idxs[idx];
	if ( !asc ) sortidx = nridxs - sortidx - 1;
	newvals[ idx ] = vals[ sortidx ];
    }

    for ( int idx=firstidx; idx<=lastidx; idx++ )
	vals[idx] = newvals[ idx - firstidx ];

    delete [] vs; delete [] idxs; delete [] newvals;
}


void BinIDValueSet::getExtended( BinIDValueSet& bvs,
				 const BinID& so, const BinID& sos ) const
{
    bool orgallowdup = bvs.allowdup;
    bvs.copyStructureFrom( *this );
    bvs.allowdup = orgallowdup;

    Pos pos; BinIDValues bivs; 
    while ( next(pos,!bvs.allowdup) )
    {
	get( pos, bivs );
	const BinID bid( bivs.binid );
	for ( int iinl=-so.inl; iinl<=so.inl; iinl+=sos.inl )
	{
	    bivs.binid.inl = bid.inl + iinl;
	    for ( int icrl=-so.crl; icrl<=so.crl; icrl+=sos.crl )
	    {
		bivs.binid.crl = bid.crl + icrl;
		bvs.add( bivs );
	    }
	}
    }
}


BinIDValueSet::Pos BinIDValueSet::add( const BinIDValue& biv )
{
    return nrvals < 2 ? add(biv.binid,&biv.value)  : add(BinIDValues(biv));
}


BinIDValueSet::Pos BinIDValueSet::add( const BinID& bid, float v )
{
    if ( nrvals < 2 )
	return add( bid, nrvals == 1 ? &v : 0 );

    BinIDValues bvs( bid, 1 );
    bvs.value(0) = v;
    return add( bvs );
}


BinIDValueSet::Pos BinIDValueSet::add( const BinID& bid, float v1, float v2 )
{
    if ( nrvals == 0 )
	return add( bid );
    else if ( nrvals < 3 )
    {
	float v[2]; v[0] = v1; v[1] = v2;
	return add( bid, v );
    }

    BinIDValues bvs( bid, 2 );
    bvs.value(0) = v1; bvs.value(1) = v2;
    return add( bvs );
}


BinIDValueSet::Pos BinIDValueSet::add( const BinID& bid,
				       const TypeSet<float>& vals )
{
    if ( !vals.size() )
	return add( bid );
    else if ( vals.size() >= nrvals )
	return add( bid, vals.arr() );

    BinIDValues bvs( bid, vals.size() );
    bvs.setVals( vals.arr() );
    return add( bvs );
}


void BinIDValueSet::get( const Pos& pos, BinIDValues& bivs ) const
{
    bivs.setSize( nrvals );
    get( pos, bivs.binid, bivs.values() );
}


void BinIDValueSet::get( const Pos& pos, BinIDValue& biv ) const
{
    if ( nrvals < 2 )
	get( pos, biv.binid, &biv.value );
    else
    {
	BinIDValues bvs; get( pos, bvs ); biv.binid = bvs.binid;
	biv.value = bvs.value(0);
    }
}


void BinIDValueSet::get( const BinIDValueSet::Pos& pos,
			 BinID& bid, float& v ) const
{
    if ( nrvals < 2 )
	get( pos, bid, &v );
    else
    {
	BinIDValues bvs; get( pos, bvs ); bid = bvs.binid;
	v = bvs.value(0);
    }
}


void BinIDValueSet::get( const BinIDValueSet::Pos& pos, BinID& bid,
			 float& v1, float& v2 ) const
{
    if ( nrvals < 3 )
    {
	float v[2]; get( pos, bid, v );
	v1 = v[0]; v2 = v[1];
    }
    else
    {
	BinIDValues bvs; get( pos, bvs ); bid = bvs.binid;
	v1 = bvs.value(0); v2 = bvs.value(1);
    }
}


void BinIDValueSet::get( const BinIDValueSet::Pos& pos, BinID& bid,
			 TypeSet<float>& vals ) const
{
    if ( vals.size() != nrvals )
    {
	vals.erase();
	for ( int idx=0; idx<nrvals; idx++ )
	    vals += mUndefValue;
    }
    get( pos, bid, vals.arr() );
}


void BinIDValueSet::set( const BinIDValueSet::Pos& pos, float v )
{
    set( pos, &v );
}


void BinIDValueSet::set( const BinIDValueSet::Pos& pos, float v1, float v2 )
{
    float v[2]; v[0] = v1; v[1] = v2;
    set( pos, v );
}


void BinIDValueSet::set( const BinIDValueSet::Pos& pos,
			 const TypeSet<float>& vals )
{
    set( pos, vals.arr() );
}


void BinIDValueSet::fillPar( IOPar& iop, const char* ky ) const
{
    FileMultiString fms;
    fms += nrvals; fms += allowdup ? "D" : "N";
    BufferString key; if ( ky && *ky ) { key = ky; key += ".Setup"; }
    iop.set( key, fms );

    for ( int iinl=0; iinl<inls.size(); iinl++ )
    {
	fms = ""; fms += inls[iinl];
	TypeSet<int>& crls = *crlsets[iinl];
	TypeSet<float*>& vals = *valsets[iinl];
	for ( int icrl=0; icrl<crls.size(); icrl++ )
	{
	    fms += crls[icrl];
	    if ( nrvals )
	    {
		const float* v = vals[icrl];
		for ( int idx=0; idx<nrvals; idx++ )
		    fms += v[idx];
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


void BinIDValueSet::usePar( const IOPar& iop, const char* ky )
{
    BinIDValues bivs( 0, 0, nrvals );
    FileMultiString fms;
    BufferString key; if ( ky && *ky ) { key = ky; key += ".Setup"; }
    const char* res = iop.find( key );
    if ( res && *res )
    {
	empty();
	fms = res;
	setNrVals( atoi(fms[0]), false );
	allowdup = *fms[1] == 'D';
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
	bivs.binid.inl = atoi( fms[0] );
	int nrpos = (fms.size() - 1) / (nrvals + 1);
	for ( int icrl=0; icrl<nrpos; icrl++ )
	{
	    int fmsidx = 1 + icrl * (nrvals + 1);
	    bivs.binid.crl = atoi( fms[fmsidx] );
	    fmsidx++;
	    for ( int ival=0; ival<nrvals; ival++ )
		bivs.value(ival) = atof( fms[fmsidx+ival] );
	    add( bivs );
	}
    }
}
