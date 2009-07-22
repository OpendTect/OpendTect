#ifndef uivelocitygridder_h
#define uivelocitygridder_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: uivelocitygridder.h,v 1.5 2009-07-22 16:01:24 cvsbert Exp $
________________________________________________________________________


-*/

#include "uivolprocstepdlg.h"

class uiGridder2DSel;
namespace VolProc { class Step; }
namespace Vel { class uiFunctionSel; }


namespace VolProc
{

class VelGriddingStep;

mClass uiVelocityGridder : public uiStepDialog
{
public:

    static void		initClass();
			uiVelocityGridder(uiParent*,VelGriddingStep*);

protected:

    bool			acceptOK(CallBacker*);
    static uiStepDialog*	create(uiParent*,VolProc::Step*);

    void			pickSelChange(CallBacker*);

    uiGridder2DSel*		griddersel_;
    Vel::uiFunctionSel*		velfuncsel_;
    VelGriddingStep*		operation_;

};


}; //namespace

#endif
