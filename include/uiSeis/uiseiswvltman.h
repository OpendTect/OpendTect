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


mClass(uiSeis) uiSeisWvltMan : public uiObjFileMan
{
public:
			uiSeisWvltMan(uiParent*);
			~uiSeisWvltMan();

    mDeclInstanceCreatedNotifierAccess(uiSeisWvltMan);

protected:

    uiGroup*			butgrp_;
    uiSeisSingleTraceDisplay*	trcdisp_;
    uiWaveletExtraction*  	wvltext_;
    uiWaveletDispPropDlg*	wvltpropdlg_;

    void			mkFileInfo();
    
    void                	closeDlg(CallBacker*);
    void			crPush(CallBacker*);
    void			dispProperties(CallBacker*);
    void			impPush(CallBacker*);
    void			mrgPush(CallBacker*);
    void			extractPush(CallBacker*);
    void			getFromOtherSurvey(CallBacker*);
    void			reversePolarity(CallBacker*);
    void			rotatePhase(CallBacker*);
    void			taper(CallBacker*);
    void                	updateCB(CallBacker*);
    void 			rotUpdateCB(CallBacker*);
};


#endif

