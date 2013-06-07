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

#include "uivolprocstepdlg.h"
#include "volprocbodyfiller.h"

class CtxtIOObj;
class uiIOObjSel;


namespace VolProc
{

mClass uiBodyFiller: public uiStepDialog
{
public:
	mDefaultFactoryInstanciationBase(
		VolProc::BodyFiller::sFactoryKeyword(),
		VolProc::BodyFiller::sFactoryDisplayName());

protected:
				uiBodyFiller(uiParent*, BodyFiller*);
   				~uiBodyFiller();
    static uiStepDialog*	createInstance(uiParent*, Step*);

    bool			acceptOK(CallBacker*);
    void			updateFlds(CallBacker*);

    BodyFiller*			bodyfiller_;
    CtxtIOObj*			ctio_;

    uiIOObjSel*			uinputselfld_;			
    uiGenInput*			useinsidefld_;
    uiGenInput*			useoutsidefld_;
    uiGenInput*			insidevaluefld_;
    uiGenInput*			outsidevaluefld_;
};

}; //namespace

#endif
