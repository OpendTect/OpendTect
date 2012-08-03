#ifndef uivelocitygridder_h
#define uivelocitygridder_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: uivelocitygridder.h,v 1.8 2012-08-03 13:01:19 cvskris Exp $
________________________________________________________________________


-*/

#include "uivolumeprocessingmod.h"
#include "uivolprocstepdlg.h"
#include "velocitygridder.h"

class uiGridder2DSel;
namespace VolProc { class Step; }
namespace Vel { class uiFunctionSel; }


namespace VolProc
{

class VelGriddingStep;

mClass(uiVolumeProcessing) uiVelocityGridder : public uiStepDialog
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

