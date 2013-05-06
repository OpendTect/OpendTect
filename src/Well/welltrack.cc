/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "welltrack.h"
#include "welld2tmodel.h"
#include "idxable.h"


Well::Track& Well::Track::operator =( const Track& t )
{
    if ( &t != this )
    {
	setName( t.name() );
	dah_ = t.dah_;
	pos_ = t.pos_;
	zistime_ = t.zistime_;
    }
    return *this;
}


const Interval<float> Well::Track::zRange() const
{
    const int nrpts = nrPoints();
    if ( nrpts < 1 )
	return Interval<float>( 0., 0. );

    const float zstart = value( 0 );
    const float zstop = value( nrpts-1 );
    return Interval<float> ( zstart, zstop );
}


const Interval<float> Well::Track::dahRange() const
{
    const int nrpts = nrPoints();
    if ( nrpts < 1 )
	return Interval<float>( 0., 0. );

    const float dahstart = dah_[0];
    const float dahstop = dah_[ nrpts-1 ];
    return Interval<float>( dahstart, dahstop );
}


void Well::Track::addPoint( const Coord& c, float z, float dahval )
{
    pos_ += Coord3(c,z);
    if ( mIsUdf(dahval) )
    {
	const int previdx = dah_.size() - 1;
	dahval = previdx < 0 && previdx < pos_.size()-1 ? 0
	    : (float) (dah_[previdx] + pos_[previdx].distTo( pos_[previdx+1] ));
    }
    dah_ += dahval;
}


void Well::Track::insertAfterIdx( int aftidx, const Coord3& c )
{
    const int oldsz = pos_.size();
    if ( aftidx > oldsz-2 )
	{ addPoint( c, (float) c.z ); return; }

    double extradah, owndah = 0;
    if ( aftidx == -1 )
	extradah = c.distTo( pos_[0] );
    else
    {
	double dist0 = c.distTo( pos_[aftidx] );
	double dist1 = c.distTo( pos_[aftidx+1] );
	owndah = dah_[aftidx] + dist0;
	extradah = dist0 + dist1 - pos_[aftidx].distTo( pos_[aftidx+1] );
    }

    pos_.insert( aftidx+1, c );
    dah_.insert( aftidx+1, mCast(float,owndah) );
    addToDahFrom( aftidx+2, mCast(float,extradah) );
}


int Well::Track::insertPoint( const Coord& c, float z )
{
    const int oldsz = pos_.size();
    if ( oldsz < 1 )
	{ addPoint( c, z ); return oldsz; }

    Coord3 cnew( c.x, c.y, z );
    if ( oldsz < 2 )
    {
	Coord3 oth( pos_[0] );
	if ( oth.z < cnew.z )
	{
	    addPoint( c, z );
	    return oldsz;
	}
	else
	{
	    pos_.erase(); dah_.erase();
	    pos_ += cnew; pos_ += oth;
	    dah_ += 0;
	    dah_ += (float) oth.distTo( cnew );
	    return 0;
	}
    }

    // Need to find 'best' position. This is when the angle of the triangle
    // at the new point is maximal
    // This boils down to min(sum of sq distances / product of distances)

    float minval = 1e30; int minidx = -1;
    float mindist = (float) pos_[0].distTo(cnew); int mindistidx = 0;
    for ( int idx=1; idx<oldsz; idx++ )
    {
	const Coord3& c0 = pos_[idx-1];
	const Coord3& c1 = pos_[idx];
	const double d = c0.distTo( c1 );
	const double d0 = c0.distTo( cnew );
	const double d1 = c1.distTo( cnew );
	if ( mIsZero(d0,1e-4) || mIsZero(d1,1e-4) )
	    return -1; // point already present
	float val = (float) (( d0 * d0 + d1 * d1 - ( d * d ) ) / (2 * d0 * d1));
	if ( val < minval )
	    { minidx = idx-1; minval = val; }
	if ( d1 < mindist )
	    { mindist = (float) d1; mindistidx = idx; }
	if ( idx == oldsz-1 && minval > 0 )
	{
	    if ( mindistidx == oldsz-1)
	    {
		addPoint( c, z );
		return oldsz;
	    }
	    else if ( mindistidx > 0 && mindistidx < oldsz-1 )
	    {
		float prevdist = (float) pos_[mindistidx-1].distTo(cnew);
		float nextdist = (float) pos_[mindistidx+1].distTo(cnew);
		minidx = prevdist > nextdist ? mindistidx : mindistidx -1;
	    }
	    else
		minidx = mindistidx;
	}
    }

    if ( minidx == 0 )
    {
	// The point may be before the first
	const Coord3& c0 = pos_[0];
	const Coord3& c1 = pos_[1];
	const double d01sq = c0.sqDistTo( c1 );
	const double d0nsq = c0.sqDistTo( cnew );
	const double d1nsq = c1.sqDistTo( cnew );
	if ( d01sq + d0nsq < d1nsq )
	    minidx = -1;
    }
    if ( minidx == oldsz-2 )
    {
	// Hmmm. The point may be beyond the last
	const Coord3& c0 = pos_[oldsz-2];
	const Coord3& c1 = pos_[oldsz-1];
	const double d01sq = c0.sqDistTo( c1 );
	const double d0nsq = c0.sqDistTo( cnew );
	const double d1nsq = c1.sqDistTo( cnew );
	if ( d01sq + d1nsq < d0nsq )
	    minidx = oldsz-1;
    }

    insertAfterIdx( minidx, cnew );
    return minidx+1;
}


