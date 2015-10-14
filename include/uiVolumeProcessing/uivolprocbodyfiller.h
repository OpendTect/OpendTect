#ifndef uivolprocbodyfiller_h
#define uivolprocbodyfiller_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y.C. Liu
 Date:		November 2007
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uivolumeprocessingmod.h"
#include "uivolprocstepdlg.h"
#include "volprocbodyfiller.h"

class uiIOObjSel;


namespace VolProc
{

mExpClass(uiVolumeProcessing) uiBodyFiller: public uiStepDialog
{ mODTextTranslationClass(uiBodyFiller);
public:
	mDefaultFactoryInstanciationBase(
		VolProc::BodyFiller::sFactoryKeyword(),
		VolProc::BodyFiller::sFactoryDisplayName());

protected:
				uiBodyFiller(uiParent*,BodyFiller*);
    static uiStepDialog*	createInstance(uiParent*,Step*);

    bool			acceptOK(CallBacker*);
    void			bodySel(CallBacker*);
    void			typeSel(CallBacker*);

    BodyFiller*			bodyfiller_;

    uiIOObjSel*			bodyfld_;
    uiGenInput*			insidetypfld_;
    uiGenInput*			insidevaluefld_;
    uiGenInput*			outsidetypfld_;
    uiGenInput*			outsidevaluefld_;
};

} // namespace VolProc

#endif


