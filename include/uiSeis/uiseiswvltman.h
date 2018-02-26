#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2006
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uiobjfileman.h"

class uiWaveletExtraction;
class uiWaveletDispPropDlg;
class uiSeisSingleTraceDisplay;
class uiToolButton;
class uiFunctionDisplay;
class uiLabel;
class Wavelet;


mExpClass(uiSeis) uiSeisWvltMan : public uiObjFileMan
{ mODTextTranslationClass(uiSeisWvltMan);
public:
			uiSeisWvltMan(uiParent*);
			~uiSeisWvltMan();

    mDeclInstanceCreatedNotifierAccess(uiSeisWvltMan);

protected:

    uiWaveletExtraction*	extrdlg_;
    uiWaveletDispPropDlg*	propdlg_;
    uiToolButton*		copybut_;
    uiToolButton*		disppropbut_;
    uiToolButton*		revpolbut_;
    uiToolButton*		rotatephbut_;
    uiToolButton*		taperbut_;
    uiFunctionDisplay*		waveletdisplay_;
    uiLabel*			wvnamdisp_;
    void			addButtons();
    void			dispWavelet(const Wavelet*);
    virtual void		ownSelChg();
    virtual bool		gtItemInfo(const IOObj&,uiPhraseSet&) const;

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
    void			wvltExtractedCB(CallBacker*);
    void			extrDlgCloseCB(CallBacker*);
    void			propDlgCloseCB(CallBacker*);
    void			rotUpdateCB(CallBacker*);
};
