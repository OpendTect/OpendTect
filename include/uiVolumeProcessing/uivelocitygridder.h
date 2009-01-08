#ifndef uivelocitygridder_h
#define uivelocitygridder_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: uivelocitygridder.h,v 1.3 2009-01-08 09:00:11 cvsranojay Exp $
________________________________________________________________________


-*/

#include "iopar.h"
#include "uivolprocchain.h"

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
			~uiVelocityGridder();

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
