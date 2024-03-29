#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "earthmodelmod.h"

#include "emhorizon.h"
#include "zaxistransform.h"

namespace EM
{

mGlobal(EarthModel) const ZDomain::Info& flattenedZDomain();

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
				  ::toUiString(sFactoryKeyword()));

			HorizonZTransform();

    bool		isOK() const override;

    void		setHorizon(const Horizon&);
    void		setFlatZValue(float);
    bool		isReferenceHorizon(const MultiID& horid,
					   float& refz) const override;

    Interval<float>	getDepthRange() const		{ return depthrange_; }
    NotifierAccess*	changeNotifier() override	{ return &change_; }

    static const char*	sKeyHorizonID()		{ return "Horizon"; }
    static const char*	sKeyReferenceZ()	{ return "Reference Z"; }

protected:
			~HorizonZTransform();
private:

    bool		usePar(const IOPar&) override;
    void		fillPar(IOPar&) const override;

    void		transformTrc(const TrcKey&,const SamplingData<float>&,
				     int sz,float* res) const override;
    void		transformTrcBack(const TrcKey&,
					 const SamplingData<float>&,
					 int sz,float* res) const override;
    void		doTransform(const TrcKey&,const SamplingData<float>&,
				    const ZDomain::Info& sdzinfo,
				    int sz,float* res) const;

    bool		canTransformSurv(OD::GeomSystem) const override
			{ return true; }

    void		calculateHorizonRange();
    void		horChangeCB(CallBacker*);
    bool		getTopBottom(const TrcKey&,float&top,float&bot) const;
    ZSampling		getWorkZSampling(const ZSampling&,
					const ZDomain::Info& from,
					const ZDomain::Info& to) const override;
    ZSampling		getModelZSampling() const override;

    ConstRefMan<Horizon> horizon_;
    Interval<float>	depthrange_;
    float		flatzval_		= 0.f;
    bool		horchanged_		= false;
    Notifier<HorizonZTransform> change_;

};

} // namespace EM
