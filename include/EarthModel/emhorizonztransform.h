#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		April 2006
 RCS:		$Id$
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
					, public CallBacker
{ mODTextTranslationClass(HorizonZTransform);
public:
    mDefaultFactoryInstantiation( ZAxisTransform, HorizonZTransform,
				  "HorizonZTransform",
				  toUiString(sFactoryKeyword()));

    static const char*	sKeyHorizonID()		{ return "Horizon"; }

			HorizonZTransform();
    void		setHorizon(const Horizon&);
    void		setFlatZValue(float);

    void		transformTrc(const TrcKey&,const SamplingData<float>&,
				  int sz,float* res) const;
    void		transformTrcBack(const TrcKey&,
				  const SamplingData<float>&,
				  int sz,float* res) const;
    bool		canTransformSurv(TrcKey::SurvID) const { return true; }

    Interval<float>	getZInterval(bool from) const;
    float		getZIntervalCenter(bool from) const;
    bool		needsVolumeOfInterest() const	{ return false; }

    Interval<float>	getDepthRange() const		{ return depthrange_; }
    NotifierAccess*	changeNotifier()		{ return &change_; }

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:
			~HorizonZTransform();
    void		calculateHorizonRange();
    void		horChangeCB( CallBacker* );
    bool		getTopBottom(const TrcKey&,float&top,float&bot) const;

    const Horizon*	horizon_;
    Interval<float>	depthrange_;
    bool		horchanged_;
    Notifier<HorizonZTransform> change_;

};

} // namespace EM

