#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellmod.h"

#include "timedepthmodel.h"
#include "welldata.h"
#include "zaxistransform.h"


/*!
\brief Time to depth transform for wells.
*/

mExpClass(Well) WellT2DTransform : public ZAxisTransform
{
mODTextTranslationClass(WellT2DTransform)
public:

    mDefaultFactoryInstantiation( ZAxisTransform, WellT2DTransform,
				  "WellT2D", toUiString(sFactoryKeyword()));

				WellT2DTransform();
				WellT2DTransform(const MultiID&);

    bool			isOK() const override;
    void			transformTrc(const TrcKey&,
					  const SamplingData<float>&,
					  int sz,float* res) const override;
    void			transformTrcBack(const TrcKey&,
					      const SamplingData<float>&,
					      int sz,float* res) const override;
    bool			canTransformSurv(OD::GeomSystem) const override
				{ return true; }

    float			getGoodZStep() const override;
    Interval<float>		getZInterval(bool time) const override;
    bool			needsVolumeOfInterest() const override
				{ return false; }

    bool			setWellID(const MultiID&);

    void			fillPar(IOPar&) const override;
    bool			usePar(const IOPar&) override;

protected:

				~WellT2DTransform();

    RefMan<Well::Data>		data_;
    TimeDepthModel		tdmodel_;

    Interval<float>		getZRange(bool time) const;

    bool			calcDepths();
    void			doTransform(const SamplingData<float>&,
					    int sz,float*,bool) const;
};
