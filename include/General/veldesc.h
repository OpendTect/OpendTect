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
class ZValueSerie;
namespace ZDomain { class Def; }
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

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    void		setUnit(const UnitOfMeasure*);

    static void		removePars(IOPar&);

    static const char*	sKeyIsVelocity();
    static const char*	sKeyVelocityType();
    static const char*	sKeyVelocityUnit();
    static const char*	sKeyVelocityVolume();

    static uiString	getVelVolumeLabel();

    Vel::Type		type_ = Vel::Unknown;
    StaticsDesc		statics_;
    BufferString	velunit_;

    static bool		isUsable(Vel::Type,const ZDomain::Def&,uiRetVal&);

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
					  const ZValueSerie& zvals,
					  const VelocityDesc& newdesc,
					  ValueSeries<double>& Vout,
					  double t0=0.) const;
			/*!< Converts velocities between compatible types
			     May also switch the units if different */

    bool		sampleVelocities(const ValueSeries<double>& Vin,
					 const ZValueSerie& zvals_in,
					 const ZValueSerie& zvals_out,
					 ValueSeries<double>& Vout,
					 double t0=0.) const;
			/*!< Given an array of velocity in time or depth,
			  create a regularly sampled one in
			  time or depth. t0 is only used by RMS type */

    bool		calcZ(const ValueSeries<double>& Vin,
			      const ZValueSerie& zvals,
			      ValueSeries<double>& Zout,double t0=0.) const;
			/*!< Given an array of velocity in time or depth
			     with its sampling, compute the transformed Z
			     t0 is only used by RMS type */

    bool		getSampledZ(const ValueSeries<double>& Vin,
				const ZValueSerie& zvals_in,
				const ZValueSerie& zvals_out,
				ValueSeries<double>& Zout,double t0=0.) const;
			/*!< Given an array of velocity in time or depth
			     with its sampling, compute the transformed Z
			     of another Z sampling
			     t0 is only used by RMS type */

    bool		calcZLinear(const ZValueSerie& zvals,
				    ValueSeries<double>& Zout) const;
			/*!< Given V0,k velocitymodel, compute the transformed Z
			     corresponding to the input array
			     of times or depths*/

    static ZValueSerie* getZVals(const ZValueSerie&,double srd,
				 const UnitOfMeasure* srduom);

    const VelocityDesc& getDesc() const		{ return desc_; }

private:

    static bool		getReverseScaler(const ZValueSerie&,double srd,
					 LinScaler&);

    const VelocityDesc	desc_;
    double		srd_;
    double		v0_	= mUdf(double);
    double		dv_	= mUdf(double);
};

} // namespace Vel
