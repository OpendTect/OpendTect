#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uiposprovgroup.h"
class CtxtIOObj;
class uiGenInput;
class uiIOObjSel;
class uiSpinBox;
class uiLabeledSpinBox;
class uiSelZRange;
class uiLabel;

/*! \brief UI for SurfacePosProvider */

mExpClass(uiEarthModel) uiSurfacePosProvGroup : public uiPosProvGroup
{ mODTextTranslationClass(uiSurfacePosProvGroup);
public:
			uiSurfacePosProvGroup(uiParent*,
					   const uiPosProvGroup::Setup&);
			~uiSurfacePosProvGroup();

    void		usePar(const IOPar&) override;
    bool		fillPar(IOPar&) const override;
    void		getSummary(BufferString&) const override;
    bool		hasRandomSampling() const override;

    static uiPosProvGroup* create( uiParent* p, const uiPosProvGroup::Setup& s)
    			{ return new uiSurfacePosProvGroup(p,s); }
    static void		initClass();

protected:

    CtxtIOObj&		ctio1_;
    CtxtIOObj&		ctio2_;
    const float		zfac_;

    uiIOObjSel*		surf1fld_;
    uiIOObjSel*		surf2fld_;
    uiGenInput*		issingfld_;
    uiLabeledSpinBox*	zstepfld_;
    uiSelZRange*	extrazfld_;
    uiGenInput*		samplingfld_;
    uiGenInput*		nrsamplesfld_;

    void		selChg(CallBacker*);
    void		samplingCB(CallBacker*);
};


