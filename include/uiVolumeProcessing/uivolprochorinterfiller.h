#ifndef uivolprochorinterfiller_h
#define uivolprochorinterfiller_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y.C. Liu
 Date:		April 2007
 RCS:		$Id: uivolprochorinterfiller.h,v 1.6 2009-07-22 16:01:24 cvsbert Exp $
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
