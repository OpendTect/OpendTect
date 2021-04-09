/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "welltrack.h"

#include "idxable.h"
#include "survinfo.h"
#include "velocitycalc.h"
#include "welldata.h"
#include "trigonometry.h"


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


bool Well::Track::isEmpty() const
{
    return dah_.isEmpty() || pos_.isEmpty();
}


float Well::Track::getKbElev() const
{
    if ( isEmpty() )
	return 0;

    return dah_[0] - value(0);
}


const Interval<double> Well::Track::zRangeD() const
{
    if ( isEmpty() )
	return Interval<double>( 0., 0. );

    double zstart = pos_[0].z;
    double zstop = pos_[0].z;

    for ( int idx=1; idx<size(); idx++ )
    {
	const double zval = pos_[idx].z;
	if ( zval < zstart )
	    zstart = zval;
	else if ( zval > zstop )
	    zstop = zval;
    }

    return Interval<double> ( zstart, zstop );
}


const Interval<float> Well::Track::zRange() const
{
    Interval<double> zrange = zRangeD();
    return Interval<float> ( (float) zrange.start, (float) zrange.stop );
}


void Well::Track::addPoint( const Coord3& c, float dahval )
{
    pos_ += c;
    if ( mIsUdf(dahval) )
    {
	const int previdx = dah_.size() - 1;
	dahval = previdx < 0 && previdx < pos_.size()-1 ? 0.f
	    : mCast(float,pos_[previdx].distTo(pos_[previdx+1])+dah_[previdx] );
    }

    dah_ += dahval;
}


void Well::Track::addPoint( const Coord& c, float z, float dahval )
{
    Coord3 c3( c, z );
    addPoint( c3, dahval );
}


