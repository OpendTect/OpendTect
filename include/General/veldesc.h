#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"

#include "odcommonenums.h"
#include "multiid.h"
#include "staticsdesc.h"
#include "uistring.h"

class LinScaler;
class UnitOfMeasure;
class ZValueSeries;
namespace ZDomain { class Def; }
template <class T> class ArrayZValues;
template <class T> class ValueSeries;


/*!
Specifies velocity type and statics for a velocity.

To tag a velocity volume as a velocity, this class can be used to do the work:

\code
    const VelocityDesc desc( Vel::Interval );

    PtrMan<IOObj> ioobj = IOM().get( multiid );
    desc.fillPar( ioobj->pars() );
    IOM().commitChanges( ioobj );

\endcode

*/

mExpClass(General) VelocityDesc
{ mODTextTranslationClass(VelocityDesc);
public:
			VelocityDesc();
			VelocityDesc(Vel::Type,const UnitOfMeasure* =nullptr);
			~VelocityDesc();

    bool		operator==(const VelocityDesc&) const;
    bool		operator!=(const VelocityDesc&) const;

    static bool		isUdf(Vel::Type);
    bool		isUdf() const;
    static bool		isVelocity(Vel::Type);
			//!<\returns true if not unknown or a Thomsen parameter
    bool		isVelocity() const;
			//!<\returns true if not unknown or a Thomsen parameter
    bool		isInterval() const;
    bool		isRMS() const;
    bool		isAvg() const;
    static bool		isThomsen(Vel::Type);
			//!<\returns true if not unknown or a Velocity
    bool		isThomsen() const;
			//!<\returns true if not unknown or a Velocity
    const UnitOfMeasure* getUnit() const;
    bool		hasVelocityUnit() const;

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    void		setUnit(const UnitOfMeasure*);

    static void		removePars(IOPar&);

    static const char*	sKeyVelocityType();
    static const char*	sKeyVelocityUnit();
    static const char*	sKeyVelocityVolume();

    static uiString	getVelVolumeLabel();

    Vel::Type		type_ = Vel::Unknown;
    StaticsDesc		statics_;

    static bool		isUsable(Vel::Type,const ZDomain::Def&,uiRetVal&);

private:

    BufferString	velunit_;
};


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
