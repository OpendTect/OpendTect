/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 21-6-1996
-*/

static const char* rcsID = "$Id$";

#include "positionlist.h"


Coord2ListImpl::Coord2ListImpl()
{}


int Coord2ListImpl::nextID( int previd ) const
{
    const int sz = points_.size();
    int res = previd + 1;
    while ( res<sz )
    {
	if ( removedids_.indexOf(res)==-1 )
	    return res;

	res++;
    }

    return -1;
}


Coord Coord2ListImpl::get( int id ) const
{
    if ( id<0 || id>=points_.size() || removedids_.indexOf( id )!=-1 )
	return Coord::udf();
    else
	return points_[id];
}


void Coord2ListImpl::set( int id, const Coord& co )
{
    for ( int idx=points_.size(); idx<=id; idx++ )
    {
	removedids_ += idx;
	points_ += Coord::udf();
    }

    removedids_ -= id;
    points_[id] = co;
}


int Coord2ListImpl::add( const Coord& co )
{
    const int nrremoved = removedids_.size();
    if ( nrremoved )
    {
	const int res = removedids_[nrremoved-1];
	removedids_.remove( nrremoved-1 );
	points_[res] = co;
	return res;
    }

    points_ += co;
    return points_.size()-1;
}


void Coord2ListImpl::addValue( int idx, const Coord& co )
{
    if ( idx>=points_.size() || removedids_.indexOf(idx)!=-1 )
	add( co );
    else
	points_[idx] += co;
}


void Coord2ListImpl::remove( int id )
{
    removedids_ += id;
}


Coord3ListImpl::Coord3ListImpl()
{}


int Coord3ListImpl::nextID( int previd ) const
{
    const int sz = coords_.size();
    int res = previd + 1;
    while ( res<sz )
    {
	if ( removedids_.indexOf(res)==-1 )
	    return res;

	res++;
    }

    return -1;
}


Coord3 Coord3ListImpl::get( int id ) const
{
    if ( id<0 || id>=coords_.size() || removedids_.indexOf( id )!=-1 )
	return Coord3::udf();
    else
	return coords_[id];
}


void Coord3ListImpl::set( int id, const Coord3& coord3 )
{
    for ( int idx=coords_.size(); idx<=id; idx++ )
    {
	removedids_ += idx;
	coords_ += Coord3::udf();
    }

    removedids_ -= id;
    coords_[id] = coord3;
}


int Coord3ListImpl::add( const Coord3& coord3 )
{
    const int nrremoved = removedids_.size();
    if ( nrremoved )
    {
	const int res = removedids_[nrremoved-1];
	removedids_.remove( nrremoved-1 );
	coords_[res] = coord3;
	return res;
    }

    coords_ += coord3;
    return coords_.size()-1;
}


void Coord3ListImpl::addValue( int idx, const Coord3& co )
{
    if ( idx>=coords_.size() || removedids_.indexOf(idx)!=-1 )
	add( co );
    else
	coords_[idx] += co;
}


bool Coord3ListImpl::isDefined( int idx ) const
{
    return idx >= 0 && idx < coords_.size() && coords_[idx] != Coord3::udf(); 
}


void Coord3ListImpl::remove( int id )
{
     removedids_ += id;
}
