/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/

static const char* rcsID = "$Id: well.cc,v 1.29 2004-05-27 11:56:11 bert Exp $";

#include "welldata.h"
#include "welltrack.h"
#include "welllog.h"
#include "welllogset.h"
#include "welld2tmodel.h"
#include "wellmarker.h"
#include "finding.h"
#include "iopar.h"

const char* Well::Info::sKeyuwid	= "Unique Well ID";
const char* Well::Info::sKeyoper	= "Operator";
const char* Well::Info::sKeystate	= "State";
const char* Well::Info::sKeycounty	= "County";
const char* Well::Info::sKeycoord	= "Surface coordinate";
const char* Well::Info::sKeyelev	= "Surface elevation";
const char* Well::D2TModel::sKeyTimeWell = "=Time";
const char* Well::D2TModel::sKeyDataSrc	= "Data source";
const char* Well::Marker::sKeyDah	= "Depth along hole";
const char* Well::Log::sKeyUnitLbl	= "Unit of Measure";


float Well::DahObj::dahStep( bool ismin ) const
{
    const int sz = dah_.size();
    if ( sz < 2 ) return mUndefValue;

    float res = dah_[1] - dah_[0];
    int nrvals = 1;
    for ( int idx=2; idx<sz; idx++ )
    {
	float val = dah_[idx] - dah_[idx-1];
	if ( mIS_ZERO(val) )
	    continue;

	if ( !ismin )
	    res += val;
	else
	{
	    if ( val < res )
		res = val;
	}
	nrvals++;
    }

    if ( !ismin ) res /= nrvals; // average
    return mIS_ZERO(res) ? mUndefValue : res;
}


Well::Data::Data( const char* nm )
    : info_(nm)
    , track_(*new Well::Track)
    , logs_(*new Well::LogSet)
    , d2tmodel_(0)
    , markerschanged(this)
    , d2tchanged(this)
{
}


Well::Data::~Data()
{
    delete &track_;
    delete &logs_;
    delete d2tmodel_;
}


void Well::Data::setD2TModel( D2TModel* d )
{
    delete d2tmodel_;
    d2tmodel_ = d;
}


Well::LogSet::~LogSet()
{
    deepErase( logs );
}


void Well::LogSet::add( Well::Log* l )
{
    if ( !l ) return;

    logs += l;
    updateDahIntv( *l );;
}


void Well::LogSet::updateDahIntv( const Well::Log& wl )
{
    if ( !wl.size() ) return;

    if ( mIsUndefined(dahintv.start) )
	{ dahintv.start = wl.dah(0); dahintv.stop = wl.dah(wl.size()-1); }
    else
    {
	if ( dahintv.start > wl.dah(0) )
	    dahintv.start = wl.dah(0);
	if ( dahintv.stop < wl.dah(wl.size()-1) )
	    dahintv.stop = wl.dah(wl.size()-1);
    }
}


void Well::LogSet::updateDahIntvs()
{
    for ( int idx=0; idx<logs.size(); idx++ )
	updateDahIntv( *logs[idx] );
}


int Well::LogSet::indexOf( const char* nm ) const
{
    for ( int idx=0; idx<logs.size(); idx++ )
    {
	const Log& l = *logs[idx];
	if ( l.name() == nm )
	    return idx;
    }
    return -1;
}


Well::Log* Well::LogSet::remove( int idx )
{
    Log* l = logs[idx]; logs -= l;
    ObjectSet<Well::Log> tmp( logs );
    logs.erase(); init();
    for ( int idx=0; idx<tmp.size(); idx++ )
	add( tmp[idx] );
    return l;
}


Well::Log& Well::Log::operator =( const Well::Log& l )
{
    if ( &l != this )
    {
	setName( l.name() );
	dah_ = l.dah_; val_ = l.val_;
	range_ = l.range_; selrange_ = l.selrange_;
	displogrthm_ = l.displogrthm_;
    }
    return *this;
}


float Well::Log::getValue( float dh ) const
{
    int idx1;
    if ( findFPPos(dah_,dah_.size(),dh,-1,idx1) )
	return val_[idx1];
    else if ( idx1 < 0 || idx1 == dah_.size()-1 )
	return mUndefValue;

    const int idx2 = idx1 + 1;
    const float v1 = val_[idx1];
    const float v2 = val_[idx2];
    const float d1 = dh - dah_[idx1];
    const float d2 = dah_[idx2] - dh;
    if ( mIsUndefined(v1) )
	return d1 > d2 ? v2 : mUndefValue;
    if ( mIsUndefined(v2) )
	return d2 > d1 ? v1 : mUndefValue;

    return ( d1*val_[idx2] + d2*val_[idx1] ) / (d1 + d2);
}


void Well::Log::addValue( float z, float val )
{
    if ( !mIsUndefined(val) ) 
    {
	if ( val < range_.start ) range_.start = val;
	if ( val > range_.stop ) range_.stop = val;
	assign( selrange_, range_ );
    }

    dah_ += z; 
    val_ += val;
}


void Well::Log::setSelValueRange( const Interval<float>& newrg )
{
    assign( selrange_, newrg );
}


Well::Track& Well::Track::operator =( const Track& t )
{
    if ( &t != this )
    {
	setName( t.name() );
	dah_ = t.dah_;
	pos_ = t.pos_;
    }
    return *this;
}


