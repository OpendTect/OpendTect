#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiflatviewmod.h"
#include "uislicepos.h"
#include "zdomain.h"

/*!
\brief Toolbar for setting slice position _ 2D viewer.
*/

mExpClass(uiFlatView) uiSlicePos2DView : public uiSlicePos
{
public:
			uiSlicePos2DView(uiParent*,const ZDomain::Info&);

    void		setTrcKeyZSampling(const TrcKeyZSampling&);
    void		setLimitSampling(const TrcKeyZSampling&);

    const TrcKeyZSampling& getLimitSampling() const { return limitscs_; }

protected:

    SliceDir		getOrientation() const override
			    { return curorientation_; }

    void		setBoxRanges() override;
    void		setPosBoxValue() override;
    void		setStepBoxValue() override;
    void		slicePosChg(CallBacker*) override;
    void		sliceStepChg(CallBacker*) override;

    TrcKeyZSampling	limitscs_;
    SliceDir		curorientation_;
    ZDomain::Info	zdomaininfo_;
};
