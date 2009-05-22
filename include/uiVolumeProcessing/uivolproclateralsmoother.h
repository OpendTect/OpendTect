#ifndef uivolproclateralsmoother_h
#define uivolproclateralsmoother_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		February 2008
 RCS:		$Id: uivolproclateralsmoother.h,v 1.1 2009-05-22 18:41:55 cvskris Exp $
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
