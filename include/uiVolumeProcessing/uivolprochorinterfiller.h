#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y.C. Liu
 Date:		April 2007
________________________________________________________________________

-*/

#include "uivolumeprocessingmod.h"
#include "uivolprocstepdlg.h"
#include "volprochorinterfiller.h"

class uiIOObjSel;
class CtxtIOObj;


namespace VolProc
{

mExpClass(uiVolumeProcessing) uiHorInterFiller : public uiStepDialog
{ mODTextTranslationClass(uiHorInterFiller)
public:
    mDefaultFactoryInstanciationBase(
	    VolProc::HorInterFiller::sFactoryKeyword(),
	    VolProc::HorInterFiller::sFactoryDisplayName())
	    mDefaultFactoryInitClassImpl( uiStepDialog, createInstance )
protected:


				uiHorInterFiller(uiParent*,HorInterFiller*,
						 bool is2d);
				~uiHorInterFiller();

    static uiStepDialog*	createInstance(uiParent*,Step*,bool is2d);

    bool			acceptOK(CallBacker*) override;
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

} // namespace VolProc

