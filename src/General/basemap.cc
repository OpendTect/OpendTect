/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2005
-*/

static const char* rcsID = "$Id: basemap.cc,v 1.1 2010-08-09 20:00:28 cvskris Exp $";

#include "basemap.h"

BaseMapObject::BaseMapObject( const char* nm )
    : NamedObject(nm)
{}


int BaseMapObject::nrShapes() const
{ return 0; }


const char* BaseMapObject::getShapeName( int ) const
{ return 0; }


void BaseMapObject::getPoints( int, TypeSet<Coord>& ) const
{ }


char BaseMapObject::connectPoints(int) const
{ return cDontConnect(); }


const Color* BaseMapObject::getColor(int) const
{ return 0; }


const OD::RGBImage* BaseMapObject::getImage(Coord& origin,Coord& p11) const
{ return 0; }


const OD::RGBImage* BaseMapObject::getPreview(int approxdiagonal) const
{ return 0; }
