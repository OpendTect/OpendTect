#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          March 2013
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

    ZAxisTransform*	getSelection();

    FixedString	toDomain() const;
    FixedString	fromDomain() const;
    bool		canBeField() const			{ return true; }

protected:
    static uiZAxisTransform*	createInstance(uiParent*,const char*,
					       const char*);
    bool			acceptOK();
    void			velChangedCB(CallBacker*);

    uiGenInput*			velfld_;
    uiGenInput*			gradientfld_;
};

}; //namespace


