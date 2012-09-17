#ifndef uivolprocsmoother_h
#define uivolprocsmoother_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		February 2008
 RCS:		$Id: uivolprocsmoother.h,v 1.6 2011/08/24 13:19:43 cvskris Exp $
________________________________________________________________________

-*/

#include "uivolprocchain.h"
#include "volprocsmoother.h"

class uiWindowFunctionSel;
class uiLabeledSpinBox;

namespace VolProc
{


mClass uiSmoother : public uiStepDialog
{
public:

    mDefaultFactoryInstanciationBase(
	    VolProc::Smoother::sFactoryKeyword(),
	    VolProc::Smoother::sFactoryDisplayName())
    mDefaultFactoryInitClassImpl( uiStepDialog, createInstance );

				uiSmoother(uiParent*,Smoother*);

protected:

    static uiStepDialog*	createInstance(uiParent*, Step*);
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