Coord3 Well::Track::getPos( float dh ) const
{
    int idx1;
    if ( findFPPos(dah_,dah_.size(),dh,-1,idx1) )
	return pos_[idx1];
    else if ( idx1 < 0 || idx1 == dah_.size()-1 )
	return Coord3(0,0,0);

    return coordAfterIdx( dh, idx1 );
}


Coord3 Well::Track::coordAfterIdx( float dh, int idx1 ) const
{
    const int idx2 = idx1 + 1;
    const float d1 = dh - dah_[idx1];
    const float d2 = dah_[idx2] - dh;
    const Coord3& c1 = pos_[idx1];
    const Coord3& c2 = pos_[idx2];
    const float f = 1. / (d1 + d2);
    return Coord3( f * (d1 * c2.x + d2 * c1.x), f * (d1 * c2.y + d2 * c1.y),
		   f * (d1 * c2.z + d2 * c1.z) );
}


float Well::Track::getDahForTVD( float z, float prevdah ) const
{
    bool haveprevdah = !mIsUndefined(prevdah);
    int foundidx = -1;
    for ( int idx=0; idx<pos_.size(); idx++ )
    {
	const Coord3& c = pos_[idx];
	float cz = pos_[idx].z;
	if ( haveprevdah && prevdah-1e-4 > dah_[idx] )
	    continue;
	if ( pos_[idx].z + 1e-4 > z )
	    { foundidx = idx; break; }
    }
    if ( foundidx < 1 )
	return foundidx ? mUndefValue : dah_[0];

    const int idx1 = foundidx - 1;
    const int idx2 = foundidx;
    float z1 = pos_[idx1].z;
    float z2 = pos_[idx2].z;
    float dah1 = dah_[idx1];
    float dah2 = dah_[idx2];
    return ((z-z1) * dah2 + (z2-z) * dah1) / (z2-z1);
}


bool Well::Track::alwaysDownward() const
{
    if ( pos_.size() < 2 )
	return pos_.size();

    float prevz = pos_[0].z;
    for ( int idx=1; idx<pos_.size(); idx++ )
    {
	float curz = pos_[idx].z;
	if ( curz < prevz )
	    return false;
	prevz = curz;
    }

    return true;
}


void Well::Track::toTime( const D2TModel& d2t )
{
    TypeSet<float> newdah;
    TypeSet<Coord3> newpos;

    // We need to collect control points from both the track and the d2t model
    // because both will be 'bend' points in time

    // First, get the dahs + positions from both - in depth.
    // We'll start with the first track point
    int d2tidx = 0;
    float curdah = dah_[0];
    while ( d2tidx < d2t.size() && d2t.dah(d2tidx) < curdah + mEPSILON )
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
	pt.z = d2t.getTime( dah_[idx] );
    }
}


Well::D2TModel& Well::D2TModel::operator =( const Well::D2TModel& d2t )
{
    if ( &d2t != this )
    {
	setName( d2t.name() );
	desc = d2t.desc; datasource = d2t.datasource;
	dah_ = d2t.dah_; t_ = d2t.t_;
    }
    return *this;
}


float Well::D2TModel::getTime( float dh ) const
{
    int idx1;
    if ( findFPPos(dah_,dah_.size(),dh,-1,idx1) )
	return t_[idx1];
    else if ( dah_.size() < 2 )
	return mUndefValue;
    else if ( idx1 < 0 || idx1 == dah_.size()-1 )
    {
	// Extrapolate. Is this correct?
	int idx0 = idx1 < 0 ? 1 : idx1;
	const float v = (dah_[idx0] - dah_[idx0-1]) / (t_[idx0] - t_[idx0-1]);
	idx0 = idx1 < 0 ? 0 : idx1;
	return t_[idx0] + ( dh - dah_[idx0] ) / v;
    }

    const int idx2 = idx1 + 1;
    const float d1 = dh - dah_[idx1];
    const float d2 = dah_[idx2] - dh;
    return (d1 * t_[idx2] + d2 * t_[idx1]) / (d1 + d2);
}


float Well::D2TModel::getVelocity( float dh ) const
{
    if ( dah_.size() < 2 ) return mUndefValue;

    int idx1;
    findFPPos( dah_, dah_.size(), dh, -1, idx1 );
    if ( idx1 < 1 )
	idx1 = 1;
    else if ( idx1 > dah_.size()-1 )
	idx1 = dah_.size() - 1;

    int idx0 = idx1 - 1;
    return (dah_[idx1] - dah_[idx0]) / (t_[idx1] - t_[idx0]);
}


#define mName "Well name"

void Well::Info::fillPar(IOPar& par) const
{
    par.set( mName, name() );
    par.set( sKeyuwid, uwid );
    par.set( sKeyoper, oper );
    par.set( sKeystate, state );
    par.set( sKeycounty, county );

    BufferString coord;
    surfacecoord.fill( coord.buf() );
    par.set( sKeycoord, coord );

    par.set( sKeyelev, surfaceelev );
}

void Well::Info::usePar(const IOPar& par)
{
    setName( par[mName] );
    par.get( sKeyuwid, uwid );
    par.get( sKeyoper, oper );
    par.get( sKeystate, state );
    par.get( sKeycounty, county );

    BufferString coord;
    par.get( sKeycoord, coord );
    surfacecoord.use( coord );

    par.get( sKeyelev, surfaceelev );
}

