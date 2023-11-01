#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"

#include "odcommonenums.h"
#include "veldesc.h"
#include "multiid.h"

class LinScaler;
class UnitOfMeasure;
class ZValueSeries;
namespace ZDomain { class Def; }
template <class T> class ArrayZValues;
template <class T> class ValueSeries;


namespace Vel
{

/*!
\brief Class that should be used for applying functions of the Vel namespace
*/

mExpClass(General) Worker
{ mODTextTranslationClass(Worker);
public:
			Worker(const VelocityDesc&,double srd,
			       const UnitOfMeasure* srduom);
			Worker(double v0,double dv,double srd,
			       const UnitOfMeasure* v0uom,
			       const UnitOfMeasure* srduom);
			~Worker();

    bool		convertVelocities(const ValueSeries<double>& Vin,
					  const ZValueSeries& zvals,
					  const VelocityDesc& newdesc,
					  ValueSeries<double>& Vout,
					  double t0=0.) const;
			/*!< Converts velocities between compatible types
			     May also switch the units if different */

    bool		sampleVelocities(const ValueSeries<double>& Vin,
					 const ZValueSeries& zvals_in,
					 const ZValueSeries& zvals_out,
					 ValueSeries<double>& Vout,
					 double t0=0.) const;
			/*!< Given an array of velocity in time or depth,
			  create a regularly sampled one in
			  time or depth. t0 is only used by RMS type */

    template <class AT>
    bool		calcZ(const ValueSeries<double>& Vin,
			      const ZValueSeries& zvals,
			      ArrayZValues<AT>& Zout,double t0=0.) const;
			/*!< Given an array of velocity in time or depth
			     with its sampling, compute the transformed Z
			     t0 is only used by RMS type */

    template <class AT>
    bool		getSampledZ(const ValueSeries<double>& Vin,
				const ZValueSeries& zvals_in,
				const ZValueSeries& zvals_out,
				ArrayZValues<AT>& Zout,double t0=0.) const;
			/*!< Given an array of velocity in time or depth
			     with its sampling, compute the transformed Z
			     of another Z sampling
			     t0 is only used by RMS type */

    template <class AT>
    bool		calcZLinear(const ZValueSeries& zvals,
				    ArrayZValues<AT>& Zout) const;
			/*!< Given V0,k velocitymodel, compute the transformed Z
			     corresponding to the input array
			     of times or depths*/

    static const UnitOfMeasure* getUnit(const VelocityDesc&);
    static void		setUnit(const UnitOfMeasure*,VelocityDesc&);

    static ZValueSeries* getZVals(const ZValueSeries&,double srd,
				  const UnitOfMeasure* srduom,
				  bool forward=true);

    const VelocityDesc& getDesc() const		{ return desc_; }

private:

    bool		calcZ_(const ValueSeries<double>& Vin,
			       const ZValueSeries& zvals,
			       ZValueSeries& Zout,double t0=0.) const;
    bool		calcZLinear_(const ZValueSeries& zvals,
				     ZValueSeries& Zout) const;
    bool		getSampledZ_(const ValueSeries<double>& Vin,
				const ZValueSeries& zvals_in,
				const ZValueSeries& zvals_out,
				ZValueSeries& Zout,double t0=0.) const;

    const VelocityDesc	desc_;
    double		srd_;
    double		v0_	= mUdf(double);
    double		dv_	= mUdf(double);
};


template <class AT> inline
bool Worker::calcZ( const ValueSeries<double>& Vin,
		    const ZValueSeries& zvals,
		    ArrayZValues<AT>& Zout, double t0 ) const
{
    return calcZ_( Vin, zvals, Zout, t0 );
}


template <class AT> inline
bool Worker::calcZLinear( const ZValueSeries& zvals_in,
			  ArrayZValues<AT>& zvals_out ) const
{
    return calcZLinear_( zvals_in, zvals_out );
}


template <class AT> inline
bool Worker::getSampledZ( const ValueSeries<double>& Vin,
			  const ZValueSeries& zvals_in,
			  const ZValueSeries& zvals_out,
			  ArrayZValues<AT>& Zout, double t0 ) const
{
    return getSampledZ_( Vin, zvals_in, zvals_out, Zout, t0 );
}

} // namespace Vel
