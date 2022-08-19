#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uiattrdesced.h"
#include "uisteeringsel.h"

class uiAttrSel;
class uiGenInput;
class uiStepOutSel;


/*! \brief Similarity Attribute description editor */

mExpClass(uiAttributes) uiSimilarityAttrib : public uiAttrDescEd
{ mODTextTranslationClass(uiSimilarityAttrib);
public:

			uiSimilarityAttrib(uiParent*,bool);

    void		getEvalParams(TypeSet<EvalParam>&) const override;

protected:

    uiAttrSel*		inpfld_;
    uiGenInput*		gatefld_;
    uiGenInput*		extfld_;
    uiStepOutSel*	pos0fld_;
    uiStepOutSel*	pos1fld_;
    uiStepOutSel*	stepoutfld_;
    uiGenInput*		outpstatsfld_;
    uiGenInput*		maxdipfld_;
    uiGenInput*		deltadipfld_;
    uiGenInput*		outpdipfld_;
    uiGenInput*		dooutpstatsfld_;

    bool		setParameters(const Attrib::Desc&) override;
    bool		setInput(const Attrib::Desc&) override;
    bool		setOutput(const Attrib::Desc&) override;

    bool		getParameters(Attrib::Desc&) override;
    bool		getInput(Attrib::Desc&) override;
    bool		getOutput(Attrib::Desc&) override;

    void		extSel(CallBacker*);
    void		outSel(CallBacker*);
    void		steerTypeSel(CallBacker*);

			mDeclReqAttribUIFns

    mExpClass(uiAttributes) uiSimiSteeringSel : public uiSteeringSel
    { mODTextTranslationClass(uiSimiSteeringSel) 
	public:
			uiSimiSteeringSel(uiParent*,const Attrib::DescSet*,
					  bool is2d);
	   
	    bool	willSteer() const override;
	    bool	wantBrowseDip() const;
	    int		browseDipIdxInList() const;

	    Notifier<uiSimiSteeringSel> typeSelected;

	protected:
	    void	typeSel(CallBacker*) override;
    };

    uiSimiSteeringSel*	steerfld_;
};