bool Well::Track::insertAtDah( float dh, float zpos )
{
    if ( dah_.isEmpty() )
	return false;

    if ( dh < dah_[0] )
    {
	dah_.insert( 0, dh );
	Coord3 crd( pos_[0] ); crd.z = zpos;
	pos_.insert( 0, crd );
    }
    if ( dh > dah_[size()-1] )
    {
	dah_ += dh;
	Coord3 crd( pos_[size()-1] ); crd.z = zpos;
	pos_ += crd;
    }

    const int insertidx = indexOf( dh );
    if ( insertidx<0 )
	return false;
    Coord3 prevcrd( pos_[insertidx] );
    Coord3 nextcrd( pos_[insertidx+1] );
    Coord3 crd( ( prevcrd + nextcrd )/2 );
    crd.z = zpos;

    dah_.insert( insertidx+1, dh );
    pos_.insert( insertidx+1, crd );

    return true;
}


void Well::Track::setPoint( int idx, const Coord& c, float z )
{
    const int nrpts = pos_.size();
    if ( idx<0 || idx>=nrpts ) return;

    Coord3 oldpt( pos_[idx] );
    Coord3 newpt( c.x, c.y, z );
    float olddist0 = idx > 0 ? (float) oldpt.distTo(pos_[idx-1]) : 0;
    float newdist0 = idx > 0 ? (float) newpt.distTo(pos_[idx-1]) : 0;
    float olddist1 = 0, newdist1 = 0;
    if ( idx < nrpts-1 )
    {
	olddist1 = (float) oldpt.distTo(pos_[idx+1]);
	newdist1 = (float) newpt.distTo(pos_[idx+1]);
    }

    pos_[idx] = newpt;
    dah_[idx] += newdist0 - olddist0;
    addToDahFrom( idx+1, newdist0 - olddist0 + newdist1 - olddist1 );
}


void Well::Track::removePoint( int idx )
{
    if ( idx < pos_.size()-1 && idx < dah_.size()-1 )
    {
	float olddist = idx ? dah_[idx+1] - dah_[idx-1] : dah_[1];
	float newdist = idx ? (float) pos_[idx+1].distTo( pos_[idx-1] ) : 0;
	float extradah = olddist - newdist;
	removeFromDahFrom( idx+1, extradah );
    }

    remove( idx );
}


Coord3 Well::Track::getPos( float dh ) const
{
    int idx1;
    if ( IdxAble::findFPPos(dah_,dah_.size(),dh,-1,idx1) )
	return pos_[idx1];
    else if ( idx1 < 0 || idx1 == dah_.size()-1 )
	return Coord3::udf();

    return coordAfterIdx( dh, idx1 );
}


Coord3 Well::Track::coordAfterIdx( float dh, int idx1 ) const
{
    const int idx2 = idx1 + 1;
    const float d1 = dh - dah_[idx1];
    const float d2 = dah_[idx2] - dh;
    const Coord3& c1 = pos_[idx1];
    const Coord3& c2 = pos_[idx2];
    const float f = 1.f / (d1 + d2);
    return Coord3( f * (d1 * c2.x + d2 * c1.x), f * (d1 * c2.y + d2 * c1.y),
		   f * (d1 * c2.z + d2 * c1.z) );
}


