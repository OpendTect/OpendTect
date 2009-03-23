#ifndef uivolprochorinterfiller_h
#define uivolprochorinterfiller_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Y.C. Liu
 Date:		April 2007
 RCS:		$Id: uivolprochorinterfiller.h,v 1.5 2009-03-23 11:02:00 cvsbert Exp $
________________________________________________________________________

-*/

#include "uivolprocstepdlg.h"
class uiIOObjSel;
class CtxtIOObj;


namespace VolProc
{
class HorInterFiller;


mClass uiHorInterFiller : public uiStepDialog
{
public:

   static void			initClass();

				uiHorInterFiller(uiParent*,HorInterFiller*);
   				~uiHorInterFiller();

protected:

    static uiStepDialog*	create(uiParent*,Step*);
    bool			acceptOK(CallBacker*);
    void			updateFlds(CallBacker*);

    HorInterFiller*		horinterfiller_;
    CtxtIOObj*			topctio_;
    CtxtIOObj*			bottomctio_;

    uiGenInput*			usetophorfld_;
    uiIOObjSel*			tophorfld_;
    uiGenInput*			topvalfld_;

    uiGenInput*			usebottomhorfld_;
    uiIOObjSel*			bottomhorfld_;

    uiGenInput*			usegradientfld_;

    uiGenInput*			gradientfld_;
    uiGenInput*			bottomvalfld_;

};

}; //namespace

#endif
