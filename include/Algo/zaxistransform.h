#ifndef zaxistransform_h
#define zaxistransform_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          October 2006
 RCS:           $Id: zaxistransform.h,v 1.7 2005-11-01 23:34:49 cvskris Exp $
________________________________________________________________________

-*/

#include "enums.h"
#include "position.h"
#include "ranges.h"
#include "refcount.h"
#include "samplingdata.h"

class Coord3;
class CubeSampling;

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

    virtual int			addVolumeOfInterest( const CubeSampling& );
    virtual void		setVolumeOfInterest( int, const CubeSampling& );
    virtual void		removeVolumeOfInterest( int );
    virtual bool		loadDataIfMissing( int ) { return false; }
    				
    virtual ZType		getFromZType() const 			= 0;
    virtual ZType		getToZType() const 			= 0;

    virtual float		transform( const BinIDValue& ) const	= 0;
    float			transform( const Coord3& ) const;
    virtual float		transformBack( const BinIDValue& ) const= 0;
    float			transformBack( const Coord3& ) const;

    virtual Interval<float>	getZInterval( bool from ) const		= 0;
    				/*!<\returns the z interval in either to
				     or from domain. */

    NotifierAccess*		changeNotifier() { return 0; }
};


class ZAxisTransformSampler
{
public:
    				ZAxisTransformSampler( const ZAxisTransform&,
				   bool back, const BinID& bid,
				   const SamplingData<double>& );
    virtual			~ZAxisTransformSampler();
    void			setBinID( const BinID& nbid ) { bid=nbid; }

    float			operator[](int idx) const;

protected:
    const ZAxisTransform&	transform;
    bool			back;
    BinID			bid;
    const SamplingData<double>	sd;
};


typedef ZAxisTransform* (*ZAxisTransformFactory)
    	( const ZAxisTransform::ZType& from, const ZAxisTransform::ZType& to);


class ZAxisTransformFactorySet
{
public:
    				~ZAxisTransformFactorySet();
    				
    virtual ZAxisTransform*	create( const ZAxisTransform::ZType& t0,
	    				const ZAxisTransform::ZType& t1) const;
    				/*!<\note that returned transform can be in 
					  any direction. */

    void			addFactory( ZAxisTransformFactory );

private:
    TypeSet<ZAxisTransformFactory>	factories;
};


ZAxisTransformFactorySet& ZATF();


#endif
