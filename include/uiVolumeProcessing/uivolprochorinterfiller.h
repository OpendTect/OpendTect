#ifndef uivolprochorinterfiller_h
#define uivolprochorinterfiller_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Y.C. Liu
 Date:		April 2007
 RCS:		$Id: uivolprochorinterfiller.h,v 1.1 2008-02-25 19:14:54 cvskris Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiGenInput;
class uiIOObjSel;
class CtxtIOObj;

namespace VolProc
{

class Step;
class HorInterFiller;

class uiHorInterFiller : public uiDialog
{
public:
   static void			initClass();
   				~uiHorInterFiller();
				
				uiHorInterFiller(uiParent*,HorInterFiller*);

protected:
    static uiDialog*		create(uiParent*, Step*);
    bool			acceptOK(CallBacker*);
    void			updateFlds(CallBacker*);

    HorInterFiller*		horinterfiller_;

    uiGenInput*			usetopvalfld_;
    CtxtIOObj*			topctxt_;
    uiIOObjSel*			tophorfld_;
    uiGenInput*			topvalfld_;

    uiGenInput*			usebottomvalfld_;
    uiIOObjSel*			bottomhorfld_;
    CtxtIOObj*			bottomctxt_;
    uiGenInput*			bottomvalfld_;
};

}; //namespace

#endif
