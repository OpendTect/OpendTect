#ifndef uivolprocsmoother_h
#define uivolprocsmoother_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		February 2008
 RCS:		$Id: uivolprocsmoother.h,v 1.1 2008-02-26 23:02:58 cvskris Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiWindowFunctionSel;
class uiLabeledSpinBox;

namespace VolProc
{

class Smoother;
class Step;

class uiSmoother : public uiDialog
{
public:
   static void			initClass();
   				~uiSmoother();
				
				uiSmoother(uiParent*,Smoother*);

protected:
    static uiDialog*		create(uiParent*, Step*);
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
