/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-6-1996
-*/

static const char* rcsID = "$Id: binidvalset.cc,v 1.5 2005-02-23 14:45:23 cvsarend Exp $";

#include "binidvalset.h"
#include "iopar.h"
#include "separstr.h"
#include "finding.h"
#include "sorting.h"
#include "strmoper.h"
#include <iostream>

#define mSetUdf(arr,nvals) \
    for ( int idx=0; idx<nvals; idx++ ) \
	Values::setUdf(arr[idx])
#define mInl(pos) (inls[pos.i])
#define mCrl(pos) ((*crlsets[pos.i])[pos.j])
#define mVals(pos) (nrvals ? (*valsets[pos.i])[pos.j] : 0)
#define mCrlSet(pos) (*crlsets[pos.i])
#define mValSet(pos) (*valsets[pos.i])


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
    Pos pos; BinID bid;
    if ( nrvals <= bvs.nrvals )
    {
	while ( bvs.next(pos,!bvs.allowdup) )
	{
	    bvs.get( pos, bid );
	    add( bid, nrvals ? (*bvs.valsets[pos.i])[pos.j] : 0 );
	}
    }
    else
    {
	float* insvals = new float [nrvals];
	mSetUdf(insvals,nrvals);
	while ( bvs.next(pos,!bvs.allowdup) )
	{
	    bvs.get( pos, bid );
	    memcpy( insvals, (*bvs.valsets[pos.i])[pos.j],
		    bvs.nrvals * sizeof(float) );
	    add( bid, insvals );
	}

	delete [] insvals;
    }
}


static void ignoreName( std::istream& strm )
{
    while ( isspace(strm.peek()) && strm )
	strm.ignore(1);

    if ( strm.peek() == '"' )
    {
	strm.ignore(1);
	while ( strm.peek() != '"' && strm )
	    strm.ignore(1);
	strm.ignore(1);
    }
}


bool BinIDValueSet::getFrom( std::istream& strm )
{
    BinID bid( 0, mUdf(int) ); char buf[1024]; TypeSet<float> vals;
    int idx = 0;
    ignoreName( strm );
    while ( wordFromLine(strm,buf,1024) )
    {
	if ( idx == 0 )		bid.inl = atoi( buf );
	else if ( idx == 1 )	bid.crl = atoi( buf );
	else			vals += atof( buf );
	idx++;
    }
    if ( Values::isUdf(bid.crl) ) return false;

    empty();
    setNrVals( vals.size() );
    add( bid, vals.arr() );

    while ( strm.good() )
    {
	bid.inl = bid.crl = 0;
	ignoreName( strm );
	strm >> bid.inl >> bid.crl;
	if ( bid.inl == 0 && bid.crl == 0 )
	    return true;
	for ( int idx=0; idx<vals.size(); idx++ )
	    strm >> vals[idx];
	add( bid, vals.arr() );
    }
    return true;
}


bool BinIDValueSet::putTo( std::ostream& strm ) const
{
    Pos pos;
    while ( next(pos) )
    {
	const BinID bid( mInl(pos), mCrl(pos) );
	const float* vals = mVals(pos);
	strm << bid.inl << '\t' << bid.crl;
	for ( int idx=0; idx<nrvals; idx++ )
	    strm << '\t' << getStringFromFloat(0,vals[idx]);
	strm << '\n';
    }
    strm.flush();
    return strm.good();
}


int BinIDValueSet::nrCrls( int inl ) const
{
    const int inlidx = inls.indexOf( inl );
    return inlidx<0 ? 0 : crlsets[inlidx]->size();
}


Interval<int> BinIDValueSet::inlRange() const
{
    Interval<int> ret( mUdf(int), mUdf(int) );

    bool first = true;
    for ( int iinl=0; iinl<inls.size(); iinl++ )
    {
	if ( first )
	    { ret.start = ret.stop = inls[iinl]; first = false; }
	else
	    ret.include( inls[iinl], false );
    }
    return ret;
}


Interval<int> BinIDValueSet::crlRange( int inl ) const
{
    Interval<int> ret( mUdf(int), mUdf(int) );
    if ( !inls.size() ) return ret;

    const int inlidx = inls.indexOf( inl );
    if ( inlidx >= 0 )
    {
	TypeSet<int>& crlset = *crlsets[inlidx];
	for ( int idx=0; idx<crlset.size(); idx++ )
	    ret.include( crlset[idx], false );
	return ret;
    }
    
    Pos pos; BinID bid;
    bool first = true;
    while ( next(pos) )
    {
	get( pos, bid );
	if ( first )
	    { ret.start = ret.stop = bid.crl; first = false; }
	else
	    ret.include( bid.crl, false );
    }

    return ret;
}


Interval<float> BinIDValueSet::valRange( int valnr ) const
{
    Interval<float> ret( mUdf(float), mUdf(float) );
    if ( valnr >= nrvals || valnr < 0 || isEmpty() )
	return ret;

    Pos pos;
    while ( next(pos) )
    {
	const float val = mVals(pos)[valnr];
	if ( !Values::isUdf(val) )
	    { ret.start = ret.stop = val; break; }
    }
    while ( next(pos) )
    {
	const float val = mVals(pos)[valnr];
	if ( !Values::isUdf(val) )
	    ret.include( val, false );
    }

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
	TypeSet<int>& crls = mCrlSet(pos);
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

    TypeSet<int>& crls = mCrlSet(pos);
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

    int curcrl = mCrl(pos);
    if ( pos.j )
	pos.j--;
    else
    {
	pos.i--;
	pos.j = mCrlSet(pos).size() - 1;
    }

    if ( !skip_dup ) return true;

    while ( mCrl(pos) == curcrl )
	return prev( pos, true );

    return true;
}


