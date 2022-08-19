#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uiobjfileman.h"

class uiWaveletExtraction;
class uiWaveletDispPropDlg;
class uiSeisSingleTraceDisplay;
class uiToolButton;
class uiFuncDispBase;
class uiLabel;
class Wavelet;


mExpClass(uiSeis) uiSeisWvltMan : public uiObjFileMan
{ mODTextTranslationClass(uiSeisWvltMan);
public:
			uiSeisWvltMan(uiParent*);
			~uiSeisWvltMan();

    mDeclInstanceCreatedNotifierAccess(uiSeisWvltMan);

protected:

    uiWaveletExtraction*	wvltext_;
    uiWaveletDispPropDlg*	wvltpropdlg_;
    uiToolButton*		copybut_;
    uiToolButton*		disppropbut_;
    uiToolButton*		revpolbut_;
    uiToolButton*		rotatephbut_;
    uiToolButton*		taperbut_;
    uiFuncDispBase*		waveletdisplay_;
    uiLabel*			wvnamdisp_;
    void			addButtons();
    void			mkFileInfo() override;
    void			ownSelChg() override;
    void			dispWavelet(const Wavelet*);

    void			closeDlg(CallBacker*);
    void			crPush(CallBacker*);
    void			dispProperties(CallBacker*);

    void			impPush(CallBacker*);
    void			mrgPush(CallBacker*);
    void			extractPush(CallBacker*);
    void			matchPush(CallBacker*);
    void			copyPush(CallBacker*);

    void			getFromOtherSurvey(CallBacker*);
    void			reversePolarity(CallBacker*);
    void			rotatePhase(CallBacker*);
    void			taper(CallBacker*);
    void			updateCB(CallBacker*);
				/* Do not use this function.
				   It is replaced by
				   'wvltCreatedCB(CallBacker*)'
				   function. Because same function is
				   defined in base class 'uiObjFileMan' */
    void			rotUpdateCB(CallBacker*);
    void			wvltCreatedCB(CallBacker*);
};
