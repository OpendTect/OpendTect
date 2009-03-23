#ifndef uivelocitygridder_h
#define uivelocitygridder_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: uivelocitygridder.h,v 1.4 2009-03-23 11:02:00 cvsbert Exp $
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
