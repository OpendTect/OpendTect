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

    uiWaveletExtraction*	wvltext_		= nullptr;
    uiWaveletDispPropDlg*	wvltpropdlg_		= nullptr;
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

    void			closeDlgCB(CallBacker*);
    void			crPushCB(CallBacker*);
    void			dispPropertiesCB(CallBacker*);

    void			impPushCB(CallBacker*);
    void			mrgPushCB(CallBacker*);
    void			extractPushCB(CallBacker*);
    void			matchPushCB(CallBacker*);
    void			copyPushCB(CallBacker*);

    void			getFromOtherSurveyCB(CallBacker*);
    void			reversePolarityCB(CallBacker*);
    void			rotatePhaseCB(CallBacker*);
    void			taperCB(CallBacker*);
    void			updateCB(CallBacker*);
				/* Do not use this function.
				   It is replaced by
				   'wvltCreatedCB(CallBacker*)'
				   function. Because same function is
				   defined in base class 'uiObjFileMan' */
    void			rotUpdateCB(CallBacker*);
    void			wvltCreatedCB(CallBacker*);
    bool			waveletSaveAs(const Wavelet&, const uiString&);
};
