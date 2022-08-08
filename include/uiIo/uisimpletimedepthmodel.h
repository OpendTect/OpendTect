#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman Singh
 Date:          Jan 2021
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

