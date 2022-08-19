#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uiattrdesced.h"
#include "uiwindowfunctionsel.h"

namespace Attrib { class Desc; };

class uiFreqFilterSelFreq;
class uiImagAttrSel;
class uiGenInput;
class uiCheckBox;

/*! \brief ** Attribute description editor */

mExpClass(uiAttributes) uiFreqFilterAttrib : public uiAttrDescEd
{ mODTextTranslationClass(uiFreqFilterAttrib);
public:

			uiFreqFilterAttrib(uiParent*,bool);

    void		getEvalParams(TypeSet<EvalParam>&) const override;

protected:

    uiImagAttrSel*      inpfld_;
    uiGenInput*         isfftfld_;
    uiFreqFilterSelFreq* freqfld_;
    uiGenInput*		polesfld_;
    uiCheckBox*		freqwinselfld_;
    ObjectSet<uiWindowFunctionSel> winflds_;
    uiWindowFunctionSel::Setup* viewsetup_;

    void		finalizeCB(CallBacker*);
    void		selectionDoneCB(CallBacker*);
    void		freqChanged(CallBacker*);
    void		freqWinSel(CallBacker*);
    void		updateTaperFreqs(CallBacker*);
    void		typeSel(CallBacker*);
    void		isfftSel(CallBacker*);

    bool		setParameters(const Attrib::Desc&) override;
    bool		setInput(const Attrib::Desc&) override;

    bool		getParameters(Attrib::Desc&) override;
    bool		getInput(Attrib::Desc&) override;

    bool		areUIParsOK() override;

    			mDeclReqAttribUIFns
};
