#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uizaxistransform.h"

class uiZRangeInput;

mExpClass(uiTools) uiTime2DepthZTransformBase : public uiZAxisTransform
{
public:
    StringView		toDomain() const override;
    StringView		fromDomain() const override;

    void		enableTargetSampling() override;
    bool		getTargetSampling(StepInterval<float>&) const override;

protected:
			uiTime2DepthZTransformBase(uiParent*,bool t2d);
			~uiTime2DepthZTransformBase();

    bool		isTimeToDepth() const		{ return t2d_; }

    void		finalizeDoneCB(CallBacker*);
    virtual void	rangeChangedCB(CallBacker*) { rangechanged_ = true; }

    bool		t2d_;
    uiZRangeInput*	rangefld_;
    bool		rangechanged_;
};
