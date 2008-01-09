/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-6-1996
-*/

static const char* rcsID = "$Id: binidvalset.cc,v 1.21 2008-01-09 13:54:34 cvsbert Exp $";

#include "binidvalset.h"
#include "iopar.h"
#include "separstr.h"
#include "idxable.h"
#include "sorting.h"
#include "strmoper.h"
#include "survinfo.h"
#include <iostream>


static inline void setToUdf( float* arr, int nvals )
{
    for ( int idx=0; idx<nvals; idx++ )
	Values::setUdf( arr[idx] );
}


static int findIndexFor( const TypeSet<int>& nrs, int nr, bool* found = 0 )
{
    int ret;
    bool fnd = IdxAble::findPos( nrs.arr(), nrs.size(), nr, -1, ret );
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
	inls += bvs.inls[iinl];
	crlsets += new TypeSet<int>( *bvs.crlsets[iinl] );
	valsets += new TypeSet<float>( *bvs.valsets[iinl] );
    }

    return *this;
}


void BinIDValueSet::empty()
{
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
	    add( bid, nrvals ? bvs.getVals( pos ) : 0 );
	}
    }
    else
    {
	float* insvals = new float [nrvals];
	setToUdf(insvals,nrvals);
	while ( bvs.next(pos,!allowdup) )
	{
	    bvs.get( pos, bid );
	    memcpy( insvals, bvs.getVals( pos ), bvs.nrvals * sizeof(float) );
	    add( bid, insvals );
	}
	delete [] insvals;
    }
}


bool BinIDValueSet::getFrom( std::istream& strm )
{
    empty();
    setNrVals( 0, false );

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
	crd.x = atof( valbuf );
	mSkipBlanks( nextword ); if ( !*nextword ) continue;
	nextword = getNextWord( nextword, valbuf );
	crd.y = atof( valbuf );

	bid = SI().transform( crd );
	if ( !SI().isReasonable(bid) )
	{
	    bid.inl = (int)crd.x; bid.crl = (int)crd.y;
	    if ( !SI().isReasonable(bid) )
		continue;
	}

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
	    vals[idx] = (float)atof(valbuf);
	}
    }

    return !isEmpty();
}


