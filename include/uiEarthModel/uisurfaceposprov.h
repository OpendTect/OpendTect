#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uiposprovgroup.h"

class uiGenInput;
class uiIOObjSel;
class uiLabel;
class uiLabeledSpinBox;
class uiSelZRange;
class uiSpinBox;

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

    const float		zfac_;

    uiIOObjSel*		surf1fld_		= nullptr;
    uiIOObjSel*		surf2fld_		= nullptr;
    uiGenInput*		issingfld_		= nullptr;
    uiLabeledSpinBox*	zstepfld_		= nullptr;
    uiSelZRange*	extrazfld_		= nullptr;
    uiGenInput*		samplingfld_		= nullptr;
    uiGenInput*		nrsamplesfld_		= nullptr;

    void		selChg(CallBacker*);
    void		samplingCB(CallBacker*);
};