float Well::Track::getDahForTVD( float z, float prevdah ) const
{
    const bool haveprevdah = !mIsUdf(prevdah);
    const int sz = dah_.size();
    if ( sz < 1 )
	return mUdf(float);
    if ( zistime_ )
	{ pErrMsg("getDahForTVD called for time well");
	    return haveprevdah ? prevdah : dah_[0]; }

    static const float eps = 1e-3; // do not use lower for float
    if ( sz == 1 )
	return mIsEqual(z,value(0),eps) ? dah_[0] : mUdf(float);

    float minz = 1e6;
    float maxz = -1e6;
    for ( int idz=0; idz<sz; idz++ )
    {
	if ( value(idz) < minz )
	    minz = value(idz);
	if ( value(idz) > maxz )
	    maxz = value(idz);
    }
    if ( z < minz-eps || z > maxz+eps )
	return mUdf(float);

#define mZInRg() \
    (zrg.start-eps < z  && zrg.stop+eps  > z) \
 || (zrg.stop-eps  < z  && zrg.start+eps > z)

    Interval<double> zrg( minz, 0 );
    int idxafter = -1;
    for ( int idx=1; idx<sz; idx++ )
    {
	if ( !haveprevdah || prevdah+eps < dah_[idx] )
	{
	    zrg.stop = value(idx);
	    if ( mZInRg() )
		{ idxafter = idx; break; }
	}
	zrg.start = zrg.stop;
    }
    if ( idxafter < 1 )
	return mUdf(float);

    const int idx1 = idxafter - 1; const int idx2 = idxafter;
    const float z1 = value(idx1);
    const float z2 = value(idx2);
    const float dah1 = dah_[idx1];
    const float dah2 = dah_[idx2];
    const float zdiff = z2 - z1;
    return mIsZero(zdiff,eps) ? dah2 : ((z-z1) * dah2 + (z2-z) * dah1) / zdiff;
}


float Well::Track::nearestDah( const Coord3& posin ) const
{
    if ( dah_.isEmpty() ) return 0;

    if ( dah_.size() < 2 ) return dah_[1];

    const float zfac = mCast( float, zistime_ ? 2000 : 1 );
    Coord3 reqpos( posin ); reqpos.z *= zfac;
    Coord3 curpos( getPos( dah_[0] ) ); curpos.z *= zfac;
    double sqneardist = curpos.sqDistTo( reqpos );
    double sqsecdist = sqneardist;
    int nearidx = 0; int secondidx = 0;
    curpos = getPos( dah_[1] ); curpos.z *= zfac;
    double sqdist = curpos.sqDistTo( reqpos );
    if ( sqdist < sqneardist )
	{ nearidx = 1; sqneardist = sqdist; }
    else
	{ secondidx = 1; sqsecdist = sqdist; }

    for ( int idah=2; idah<dah_.size(); idah++ )
    {
	curpos = getPos( dah_[idah] ); curpos.z *= zfac;
	sqdist = curpos.sqDistTo( reqpos );
	if ( sqdist < 0.1 ) return dah_[idah];

	if ( sqdist < sqneardist )
	{
	    secondidx = nearidx; sqsecdist = sqneardist;
	    nearidx = idah; sqneardist = sqdist;
	}
	else if ( sqdist < sqsecdist )
	    { secondidx = idah; sqsecdist = sqdist; }

	if ( sqdist > 2 * sqneardist ) // safe for 'reasonable wells'
	    break;
    }

    const float neardist = (float) Math::Sqrt( sqneardist );
    const float secdist = (float) Math::Sqrt( sqsecdist );
    return (neardist*dah_[secondidx] + secdist * dah_[nearidx])
         / (neardist + secdist);
}


bool Well::Track::alwaysDownward() const
{
    if ( pos_.size() < 2 )
	return pos_.size();

    float prevz = (float) pos_[0].z;
    for ( int idx=1; idx<pos_.size(); idx++ )
    {
	float curz = (float) pos_[idx].z;
	if ( curz <= prevz )
	    return false;
	prevz = curz;
    }

    return true;
}


void Well::Track::toTime( const D2TModel& d2t, const Track& track )
{
    TypeSet<float> newdah;
    TypeSet<Coord3> newpos;

    // We need to collect control points from both the track and the d2t model
    // because both will be 'bend' points in time

    // First, get the dahs + positions from both - in depth.
    // We'll start with the first track point
    int d2tidx = 0;
    float curdah = dah_[0];
    while ( d2tidx < d2t.size() && d2t.dah(d2tidx) < curdah + mDefEps )
	d2tidx++; // don't need those points: before well track
    newdah += curdah;
    newpos += pos_[0];

    // Now collect the rest of the track points and the d2t control points
    // Make sure no phony double points: allow a tolerance
    const float tol = 0.001;
    for ( int trckidx=1; trckidx<dah_.size(); trckidx++ )
    {
	curdah = dah_[trckidx];
	while ( d2tidx < d2t.size() )
	{
	    const float d2tdah = d2t.dah( d2tidx );
	    const float diff = d2tdah - curdah;
	    if ( diff > tol ) // d2t dah is further down track; handle later
		break;
	    else if ( diff <= -tol ) // therfore a dah not on the track
	    {
		newdah += d2tdah;
		newpos += getPos( d2tdah );
	    }
	    d2tidx++;
	}
	newdah += curdah;
	newpos += pos_[trckidx];
    }

    // Copy the extended set into the new track definition
    dah_ = newdah;
    pos_ = newpos;

    // Now, convert to time
    for ( int idx=0; idx<dah_.size(); idx++ )
    {
	Coord3& pt = pos_[idx];
	pt.z = d2t.getTime( dah_[idx], track );
    }

    zistime_ = true;
}
