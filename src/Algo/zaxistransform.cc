/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 2005
-*/

static const char* rcsID = "$Id: zaxistransform.cc,v 1.2 2005-10-04 17:37:28 cvskris Exp $";

#include "zaxistransform.h"

#include "survinfo.h"

DefineEnumNames(ZAxisTransform,ZType,1,"Z Type")
{ "Time", "Depth", "StratDepth", 0 };


ZAxisTransform::ZAxisTransform()
{ mRefCountConstructor; }


ZAxisTransform::~ZAxisTransform()
{ }


float ZAxisTransform::transform( const Coord3& pos ) const
{ return transform( BinIDValue( SI().transform(pos), pos.z ) ); }



float ZAxisTransform::transformBack( const Coord3& pos ) const
{ return transformBack( BinIDValue( SI().transform(pos), pos.z ) ); }


ZAxisTransformFactorySet::~ZAxisTransformFactorySet()
{}


ZAxisTransform* ZAxisTransformFactorySet::create(
	const ZAxisTransform::ZType& t0, const ZAxisTransform::ZType& t1) const
{
    for ( int idx=0; idx<factories.size(); idx++ )
    {
	ZAxisTransform* res = factories[idx]( t0, t1 );
	if ( res ) return res;
	res = factories[idx]( t1, t0 );
	if ( res ) return res;
    }

    return 0;
}


void ZAxisTransformFactorySet::addFactory(
	ZAxisTransformFactory factory )
{
    factories += factory;
}


ZAxisTransformFactorySet& ZATF()
{
    static ZAxisTransformFactorySet factoryset;
    return factoryset;
}
