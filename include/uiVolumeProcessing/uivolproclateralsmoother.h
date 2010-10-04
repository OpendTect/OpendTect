#ifndef uivolproclateralsmoother_h
#define uivolproclateralsmoother_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		February 2008
 RCS:		$Id: uivolproclateralsmoother.h,v 1.3 2010-10-04 19:56:14 cvskris Exp $
________________________________________________________________________

-*/

#include "uivolprocchain.h"

class uiGenInput;
class uiLabeledSpinBox;

namespace VolProc
{

class LateralSmoother;


mClass uiLateralSmoother : public uiStepDialog
{
public:

   static void			initClass();
				
				uiLateralSmoother(uiParent*,LateralSmoother*);

protected:

    static uiStepDialog*	create(uiParent*, Step*);
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
