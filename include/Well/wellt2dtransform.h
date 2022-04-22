#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          July 2010
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
