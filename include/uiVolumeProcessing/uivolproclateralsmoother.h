#ifndef uivolproclateralsmoother_h
#define uivolproclateralsmoother_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		February 2008
 RCS:		$Id: uivolproclateralsmoother.h,v 1.2 2009-07-22 16:01:24 cvsbert Exp $
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

    uiGenInput*			ismedianfld_;
    uiGenInput*			weightedfld_;
    uiLabeledSpinBox*		inllenfld_;
    uiLabeledSpinBox*		crllenfld_;
};

}; //namespace

#endif
