#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
________________________________________________________________________


-*/

#include "velocitymod.h"
#include "samplingdata.h"
#include "thread.h"
#include "velocityfunction.h"
#include "velocitycalc.h"
#include "uistring.h"

class BinnedValueSet;
namespace Seis { class Provider; }

namespace Vel
{

class VolumeFunctionSource;


/*!VelocityFunction that gets its information from a Velocity Volume. */

mExpClass(Velocity) VolumeFunction : public Function
{ mODTextTranslationClass(VolumeFunction);
public:
			VolumeFunction(VolumeFunctionSource&);
    bool		moveTo(const BinID&);
    StepInterval<float>	getAvailableZ() const;
    StepInterval<float>	getLoadedZ() const;

    void		enableExtrapolation(bool);
    void		setStatics(float t,float vel);
			//!<Only used with RMS velocities extrapolation

protected:

    bool		computeVelocity(float z0, float dz, int nr,
					float* res ) const;

    bool			zit_;
    SamplingData<float>		velsampling_;
    TypeSet<float>		vel_;

    bool			extrapolate_;
    float			statics_;
    float			staticsvel_;
};


mExpClass(Velocity) VolumeFunctionSource : public FunctionSource
{ mODTextTranslationClass(VolumeFunctionSource);
public:
				mDefaultFactoryInstantiationBase(
				"Velocity volume",
				toUiString(sFactoryKeyword()));
				VolumeFunctionSource();

    const VelocityDesc&		getDesc() const	{ return desc_; }

    bool			zIsTime() const;
    bool			setFrom(const DBKey& vel);

    VolumeFunction*		createFunction(const BinID&);

    void			getAvailablePositions(BinnedValueSet&) const;
    bool			getVel(const BinID&,SamplingData<float>&,
				       TypeSet<float>&);

    static const char*		sKeyZIsTime() { return "Z is Time"; }

protected:
    static FunctionSource*	create(const DBKey&);
				~VolumeFunctionSource();

    Seis::Provider*		getProvider(uiRetVal&);

    ObjectSet<Seis::Provider>	velprovider_;
    ObjectSet<const void>	threads_;

    Threads::Lock		providerlock_;
    bool			zit_;
    VelocityDesc		desc_;
};

} // namespace Vel
