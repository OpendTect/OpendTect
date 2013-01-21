#ifndef uivolproclateralsmoother_h
#define uivolproclateralsmoother_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		February 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uivolumeprocessingmod.h"
#include "uivolprocchain.h"
#include "volproclateralsmoother.h"

class uiGenInput;
class uiLabeledSpinBox;

namespace VolProc
{

class LateralSmoother;


mExpClass(uiVolumeProcessing) uiLateralSmoother : public uiStepDialog
{
public:
    mDefaultFactoryInstanciationBase(
	VolProc::LateralSmoother::sFactoryKeyword(),
	VolProc::LateralSmoother::sFactoryDisplayName())
	mDefaultFactoryInitClassImpl( uiStepDialog, createInstance );


protected:

				uiLateralSmoother(uiParent*,LateralSmoother*);
    static uiStepDialog*	createInstance(uiParent*, Step*);

    bool			acceptOK(CallBacker*);
    void			updateFlds(CallBacker*);

    LateralSmoother*		smoother_;

    uiLabeledSpinBox*		inllenfld_;
    uiLabeledSpinBox*		crllenfld_;
    uiGenInput*			replaceudfsfld_;

    uiGenInput*			ismedianfld_;
    uiGenInput*			weightedfld_;
    uiGenInput*			mirroredgesfld_;

    uiGenInput*			udfhandling_;
    uiGenInput*			udffixedvalue_;
    

};

}; //namespace

#endif

