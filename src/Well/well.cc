/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/

static const char* rcsID = "$Id: well.cc,v 1.7 2003-08-25 15:10:12 bert Exp $";

#include "welldata.h"
#include "welltrack.h"
#include "welllog.h"
#include "welllogset.h"
#include "welld2tmodel.h"
#include "wellmarker.h"
#include "finding.h"

const char* Well::Info::sKeyuwid	= "Unique Well ID";
const char* Well::Info::sKeyoper	= "Operator";
const char* Well::Info::sKeystate	= "State";
const char* Well::Info::sKeycounty	= "County";
const char* Well::Info::sKeycoord	= "Surface coordinate";
const char* Well::Info::sKeyelev	= "Surface elevation";
const char* Well::D2TModel::sKeyTimeWell = "=Time";
const char* Well::D2TModel::sKeyDataSrc	= "Data source";
const char* Well::Marker::sKeyDah	= "Depth along hole";


Well::Data::Data( const char* nm )
    	: info_(nm)
    	, track_(*new Well::Track)
    	, logs_(*new Well::LogSet)
    	, d2tmodel_(0)
{
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

    if ( dahintv.start > wl.dah(0) )
	dahintv.start = wl.dah(0);
    if ( dahintv.stop < wl.dah(wl.size()-1) )
	dahintv.stop = wl.dah(wl.size()-1);
}


void Well::LogSet::updateDahIntvs()
{
    for ( int idx=0; idx<logs.size(); idx++ )
	updateDahIntv( *logs[idx] );
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


float Well::Log::getValue( float dh ) const
{
    int idx1;
    if ( findFPPos(dah_,dah_.size(),dh,-1,idx1) )
	return val_[idx1];
    else if ( idx1 < 0 || idx1 == dah_.size()-1 )
	return mUndefValue;

    const int idx2 = idx1 + 1;
    const float d1 = dh - dah_[idx1];
    const float d2 = dah_[idx2] - dh;
    return d1 * val_[idx2] + d2 * val_[idx1] / (d1 + d2);
}


Coord3 Well::Track::getPos( float dh ) const
{
    int idx1;
    if ( findFPPos(dah_,dah_.size(),dh,-1,idx1) )
	return pos_[idx1];
    else if ( idx1 < 0 || idx1 == dah_.size()-1 )
	return Coord3(0,0,0);

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
    return ((z-z1) * dah2 + (z2-z) * dah1) / (dah2 + dah1);
}


float Well::D2TModel::getTime( float dh ) const
{
    int idx1;
    if ( findFPPos(dah_,dah_.size(),dh,-1,idx1) )
	return t_[idx1];
    else if ( idx1 < 0 || idx1 == dah_.size()-1 )
	return mUndefValue;

    const int idx2 = idx1 + 1;
    const float d1 = dh - dah_[idx1];
    const float d2 = dah_[idx2] - dh;
    //TODO not time-correct
    return (d1 * t_[idx2] + d2 * t_[idx1]) / (d1 + d2);
}
