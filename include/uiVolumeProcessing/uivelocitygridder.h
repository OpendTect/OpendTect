#ifndef uivelocitygridder_h
#define uivelocitygridder_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: uivelocitygridder.h,v 1.1 2008-07-22 19:44:22 cvskris Exp $
________________________________________________________________________


-*/

#include "iopar.h"
#include "uidialog.h"

class uiGridder2DSel;
namespace VolProc { class Step; }
namespace Vel { class uiFunctionSel; }


namespace VolProc
{

class VelGriddingStep;

class uiVelocityGridder : public uiDialog
{
public:
    static void		initClass();
			uiVelocityGridder(uiParent*,VelGriddingStep*);
			~uiVelocityGridder();

protected:
    bool			acceptOK(CallBacker*);
    static uiDialog*		create(uiParent*,VolProc::Step*);

    void			pickSelChange(CallBacker*);
    uiGridder2DSel*		griddersel_;
    Vel::uiFunctionSel*		velfuncsel_;
    VelGriddingStep*		operation_;
};


}; //namespace

#endif
