#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellmod.h"

#include "uitime2depthzaxistrans.h"
#include "wellt2dtransform.h"

class uiIOObjSel;

/*!User interface that creates a ZAxisTransform from a Well's t2d model. */

mExpClass(uiWell) uiWellT2DTransform : public uiTime2DepthZTransformBase
{ mODTextTranslationClass(uiWellT2DTransform);
private:
			uiWellT2DTransform(uiParent*);
			~uiWellT2DTransform();

    ZAxisTransform*	getSelection() override;
    bool		canBeField() const override	{ return true; }

    void		doInitGrp() override;
    void		setZRangeCB(CallBacker*);
    bool		acceptOK() override;
    bool		usePar(const IOPar&) override;
    const char*		transformName() const override;

    uiIOObjSel*			selfld_;
    RefMan<ZAxisTransform>	transform_;

    static uiZAxisTransform*	createInstance(uiParent*,
						const uiZAxisTranformSetup&);
public:
    static void initClass();
};
