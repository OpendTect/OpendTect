#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "velocitymod.h"
#include "samplingdata.h"
#include "thread.h"
#include "velocityfunction.h"
#include "velocitycalc.h"
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

    bool		moveTo(const BinID&) override;
    StepInterval<float> getAvailableZ() const override;
    StepInterval<float> getLoadedZ() const;

    void		enableExtrapolation(bool);
    void		setStatics(float t,float vel);
			//!<Only used with RMS velocities extrapolation

protected:

    bool		computeVelocity(float z0, float dz, int nr,
					float* res ) const override;

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
				mDefaultFactoryInstanciationBase(
				"Velocity volume", 
				toUiString(sFactoryKeyword()));
				VolumeFunctionSource();

    const VelocityDesc&		getDesc() const override { return desc_; }

    bool			zIsTime() const;
    bool			setFrom(const MultiID& vel);

    VolumeFunction*		createFunction(const BinID&) override;

    void			getAvailablePositions(
						BinIDValueSet&) const override;
    bool			getVel(const BinID&,SamplingData<float>&,
				       TypeSet<float>&);

    static const char*		sKeyZIsTime() { return "Z is Time"; }

protected:
    static FunctionSource*	create(const MultiID&);
				~VolumeFunctionSource();

    SeisTrcReader*		getReader();

    ObjectSet<SeisTrcReader>	velreader_;
    ObjectSet<const void>	threads_;

    Threads::Lock		readerlock_;
    bool			zit_;
    VelocityDesc		desc_;
};

} // namespace Vel
