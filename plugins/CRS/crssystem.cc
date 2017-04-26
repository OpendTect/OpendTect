/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : April 2017
-*/


#include "crssystem.h"

Coords::ProjectionBasedSystem::ProjectionBasedSystem()
{}


bool Coords::ProjectionBasedSystem::isOK() const
{ return false; }

bool Coords::ProjectionBasedSystem::geographicTransformOK() const
{ return false; }

LatLong Coords::ProjectionBasedSystem::toGeographicWGS84( const Coord& ) const
{ return LatLong::udf(); }

Coord Coords::ProjectionBasedSystem::fromGeographicWGS84( const LatLong& ) const
{ return Coord::udf(); }

uiString Coords::ProjectionBasedSystem::toUiString( const Coord& ) const
{ return uiString::emptyString(); }

BufferString Coords::ProjectionBasedSystem::toString( const Coord&,
						      bool withsystem ) const
{ return BufferString::empty(); }

Coord Coords::ProjectionBasedSystem::fromString( const char* ) const
{ return Coord::udf(); }

bool Coords::ProjectionBasedSystem::isOrthogonal() const
{ return false; }

bool Coords::ProjectionBasedSystem::isFeet() const
{ return false; }

bool Coords::ProjectionBasedSystem::isMeter() const
{ return false; }

bool Coords::ProjectionBasedSystem::usePar( const IOPar& )
{ return false; }

void Coords::ProjectionBasedSystem::fillPar( IOPar& ) const
{}
