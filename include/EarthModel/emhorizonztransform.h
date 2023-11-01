#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "zaxistransform.h"

namespace EM
{
class Horizon;

/*!
\brief Z-transform that flattens a horizon. Everything else will also be
flattened accordingly. In case of reverse faulting, the area between the two
patches will not be included.
*/

mExpClass(EarthModel) HorizonZTransform : public ZAxisTransform
{
mODTextTranslationClass(HorizonZTransform)
public:
    mDefaultFactoryInstantiation( ZAxisTransform, HorizonZTransform,
				  "HorizonZTransform",
				  toUiString(sFactoryKeyword()));

    static const char*	sKeyHorizonID()		{ return "Horizon"; }

			HorizonZTransform();
    void		setHorizon(const Horizon&);
    void		setFlatZValue(float);

    void		transformTrc(const TrcKey&,const SamplingData<float>&,
				  int sz,float* res) const override;
    void		transformTrcBack(const TrcKey&,
				  const SamplingData<float>&,
				  int sz,float* res) const override;
    bool		canTransformSurv(OD::GeomSystem) const override
							{ return true; }

    Interval<float>	getZInterval(bool from) const override;
    ZSampling		getZInterval(bool from,bool makenice) const;

    mDeprecatedDef
    float		getZIntervalCenter(bool from) const override;

    bool		needsVolumeOfInterest() const override { return false; }

    bool		isReferenceHorizon(const MultiID& horid,
					   float& refz) const override;

    Interval<float>	getDepthRange() const		{ return depthrange_; }
    NotifierAccess*	changeNotifier() override	{ return &change_; }

    void		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;

protected:
			~HorizonZTransform();

    void		doTransform(const TrcKey&,const SamplingData<float>&,
				    const ZDomain::Info& sdzinfo,
				    int sz,float* res) const;
    void		calculateHorizonRange();
    void		horChangeCB( CallBacker* );
    bool		getTopBottom(const TrcKey&,float&top,float&bot) const;

    ZSampling		getZInterval(const ZSampling&,
				     const ZDomain::Info& from,
				     const ZDomain::Info& to,
				     bool makenice=true) const;
    ZSampling		getWorkZSampling(const ZSampling&,
					 const ZDomain::Info& from,
					 const ZDomain::Info& to) const;

    const Horizon*	horizon_;
    Interval<float>	depthrange_;
    float		flatzval_		= 0.f;
    bool		horchanged_		= false;
    Notifier<HorizonZTransform> change_;

};

} // namespace EM
