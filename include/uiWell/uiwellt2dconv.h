#ifndef uiwellt2dconv_h
#define uiwellt2dconv_h

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

    ZAxisTransform*	getSelection();
    bool		canBeField() const	{ return true; }

    bool		acceptOK();

    static void 	initClass();

protected:

    static uiZAxisTransform*	createInstance(uiParent*,const char*,
					       const char*);

    uiIOObjSel* 		fld_;
    WellT2DTransform*		transform_;
};

#endif

