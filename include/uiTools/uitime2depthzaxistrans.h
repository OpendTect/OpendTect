#ifndef uitime2depthzaxistrans_h
#define uitime2depthzaxistrans_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert/Nanne
 Date:		Aug 2007
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uizaxistransform.h"

class uiZRangeInput;

mExpClass(uiTools) uiTime2DepthZTransformBase : public uiZAxisTransform
{
public:
    FixedString 	toDomain() const;
    FixedString 	fromDomain() const;

    void		enableTargetSampling();
    bool		getTargetSampling(StepInterval<float>&) const;

protected:
			uiTime2DepthZTransformBase(uiParent*,bool t2d);
    void		finalizeDoneCB(CallBacker*);
    virtual void	rangeChangedCB(CallBacker*) { rangechanged_ = true; }

    bool		t2d_;
    uiZRangeInput*	rangefld_;
    bool		rangechanged_;
};

#endif

