#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2010
 RCS:		$Id: uiwellt2dconv.h 32104 2013-10-23 20:11:53Z kristofer.tingdahl@dgbes.com $
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
    void			setZRangeCB(CallBacker*);

    uiIOObjSel* 		fld_;
    WellT2DTransform*		transform_;
};

