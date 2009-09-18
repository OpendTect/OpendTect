#ifndef uivolprocbodyfiller_h
#define uivolprocbodyfiller_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y.C. Liu
 Date:		November 2007
 RCS:		$Id: uivolprocbodyfiller.h,v 1.1 2009-09-18 18:13:43 cvskris Exp $
________________________________________________________________________

-*/

#include "uivolprocstepdlg.h"
class CtxtIOObj;
class uiIOObjSel;


namespace VolProc
{

class BodyFiller;

mClass uiBodyFiller: public uiStepDialog
{
public:

   static void			initClass();

				uiBodyFiller(uiParent*, BodyFiller*);
   				~uiBodyFiller();

protected:

    static uiStepDialog*	create(uiParent*, Step*);
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
