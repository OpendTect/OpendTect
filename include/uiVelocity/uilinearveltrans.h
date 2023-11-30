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
private:
			uiLinearVelTransform(uiParent*,bool t2d);
			~uiLinearVelTransform();

    ZAxisTransform*	getSelection() override;
    bool		canBeField() const override	{ return true; }

    StringView		toDomain() const override;
    StringView		fromDomain() const override;

    void		doInitGrp() override;
    void		velChangedCB(CallBacker*);
    bool		acceptOK() override;

    uiGenInput*		velfld_;
    uiGenInput*		gradientfld_;

    static uiZAxisTransform*	createInstance(uiParent*,
						const uiZAxisTranformSetup&);
public:
    static void		initClass();
};

} // namespace Vel
