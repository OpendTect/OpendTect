#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "velocitymod.h"

#include "thread.h"
#include "velocityfunction.h"
#include "uistring.h"


class BinIDValueSet;
class SeisTrcReader;

namespace Vel
{

class VolumeFunctionSource;


/*!VelocityFunction that gets its information from a Velocity Volume. */

mExpClass(Velocity) VolumeFunction : public Function
{ mODTextTranslationClass(VolumeFunction);
public:
				VolumeFunction(VolumeFunctionSource&);
				~VolumeFunction();

    bool			moveTo(const BinID&) override;
    ZSampling			getAvailableZ() const override;
    ZSampling			getLoadedZ() const;

    void			enableExtrapolation(bool);
    void			setStatics(float t,float vel);
				//!<Only used with RMS velocities extrapolation

protected:

    bool			computeVelocity(float z0,float dz,int sz,
						float* res) const override;

    SamplingData<double>	velsampling_;
    TypeSet<double>		vel_;

    bool			extrapolate_ = false;
    float			statics_ = 0.f;
    float			staticsvel_ = 0.f;
};


mExpClass(Velocity) VolumeFunctionSource : public FunctionSource
{ mODTextTranslationClass(VolumeFunctionSource);
public:
				mDefaultFactoryInstanciationBase(
				"Velocity volume",
				::toUiString(sFactoryKeyword()));
				VolumeFunctionSource();

    const VelocityDesc&		getDesc() const override { return desc_; }

    bool			setFrom(const MultiID& vel);

    VolumeFunction*		createFunction(const BinID&) override;

    void			getAvailablePositions(
						BinIDValueSet&) const override;
    bool			getVel(const BinID&,SamplingData<double>&,
				       TypeSet<double>&);

protected:
				~VolumeFunctionSource();

    SeisTrcReader*		getReader();

    static FunctionSource*	create(const MultiID&);

    ObjectSet<SeisTrcReader>	velreader_;
    ObjectSet<const void>	threads_;

    Threads::Lock		readerlock_;
    VelocityDesc&		desc_;
};

} // namespace Vel
