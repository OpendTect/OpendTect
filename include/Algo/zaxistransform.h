#ifndef zaxistransform_h
#define zaxistransform_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          October 2006
 RCS:           $Id: zaxistransform.h,v 1.1 2005-10-04 14:04:20 cvskris Exp $
________________________________________________________________________

-*/

#include "enums.h"
#include "refcount.h"

class BinIDValue;
class Coord3;

/*! Baseclass for z stretching in different ways. The z-stretch may be dependent
on the location (binid). The various transforms can be retrieved from factory
ZATF().
*/

class ZAxisTransform
{ mRefCountImpl(ZAxisTransform);
public:
    enum			ZType { Time, Depth, StratDepth };
    				DeclareEnumUtils( ZType );

    				ZAxisTransform();
    				
    virtual ZType		getFromZType() const 			= 0;
    virtual ZType		getToZType() const 			= 0;

    virtual float		transform( const BinIDValue& ) const	= 0;
    float			transform( const Coord3& ) const;
    virtual float		transformBack( const BinIDValue& ) const= 0;
    float			transformBack( const Coord3& ) const;
};


class ZAxisTransformFactory
{ mRefCountImpl(ZAxisTransformFactory);
public:
    				ZAxisTransformFactory();
    virtual ZAxisTransform*	create( const ZAxisTransform::ZType& from,
	    				const ZAxisTransform::ZType& to ) const;
};


class ZAxisTransformFactorySet
{
public:
    				~ZAxisTransformFactorySet();
    				
    virtual ZAxisTransform*	create( const ZAxisTransform::ZType& t0,
	    				const ZAxisTransform::ZType& t1) const;
    				/*!<\note that returned transform can be in 
					  any direction. */

    void			addFactory( const ZAxisTransformFactory* );

private:
    ObjectSet<const ZAxisTransformFactory>	factories;
};


ZAxisTransformFactorySet& ZATF();


#endif
