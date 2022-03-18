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

    bool			isOK() const;
    void			transformTrc(const TrcKey&,
					  const SamplingData<float>&,
					  int sz,float* res) const;
    void			transformTrcBack(const TrcKey&,
					      const SamplingData<float>&,
					      int sz,float* res) const;
    bool			canTransformSurv(OD::GeomSystem) const
				{ return true; }

    float			getGoodZStep() const;
    Interval<float>		getZInterval(bool time) const;
    bool			needsVolumeOfInterest() const { return false; }

    bool			setWellID(const MultiID&);

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

protected:

				~WellT2DTransform();

    RefMan<Well::Data>		data_;
    TimeDepthModel		tdmodel_;

    Interval<float>		getZRange(bool time) const;

    bool			calcDepths();
    void			doTransform(const SamplingData<float>&,
					    int sz,float*,bool) const;
};
