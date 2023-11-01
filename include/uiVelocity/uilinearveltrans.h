#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivelocitymod.h"
#include "uitime2depthzaxistrans.h"

class uiGenInput;
class uiZRangeInput;

namespace Vel
{

mExpClass(uiVelocity) uiLinearVelTransform : public uiTime2DepthZTransformBase
{ mODTextTranslationClass(uiLinearVelTransform);
public:
    static void		initClass();
			uiLinearVelTransform(uiParent*,bool t2d);
			~uiLinearVelTransform();

    ZAxisTransform*	getSelection() override;

    StringView		toDomain() const override;
    StringView		fromDomain() const override;
    bool		canBeField() const override	{ return true; }

protected:
    static uiZAxisTransform*	createInstance(uiParent*,const char*,
					       const char*);
    bool			acceptOK() override;
    void			initGrpCB(CallBacker*);
    void			velChangedCB(CallBacker*);

    uiGenInput*			velfld_;
    uiGenInput*			gradientfld_;
};

} // namespace Vel
