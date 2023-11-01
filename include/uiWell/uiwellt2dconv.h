#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellmod.h"

#include "uitime2depthzaxistrans.h"

class WellT2DTransform;
class uiIOObjSel;

/*!User interface that creates a ZAxisTransform from a Well's t2d model. */

mExpClass(uiWell) uiWellT2DTransform : public uiTime2DepthZTransformBase
{ mODTextTranslationClass(uiWellT2DTransform);
public:
			uiWellT2DTransform(uiParent*);
			~uiWellT2DTransform();

    ZAxisTransform*	getSelection() override;
    bool		canBeField() const override	{ return true; }

    bool		acceptOK() override;

    static void 	initClass();

protected:

    static uiZAxisTransform*	createInstance(uiParent*,const char*,
					       const char*);
    void			initGrpCB(CallBacker*);
    void			setZRangeCB(CallBacker*);

    uiIOObjSel* 		fld_;
    WellT2DTransform*		transform_;
};