bool BinIDValueSet::valid( const Pos& pos ) const
{
    return pos.valid()
	&& inls.indexOf(pos.i) >= 0
	&& mCrlSet(pos).size() > pos.j;
}


void BinIDValueSet::get( const Pos& pos, BinID& bid, float* vs ) const
{
    if ( !pos.valid() )
	{ bid.inl = bid.crl = 0; }
    else
    {
	bid.inl = mInl(pos); bid.crl = mCrl(pos);
	if ( vs && nrvals )
	{
	    memcpy( vs, mVals(pos), nrvals * sizeof(float) );
	    return;
	}
    }

    if ( vs ) 
	mSetUdf(vs,nrvals);
}


float* BinIDValueSet::gtVals( const Pos& pos ) const
{
    const float* res = pos.valid() && nrvals ? mVals(pos) : 0;
    return const_cast<float*>( res );
}


BinIDValueSet::Pos BinIDValueSet::getPos( int glidx ) const
{
    int firstidx = 0; Pos pos;
    for ( pos.i=0; pos.i<inls.size(); pos.i++ )
    {
	TypeSet<int>& crls = mCrlSet(pos);
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
    TypeSet<int>& crls = mCrlSet(pos);
    TypeSet<float*>& vals = mValSet(pos);

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
	Values::setUdf( locbivs.value(idx) );
    return add( bivs.binid, locbivs.values() );
}


void BinIDValueSet::set( BinIDValueSet::Pos pos, const float* vals )
{
    if ( !pos.valid() || !nrvals ) return;

    if ( vals )
	memcpy( mVals(pos), vals, nrvals*sizeof(float) );
    else
	mSetUdf( mVals(pos), nrvals );
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
    TypeSet<int>& crls = mCrlSet(pos);
    TypeSet<float*>& vals = mValSet(pos);
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


void BinIDValueSet::removeVal( int validx )
{
    if ( validx < 0 || validx >= nrvals ) return;

    for ( int iinl=0; iinl<inls.size(); iinl++ )
    {
	TypeSet<float*>& vals = *valsets[iinl];
	for ( int icrl=0; icrl<vals.size(); icrl++ )
	{
	    float* oldvals = vals[icrl];
	    float* newvals = new float [nrvals-1];
	    vals[icrl] = newvals;
	    int newidx = 0;
	    const int stopidx = validx == nrvals - 1 ? nrvals - 1 : nrvals;
	    for ( int oldidx=0; oldidx<stopidx; oldidx++ )
	    {
		newvals[newidx] = oldvals[oldidx];
		if ( oldidx != validx )
		    newidx++;
	    }
	    delete [] oldvals;
	}
    }

    const_cast<int&>(nrvals)--;
}


void BinIDValueSet::setNrVals( int newnrvals, bool kp_data )
{
    if ( newnrvals == nrvals ) return;

    const int orgnrvals = nrvals;
    const_cast<int&>( nrvals ) = newnrvals;

    if ( nrvals == 0 )
    {
	for ( int iinl=0; iinl<inls.size(); iinl++ )
	{
	    TypeSet<float*>& vals = *valsets[iinl];
	    const int sz = crlsets[iinl]->size();
	    for ( int icrl=0; icrl<sz; icrl++ )
		delete [] vals[icrl];
	    vals.erase();
	}
    }
    else if ( orgnrvals == 0 )
    {
	for ( int iinl=0; iinl<inls.size(); iinl++ )
	{
	    TypeSet<float*>& vals = *valsets[iinl];
	    const int sz = crlsets[iinl]->size();
	    for ( int icrl=0; icrl<sz; icrl++ )
	    {
		float* newvals = new float [ nrvals ];
		mSetUdf( newvals, nrvals );
		vals += newvals;
	    }
	}
    }
    else
    {
	const int transfsz = kp_data ? (orgnrvals > nrvals ? nrvals : orgnrvals)
	    			     : 0;
	for ( int iinl=0; iinl<inls.size(); iinl++ )
	{
	    TypeSet<float*>& vals = *valsets[iinl];
	    const int sz = crlsets[iinl]->size();
	    for ( int icrl=0; icrl<sz; icrl++ )
	    {
		float*& data = vals[icrl];
		float* newdata = new float [ nrvals ];
		for ( int idx=0; idx<transfsz; idx++ )
		    newdata[idx] = data[idx];
		delete [] data;
		for ( int idx=transfsz; idx<nrvals; idx++ )
		    Values::setUdf( newdata[idx] );
		data = newdata;
	    }
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


void BinIDValueSet::extend( const BinID& so, const BinID& sos )
{
    if ( (!so.inl && !so.crl) || (!sos.inl && !sos.crl) ) return;

    BinIDValueSet bvs( *this );
    bvs.allowdup = false;

    float* vals = nrvals ? new float [nrvals] : 0;
    mSetUdf(vals,nrvals);
    Pos pos; BinID bid;
    while ( next(pos) )
    {
	const BinID centralbid( mInl(pos), mCrl(pos) );
	for ( int iinl=-so.inl; iinl<=so.inl; iinl++ )
	{
	    bid.inl = centralbid.inl + iinl * sos.inl;
	    for ( int icrl=-so.crl; icrl<=so.crl; icrl++ )
	    {
		if ( !iinl && !icrl ) continue;
		bid.crl = centralbid.crl + icrl * sos.crl;
		bvs.add( bid, vals );
	    }
	}
    }

    delete [] vals;
    bvs.allowdup = allowdup;
    *this = bvs;
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
	    vals += mUdf(float);
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
		    fms += getStringFromFloat(0,v[idx]);
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
