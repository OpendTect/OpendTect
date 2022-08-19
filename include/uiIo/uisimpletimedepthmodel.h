#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uitime2depthzaxistrans.h"

class SimpleTimeDepthTransform;
class uiIOObjSel;

/*!User interface that creates a ZAxisTransform from a Well's t2d model. */

mExpClass(uiIo) uiSimpleTimeDepthTransform : public uiTime2DepthZTransformBase
{ mODTextTranslationClass(uiSimpleTimeDepthTransform);
public:
			uiSimpleTimeDepthTransform(uiParent*,bool t2d);
			~uiSimpleTimeDepthTransform();

    ZAxisTransform*	getSelection() override;
    bool		canBeField() const override	{ return true; }

    bool		acceptOK() override;

    static void 	initClass();

protected:

    static uiZAxisTransform*	createInstance(uiParent*,const char*,
					       const char*);
    void			createCB(CallBacker*);
    void			setZRangeCB(CallBacker*);
    void			editCB(CallBacker*);

    uiIOObjSel* 		selfld_;
    SimpleTimeDepthTransform*	transform_;
};
