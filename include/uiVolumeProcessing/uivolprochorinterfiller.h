#ifndef uivolprochorinterfiller_h
#define uivolprochorinterfiller_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y.C. Liu
 Date:		April 2007
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uivolprocstepdlg.h"
#include "volprochorinterfiller.h"

class uiIOObjSel;
class CtxtIOObj;


namespace VolProc
{

mClass uiHorInterFiller : public uiStepDialog
{
public:
    mDefaultFactoryInstanciationBase(
	    VolProc::HorInterFiller::sFactoryKeyword(),
	    VolProc::HorInterFiller::sFactoryDisplayName())
	    mDefaultFactoryInitClassImpl( uiStepDialog, createInstance );
protected:


				uiHorInterFiller(uiParent*,HorInterFiller*);
   				~uiHorInterFiller();
    static uiStepDialog*	createInstance(uiParent*,Step*);

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