void Well::Track::insertAfterIdx( int aftidx, const Coord3& c )
{
    const int oldsz = pos_.size();
    if ( aftidx > oldsz-2 )
	{ addPoint( c ); return; }

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


int Well::Track::insertPoint( const Coord3& c )
{
    const int oldsz = pos_.size();
    if ( oldsz < 1 )
	{ addPoint( c ); return oldsz; }

    Coord3 cnew( c );
    if ( oldsz < 2 )
    {
	Coord3 oth( pos_[0] );
	if ( oth.z < cnew.z )
	{
	    addPoint( c );
	    return oldsz;
	}
	else
	{
	    pos_.erase(); dah_.erase();
	    pos_ += cnew; pos_ += oth;
	    dah_ += 0.f;
	    dah_ += mCast(float,oth.distTo( cnew ));
	    return 0;
	}
    }

    // Need to find 'best' position. This is when the angle of the triangle
    // at the new point is maximal
    // This boils down to min(sum of sq distances / product of distances)

    double minval = 1e30; int minidx = -1;
    int mindistidx = 0;
    double mindist = pos_[mindistidx].distTo(cnew);
    for ( int idx=1; idx<oldsz; idx++ )
    {
	const Coord3& c0 = pos_[idx-1];
	const Coord3& c1 = pos_[idx];
	const double d = c0.distTo( c1 );
	const double d0 = c0.distTo( cnew );
	const double d1 = c1.distTo( cnew );
	if ( mIsZero(d0,1e-4) || mIsZero(d1,1e-4) )
	    return -1; // point already present
	double val = (( d0 * d0 + d1 * d1 - ( d * d ) ) / (2 * d0 * d1));
	if ( val < minval )
	    { minidx = idx-1; minval = val; }
	if ( d1 < mindist )
	    { mindist = d1; mindistidx = idx; }
	if ( idx == oldsz-1 && minval > 0 )
	{
	    if ( mindistidx == oldsz-1)
	    {
		addPoint( c );
		return oldsz;
	    }
	    else if ( mindistidx > 0 && mindistidx < oldsz-1 )
	    {
		double prevdist = pos_[mindistidx-1].distTo(cnew);
		double nextdist = pos_[mindistidx+1].distTo(cnew);
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


int  Well::Track::insertPoint( const Coord& c, float z )
{
    Coord3 c3( c, z );
    return insertPoint( c3 );
}


bool Well::Track::insertAtDah( float dh, float zpos )
{
    if ( dah_.isEmpty() )
	return false;

    if ( dh < dah_[0] )
    {
	dah_.insert( 0, dh );
	Coord3 crd( pos_[0] ); crd.z = mCast(double,zpos);
	pos_.insert( 0, crd );
	return true;
    }
    if ( dh > dah_[size()-1] )
    {
	dah_ += dh;
	Coord3 crd( pos_[size()-1] ); crd.z = zpos;
	pos_ += crd;
	return true;
    }

    const int insertidx = indexOf( dh );
    if ( insertidx<0 )
	return false;

    if ( mIsEqual(dh,dah_[insertidx],1e-3) )
	return true;

    Coord3 prevcrd( pos_[insertidx] );
    Coord3 nextcrd( pos_[insertidx+1] );
    Coord3 crd( ( prevcrd + nextcrd )/2 );
    crd.z = zpos;

    dah_.insert( insertidx+1, dh );
    pos_.insert( insertidx+1, crd );

    return true;
}


void Well::Track::setPoint( int idx, const Coord3& c )
{
    const int nrpts = pos_.size();
    if ( idx<0 || idx>=nrpts ) return;

    Coord3 oldpt( pos_[idx] );
    Coord3 newpt( c );
    double olddist0 = idx > 0 ? oldpt.distTo(pos_[idx-1]) : 0;
    double newdist0 = idx > 0 ? newpt.distTo(pos_[idx-1]) : 0;
    double olddist1 = 0, newdist1 = 0;
    if ( idx < nrpts-1 )
    {
	olddist1 = oldpt.distTo(pos_[idx+1]);
	newdist1 = newpt.distTo(pos_[idx+1]);
    }

    pos_[idx] = newpt;
    dah_[idx] += mCast( float, newdist0 - olddist0 );
    const float dist = mCast(float, newdist0 - olddist0 + newdist1 - olddist1 );
    addToDahFrom( idx+1, dist );
}


void Well::Track::setPoint( int idx, const Coord& c, float z )
{
    Coord3 c3( c, z );
    setPoint( idx, c3 );
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
    if ( pos_.isEmpty() )
	return mUdf(Coord3);

    int idx1;
    if ( IdxAble::findFPPos(dah_,dah_.size(),dh,-1,idx1) )
	return pos_[idx1];
    else if ( idx1 < 0 || idx1 == dah_.size()-1 )
    {
	Coord3 ret ( idx1 < 0 ? pos_[0] : pos_[idx1] );
	double deltamd = idx1<0 ? dah_[0] - dh :  dh - dah_[idx1];
	if ( zistime_ )
	{
	    const double grad = idx1<0
		? ((pos_[1].z-pos_[0].z)/(dah_[1]-dah_[0]))
		: ((pos_[idx1].z-pos_[idx1-1].z)/(dah_[idx1]-dah_[idx1-1]));
	    deltamd *= grad;
	}
	if ( idx1 < 0 )
	    ret.z -= deltamd;
	else
	    ret.z += deltamd;

	return ret;
    }

    return coordAfterIdx( dh, idx1 );
}


Coord3 Well::Track::coordAfterIdx( float dh, int idx1 ) const
{
    const int idx2 = idx1 + 1;
    const double d1 = (double)( dh - dah_[idx1] );
    const double d2 = (double)( dah_[idx2] - dh );
    const Coord3& c1 = pos_[idx1];
    const Coord3& c2 = pos_[idx2];
    const double f =  1. / (d1 + d2);
    return Coord3( f * ( d1 * c2.x + d2 * c1.x ),
		   f * ( d1 * c2.y + d2 * c1.y ),
		   f * ( d1 * c2.z + d2 * c1.z ) );
}


float Well::Track::getDahForTVD( double z, float prevdah ) const
{
    const bool haveprevdah = !mIsUdf(prevdah);
    const int sz = dah_.size();
    if ( sz < 1 )
	return mUdf(float);

    if ( zistime_ )
    {
	pErrMsg("getDahForTVD called for time well");
	const float res = haveprevdah ? prevdah : dah_[0];
	return res;
    }

    static const double eps = 1e-3; // do not use lower for float precision
    static const double epsf = 1e-3f; // do not use lower for float precision
    if ( sz == 1 )
	return mIsEqual(z,pos_[0].z,eps) ? dah_[0] : mUdf(float);

    const Interval<double> zrange = zRangeD();
    if ( !zrange.includes(z,false) )
    {
	if ( z < pos_[0].z && dah_[0] > epsf )
	{
	    const float retdah = z + getKbElev();
	    return retdah > -1*epsf ? retdah : mUdf(float);
	}

	return mUdf(float);
    }

#define mZInRg() \
    (zrg.start-eps < z  && zrg.stop+eps  > z) \
 || (zrg.stop-eps  < z  && zrg.start+eps > z)

    Interval<double> zrg( zrange.start, 0 );
    int idxafter = -1;
    for ( int idx=1; idx<sz; idx++ )
    {
	if ( !haveprevdah || prevdah+epsf < dah_[idx] )
	{
	    zrg.stop = pos_[idx].z;
	    if ( mZInRg() )
		{ idxafter = idx; break; }
	}
	zrg.start = zrg.stop;
    }
    if ( idxafter < 1 )
	return mUdf(float);

    const int idx1 = idxafter - 1;
    const int idx2 = idxafter;
    const double z1 = pos_[idx1].z;
    const double z2 = pos_[idx2].z;
    const double dah1 = mCast(double,dah_[idx1]);
    const double dah2 = mCast(double,dah_[idx2]);
    const double zdiff = z2 - z1;
    const double res = ( (z-z1) * dah2 + (z2-z) * dah1 ) / zdiff;
    return mIsZero(zdiff,eps) ? dah_[idx2] : mCast( float, res );
}


float Well::Track::getDahForTVD( float z, float prevdah ) const
{
    return getDahForTVD( (double)z, prevdah );
}


float Well::Track::nearestDah( const Coord3& posin ) const
{
    if ( dah_.isEmpty() )
	return 0;
    if ( dah_.size() < 2 )
	return dah_[0];

    const double zfac = zistime_ ? 2000. : 1.;
    Coord3 curpos(posin); curpos.z *= zfac;
    int startidx = 0;
    Coord3 actualboundstart = getPos( dah_[startidx] );
    actualboundstart.z *= zfac;
    Coord3 actualboundstop = getPos( dah_[startidx+1] );
    actualboundstop.z *= zfac;
    Coord3 curposonline;
    for ( int idx=0; idx<dah_.size()-1; idx++ )
    {
	Coord3 boundposstart = getPos( dah_[idx] ); boundposstart.z *= zfac;
	Coord3 boundposstop = getPos( dah_[idx+1] ); boundposstop.z *= zfac;
	Vector3 dir = boundposstop-boundposstart;
	Line3 newline(boundposstop,dir);

	Interval<float> zintrvl( mCast(float,boundposstart.z),
				       mCast(float,boundposstop.z) );
	if ( zintrvl.includes(curpos.z,true) )
	{
	    Coord3 posonline = newline.getPoint(newline.closestPoint(curpos));
	    if ( posonline.isDefined() )
	    {
		curposonline = posonline;
		actualboundstart = boundposstart;
		actualboundstop = boundposstop;
		startidx = idx;
	    }
	}
    }

    double sqrdisttostart = actualboundstart.sqDistTo( curposonline );
    double sqrdisttostop = actualboundstop.sqDistTo( curposonline );

    const double distfrmstrart = Math::Sqrt( sqrdisttostart );
    const double disttoend = Math::Sqrt( sqrdisttostop );
    const double dahnear = mCast(double,dah_[startidx]);
    const double dahsec = mCast(double,dah_[startidx+1]);
    double res = ( distfrmstrart*dahsec+disttoend*dahnear )/
					  ( distfrmstrart+disttoend );
    return mCast( float, res );
}


bool Well::Track::alwaysDownward() const
{
    if ( size() < 2 )
	return size();

    double prevz = pos_[0].z;
    for ( int idx=1; idx<pos_.size(); idx++ )
    {
	double curz = pos_[idx].z;
	if ( curz <= prevz )
	    return false;

	prevz = curz;
    }

    return true;
}

#define cDistTol 0.5f
void Well::Track::toTime( const Data& wd )
{
    const Track& track = wd.track();
    const D2TModel* d2t = wd.d2TModel();
    if ( track.isEmpty() )
	return;

    TimeDepthModel replvelmodel;
    const double srddepth = -1. * SI().seismicReferenceDatum();
    const float dummythickness = 1000.f;
    TypeSet<float> replveldepths, replveltimes;
    replveldepths += mCast(float,srddepth) - dummythickness;
    replveltimes += -2.f * dummythickness / wd.info().replvel;
    replveldepths += mCast(float,srddepth);
    replveltimes += 0.f;
    replvelmodel.setModel( replveldepths.arr(), replveltimes.arr(),
			   replveldepths.size() );

    TimeDepthModel dtmodel;
    if ( d2t && !d2t->getTimeDepthModel(wd,dtmodel) )
	return;

    TypeSet<float> newdah;
    TypeSet<Coord3> newpos;
    newdah += track.dah( 0 );
    newpos += track.pos( 0 );

    float prevdah = newdah[0];
    for ( int trckidx=1; trckidx<track.size(); trckidx++ )
    {
	const float curdah = track.dah( trckidx );
	const float dist = curdah - prevdah;
	if ( dist > cDistTol )
	{
	    const int nrchunks = mCast( int, dist / cDistTol );
	    const float step = dist / mCast(float, nrchunks );
	    StepInterval<float> dahrange( prevdah, curdah, step );
	    for ( int idx=1; idx<dahrange.nrSteps(); idx++ )
	    {
		const float dahsegment = dahrange.atIndex( idx );
		newdah += dahsegment;
		newpos += track.getPos( dahsegment );
	    }
	}

	newdah += curdah;
	newpos += track.pos( trckidx );
	prevdah = curdah;
    }

    // Copy the extended set into the new track definition
    dah_ = newdah;
    pos_ = newpos;
    // Now, convert to time
    for ( int idx=0; idx<dah_.size(); idx++ )
    {
	double& depth = pos_[idx].z;
	const bool abovesrd = depth < srddepth;
	if ( !abovesrd && !d2t )
	{
	    depth = mUdf(double);
	    continue; //Should never happen
	}

	depth = mCast( double, abovesrd ? replvelmodel.getTime( (float)depth )
					: dtmodel.getTime( (float)depth ) );
    }

    zistime_ = true;
}