bool BinIDValueSet::putTo( std::ostream& strm ) const
{
    Pos pos;
    while ( next(pos) )
    {
	const BinID bid( getInl(pos), getCrl(pos) );
	const float* vals = getVals(pos);
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
    return inlidx<0 ? 0 : getCrlSet(inlidx).size();
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
    if ( inls.isEmpty() ) return ret;

    const int inlidx = inls.indexOf( inl );
    if ( inlidx >= 0 )
    {
	const TypeSet<int>& crlset = getCrlSet(inlidx);
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
	const TypeSet<int>& crls = getCrlSet(pos);
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

    const TypeSet<int>& crls = getCrlSet(pos);
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


bool BinIDValueSet::valid( const BinID& bid ) const
{
    Pos pos = findFirst( bid );
    return pos.valid()
	&& inls.indexOf(bid.inl) >= 0
	&& getCrlSet(pos).size() > pos.j;
}


void BinIDValueSet::get( const Pos& pos, BinID& bid, float* vs ) const
{
    if ( !pos.valid() )
	{ bid.inl = bid.crl = 0; }
    else
    {
	bid.inl = getInl(pos); bid.crl = getCrl(pos);
	if ( vs && nrvals )
	{
	    memcpy( vs, getVals(pos), nrvals * sizeof(float) );
	    return;
	}
    }

    if ( vs ) 
	setToUdf(vs,nrvals);
}


BinID BinIDValueSet::getBinID( const Pos& pos ) const
{
    return pos.valid() ? BinID(getInl(pos),getCrl(pos)) : BinID(0,0);
}


BinIDValueSet::Pos BinIDValueSet::getPos( int glidx ) const
{
    int firstidx = 0; Pos pos;
    for ( pos.i=0; pos.i<inls.size(); pos.i++ )
    {
	const TypeSet<int>& crls = getCrlSet(pos);
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
	    valsets += new TypeSet<float>;
	    pos.i = inls.size() - 1;
	}
	else
	{
	    inls.insert( pos.i, bid.inl );
	    crlsets.insertAt( new TypeSet<int>, pos.i );
	    valsets.insertAt( new TypeSet<float>, pos.i );
	}
    }

    if ( pos.j < 0 || allowdup )
	addNew( pos, bid.crl, arr );

    return pos;
}


void BinIDValueSet::addNew( BinIDValueSet::Pos& pos, int crl, const float* arr )
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
	for ( int idx=0; idx<nrvals; idx++ )
	    vals += arr ? arr[idx] : mUdf(float);
    }
    else
    {
	crls.insert( pos.j, crl );
	//TOOPTIM: with memcpy's. This will be slow for high nrvals
	for ( int idx=nrvals-1; idx>=0; idx-- )
	    vals.insert( pos.j*nrvals, arr ? arr[idx] : mUdf(float) );
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
	memcpy( getVals(pos), vals, nrvals*sizeof(float) );
    else
	setToUdf( getVals(pos), nrvals );
}


int BinIDValueSet::nrPos( int inlidx ) const
{
    return inlidx < 0 || inlidx >= inls.size() ? 0 : getCrlSet(inlidx).size();
}


int BinIDValueSet::totalSize() const
{
    int nr = 0;
    for ( int idx=0; idx<inls.size(); idx++ )
	nr += getCrlSet(idx).size();
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
	const TypeSet<int>& crls = getCrlSet(iinl);
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
    if ( pos.i < 0 || pos.i >= inls.size() )
	return;

    TypeSet<int>& crls = getCrlSet(pos);
    if ( pos.j < 0 || pos.j >= crls.size() )
	return;

    crls.remove( pos.j );
    if ( crls.size() )
    {
	if ( nrvals )
	    getValSet(pos).remove( pos.j*nrvals, (pos.j+1)*nrvals - 1 );
    }
    else
    {
	inls.remove( pos.i );
	delete crlsets[pos.i];
	crlsets.remove( pos.i );
	delete valsets[pos.i];
	valsets.remove( pos.i );
    }
}


void BinIDValueSet::remove( const TypeSet<BinIDValueSet::Pos>& poss )
{
    if ( poss.size() < 1 )
	return;
    else if ( poss.size() == 1 )
	{ remove( poss[0] ); return; }

    BinIDValueSet bvs( *this );
    empty();
    Pos pos; BinID bid;
    while ( bvs.next(pos) )
    {
	if ( poss.indexOf(pos) < 0 )
	{
	    bvs.get( pos, bid );
	    add( bid, bvs.getVals(pos) );
	}
    }
}


void BinIDValueSet::removeDuplicateBids()
{
    if ( isEmpty() ) return;

    Pos pos; next(pos,false);
    BinID prevbid; get( pos, prevbid );
    TypeSet<BinIDValueSet::Pos> poss;
    BinID cur;
    while ( next(pos,false) )
    {
	get( pos, cur );
	if ( prevbid == cur )
	    poss += pos;
	else
	    prevbid = cur;
    }
    remove( poss );
}


void BinIDValueSet::removeVal( int validx )
{
    if ( validx < 0 || validx >= nrvals )
	return;

    if ( nrvals == 1 )
    {
	setNrVals( 0, false );
	return;
    }

    for ( int iinl=0; iinl<inls.size(); iinl++ )
    {
	TypeSet<float>& vals = getValSet(iinl);
	TypeSet<int>& crls = getCrlSet(iinl);
	for ( int icrl=crls.size()-1; icrl>=0; icrl-- )
	    vals.remove( nrvals*icrl+validx );
    }
    const_cast<int&>(nrvals)--;
}


void BinIDValueSet::setNrVals( int newnrvals, bool keepdata )
{
    if ( newnrvals == nrvals )
	return;

    const int oldnrvals = nrvals;
    const_cast<int&>( nrvals ) = newnrvals;

    for ( int iinl=0; iinl<inls.size(); iinl++ )
    {
	const int nrcrl = getCrlSet(iinl).size();
	if ( nrvals == 0 )
	    getValSet(iinl).erase();
	else if ( oldnrvals == 0 )
	    getValSet(iinl).setSize( nrcrl * nrvals, mUdf(float) );
	else
	{
	    TypeSet<float>* oldvals = valsets[iinl];
	    TypeSet<float>* newvals = new TypeSet<float>( nrcrl*nrvals,
		    					  mUdf(float) );
	    valsets.replace( iinl, newvals );
	    if ( keepdata )
	    {
		float* oldarr = oldvals->arr();
		float* newarr = newvals->arr();
		const int cpsz = (oldnrvals > nrvals ? nrvals : oldnrvals)
		    		   * sizeof( float );
		for ( int icrl=0; icrl<nrcrl; icrl++ )
		    memcpy( newarr+icrl*nrvals, oldarr+icrl*oldnrvals, cpsz );
	    }
	    delete oldvals;
	}
    }
}


void BinIDValueSet::sortDuplicateBids( int valnr, bool asc )
{
    if ( valnr >= nrvals || !allowdup ) return;

    for ( int iinl=0; iinl<inls.size(); iinl++ )
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


void BinIDValueSet::sortPart( TypeSet<int>& crls, TypeSet<float>& vals,
			      int valnr, int firstidx, int lastidx, bool asc )
{
    int nridxs = lastidx - firstidx + 1;
    float* vs = new float [ nridxs ];
    int* idxs = new int [ nridxs ];
    for ( int idx=firstidx; idx<=lastidx; idx++ )
    {
	idxs[idx] = idx;
	vs[idx-firstidx] = vals[nrvals*idx+valnr];
    }
    sort_coupled( vs, idxs, nridxs );

    for ( int idx=0; idx<nridxs; idx++ )
    {
	if ( idxs[idx] != idx )
	{
	    Swap( crls[idx], crls[ idxs[idx] ] );
	    for ( int iv=0; iv<nrvals; iv++ )
		Swap( vals[ idx*nrvals + iv], vals[ idxs[idx]*nrvals + iv ] );
	}
    }

    delete [] vs; delete [] idxs;
}


void BinIDValueSet::extend( const BinID& so, const BinID& sos )
{
    if ( (!so.inl && !so.crl) || (!sos.inl && !sos.crl) ) return;

    BinIDValueSet bvs( *this );

    const bool kpdup = allowdup;
    allowdup = false;

    Pos pos; BinID bid;
    float* vals = nrvals ? new float [nrvals] : 0;
    while ( bvs.next(pos) )
    {
	bvs.get( pos, bid, vals );
	const BinID centralbid( bid );
	for ( int iinl=-so.inl; iinl<=so.inl; iinl++ )
	{
	    bid.inl = centralbid.inl + iinl * sos.inl;
	    for ( int icrl=-so.crl; icrl<=so.crl; icrl++ )
	    {
		if ( !iinl && !icrl )
		    continue;
		bid.crl = centralbid.crl + icrl * sos.crl;
		add( bid, vals );
	    }
	}
    }

    delete [] vals;
    allowdup = kpdup;
}


void BinIDValueSet::getColumn( int valnr, TypeSet<float>& vals,
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


void BinIDValueSet::removeRange( int valnr, const Interval<float>& rg,
				 bool inside )
{
    if ( valnr < 0 || valnr >= nrVals() ) return;
    TypeSet<Pos> poss; Pos pos;
    while ( next(pos) )
    {
	const float* v = getVals( pos );
	if ( inside == rg.includes(v[valnr]) )
	    poss += pos;
    }
    remove( poss );
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
    if ( vals.isEmpty() )
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
	const TypeSet<int>& crls = getCrlSet(iinl);
	const TypeSet<float>& vals = getValSet(iinl);
	for ( int icrl=0; icrl<crls.size(); icrl++ )
	{
	    fms += crls[icrl];
	    if ( nrvals )
	    {
		const float* v = vals.arr() + icrl*nrvals;
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


bool BinIDValueSet::areBinidValuesThere( const BinIDValues& bidvals ) const
{
    Pos pos = findFirst( bidvals.binid );
    bool found = false;
    BinID tmpbid;
    while ( !found && pos.valid() )
    {
	if ( getBinID(pos) != bidvals.binid )
	    break;
	
	TypeSet<float> valofset;
	get( pos, tmpbid, valofset );
	if ( valofset.size() == bidvals.size() )
	{
	    bool diff = false;
	    for ( int idx=0; idx<valofset.size(); idx++ )
	    {
		if ( bidvals.value(idx) != valofset[idx] )
		    diff = true;
	    }
	    found = !diff;
	}
	next( pos );
    }
    
    return found;
}



