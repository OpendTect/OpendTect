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
    void		enableTargetSampling() override;
    bool		getTargetSampling(ZSampling&) const override;

    StringView		toDomain() const override;
    StringView		fromDomain() const override;

protected:
			uiTime2DepthZTransformBase(uiParent*,bool t2d);
			~uiTime2DepthZTransformBase();

    bool		isTimeToDepth() const		{ return t2d_; }

    virtual void	doInitGrp()			{}
    virtual void	rangeChangedCB(CallBacker*) { rangechanged_ = true; }

    uiZRangeInput*	rangefld_ = nullptr;
    bool		rangechanged_ = false;

private:
    void		initGrp(CallBacker*);

    bool		t2d_;
};
