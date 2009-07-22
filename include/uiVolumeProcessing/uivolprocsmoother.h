#ifndef uivolprocsmoother_h
#define uivolprocsmoother_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		February 2008
 RCS:		$Id: uivolprocsmoother.h,v 1.5 2009-07-22 16:01:24 cvsbert Exp $
________________________________________________________________________

-*/

#include "uivolprocchain.h"

class uiWindowFunctionSel;
class uiLabeledSpinBox;

namespace VolProc
{

class Smoother;


mClass uiSmoother : public uiStepDialog
{
public:

   static void			initClass();
				
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
