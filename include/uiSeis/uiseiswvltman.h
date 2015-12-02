#ifndef uiseiswvltman_h
#define uiseiswvltman_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2006
 RCS:           $Id$
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

    uiWaveletExtraction*	wvltext_;
    uiWaveletDispPropDlg*	wvltpropdlg_;
    uiToolButton*		disppropbut_;
    uiToolButton*		revpolbut_;
    uiToolButton*		rotatephbut_;
    uiToolButton*		taperbut_;
    uiFunctionDisplay*		waveletdisplay_;
    uiLabel*			wvnamdisp_;
    void			addButtons();
    void			mkFileInfo();
    void			ownSelChg();
    void			dispWavelet(const Wavelet*);

    void			closeDlg(CallBacker*);
    void			crPush(CallBacker*);
    void			dispProperties(CallBacker*);

    void			impPush(CallBacker*);
    void			mrgPush(CallBacker*);
    void			extractPush(CallBacker*);
    void			matchPush(CallBacker*);

    void			getFromOtherSurvey(CallBacker*);
    void			reversePolarity(CallBacker*);
    void			rotatePhase(CallBacker*);
    void			taper(CallBacker*);
    void			updateCB(CallBacker*);
    void			rotUpdateCB(CallBacker*);
};

#endif
