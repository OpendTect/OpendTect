#ifndef uivolprocsmoother_h
#define uivolprocsmoother_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		February 2008
 RCS:		$Id: uivolprocsmoother.h,v 1.3 2009-01-08 09:00:11 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uivolprocchain.h"

class uiWindowFunctionSel;
class uiLabeledSpinBox;

namespace VolProc
{

class Smoother;
class Step;

mClass uiSmoother : public uiStepDialog
{
public:
   static void			initClass();
   				~uiSmoother();
				
				uiSmoother(uiParent*,Smoother*);

protected:
    static uiStepDialog*	create(uiParent*, Step*);
    bool			acceptOK(CallBacker*);
    void			updateFlds(CallBacker*);

    Smoother*			smoother_;

    uiWindowFunctionSel*	operatorselfld_;
    uiLabeledSpinBox*		inllenfld_;
    uiLabeledSpinBox*		crllenfld_;
    uiLabeledSpinBox*		zlenfld_;
};

}; //namespace

#endif
