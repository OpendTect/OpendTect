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

    bool			setWellID(const MultiID&);
    bool			isOK() const override;

protected:
				~WellT2DTransform();
private:

    bool			usePar(const IOPar&) override;
    void			fillPar(IOPar&) const override;

    void			transformTrc(const TrcKey&,
					  const SamplingData<float>&,
					  int sz,float* res) const override;
    void			transformTrcBack(const TrcKey&,
					      const SamplingData<float>&,
					      int sz,float* res) const override;
    void			doTransform(const SamplingData<float>& sd,
					    const ZDomain::Info& sdzinfo,
					    int sz,float*) const;

    bool			needsVolumeOfInterest() const override
				{ return false; }
    bool			canTransformSurv(OD::GeomSystem) const override
				{ return true; }

    bool			calcDepths();

    ZSampling			getWorkZrg(const ZSampling&,
					   const ZDomain::Info& from,
				       const ZDomain::Info& to) const override;

    RefMan<Well::Data>		data_;
    TimeDepthModel		tdmodel_;

};
