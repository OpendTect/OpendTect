#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"

#include "simpletimedepthmodel.h"
#include "uitime2depthzaxistrans.h"

class uiIOObjSel;

/*!User interface that creates a ZAxisTransform from a table t2d model. */

mExpClass(uiIo) uiSimpleTimeDepthTransform : public uiTime2DepthZTransformBase
{ mODTextTranslationClass(uiSimpleTimeDepthTransform);
private:
			uiSimpleTimeDepthTransform(uiParent*,bool t2d);
			~uiSimpleTimeDepthTransform();

    ZAxisTransform*	getSelection() override;
    bool		canBeField() const override	{ return true; }

    void		doInitGrp() override;
    void		setZRangeCB(CallBacker*);
    void		createCB(CallBacker*);
    void		editCB(CallBacker*);
    bool		acceptOK() override;

    uiIOObjSel*		selfld_;
    RefMan<SimpleTimeDepthTransform> transform_;

    static uiZAxisTransform*	createInstance(uiParent*,
						const uiZAxisTranformSetup&);
public:
    static void		initClass();

};
