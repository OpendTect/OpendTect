#ifndef uivolprochorinterfiller_h
#define uivolprochorinterfiller_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Y.C. Liu
 Date:		April 2007
 RCS:		$Id: uivolprochorinterfiller.h,v 1.3 2008-08-14 21:52:44 cvskris Exp $
________________________________________________________________________

-*/

#include "uivolprocchain.h"

class uiGenInput;
class uiIOObjSel;
class CtxtIOObj;

namespace VolProc
{

class Step;
class HorInterFiller;

class uiHorInterFiller : public uiStepDialog
{
public:
   static void			initClass();
   				~uiHorInterFiller();
				
				uiHorInterFiller(uiParent*,HorInterFiller*);

protected:
    static uiStepDialog*	create(uiParent*, Step*);
    bool			acceptOK(CallBacker*);
    void			updateFlds(CallBacker*);

    HorInterFiller*		horinterfiller_;

    uiGenInput*			usetophorfld_;
    CtxtIOObj*			topctxt_;
    uiIOObjSel*			tophorfld_;
    uiGenInput*			topvalfld_;

    uiGenInput*			usebottomhorfld_;
    uiIOObjSel*			bottomhorfld_;
    CtxtIOObj*			bottomctxt_;

    uiGenInput*			usegradientfld_;

    uiGenInput*			gradientfld_;
    uiGenInput*			bottomvalfld_;
};

}; //namespace

#endif
