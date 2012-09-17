#ifndef uivelocitygridder_h
#define uivelocitygridder_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: uivelocitygridder.h,v 1.7 2011/08/24 13:19:43 cvskris Exp $
________________________________________________________________________


-*/

#include "uivolprocstepdlg.h"
#include "velocitygridder.h"

class uiGridder2DSel;
namespace VolProc { class Step; }
namespace Vel { class uiFunctionSel; }


namespace VolProc
{

class VelGriddingStep;

mClass uiVelocityGridder : public uiStepDialog
{
public:

		mDefaultFactoryInstanciationBase(
		    VolProc::VelGriddingStep::sFactoryKeyword(),
		    VolProc::VelGriddingStep::sFactoryDisplayName())
		    mDefaultFactoryInitClassImpl( uiStepDialog, createInstance );

protected:

				uiVelocityGridder(uiParent*,VelGriddingStep*);
    bool			acceptOK(CallBacker*);
    static uiStepDialog*	createInstance(uiParent*,VolProc::Step*);

    void			pickSelChange(CallBacker*);
    void			nameChangeCB(CallBacker*);
    void			sourceChangeCB(CallBacker*);

    uiGridder2DSel*		griddersel_;
    Vel::uiFunctionSel*		velfuncsel_;
    VelGriddingStep*		operation_;

    bool			namenotset_;
};


}; //namespace

#endif
