#ifndef uivolprocsmoother_h
#define uivolprocsmoother_h

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
#include "volprocsmoother.h"

class uiWindowFunctionSel;
class uiLabeledSpinBox;

namespace VolProc
{


mClass(uiVolumeProcessing) uiSmoother : public uiStepDialog
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

