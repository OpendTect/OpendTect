#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          July 2001
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

    void		getEvalParams(TypeSet<EvalParam>&) const;

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

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);

    virtual bool        areUIParsOK();

    			mDeclReqAttribUIFns
};

