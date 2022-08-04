#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert/Nanne
 Date:		Aug 2007
________________________________________________________________________

-*/

#include "uizaxistransform.h"

class uiZRangeInput;

mExpClass(uiTools) uiTime2DepthZTransformBase : public uiZAxisTransform
{
public:
    FixedString toDomain() const override;
    FixedString fromDomain() const override;

    void		enableTargetSampling() override;
    bool		getTargetSampling(StepInterval<float>&) const override;

protected:
			uiTime2DepthZTransformBase(uiParent*,bool t2d);
			~uiTime2DepthZTransformBase();

    void		finalizeDoneCB(CallBacker*);
    virtual void	rangeChangedCB(CallBacker*) { rangechanged_ = true; }

    bool		t2d_;
    uiZRangeInput*	rangefld_;
    bool		rangechanged_;
};

