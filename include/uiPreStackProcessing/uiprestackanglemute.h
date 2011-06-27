#ifndef uiprestackanglemute_h
#define uiprestackanglemute_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y. Liu
 Date:		January 2011
 RCS:		$Id: uiprestackanglemute.h,v 1.3 2011-06-27 09:56:40 cvsbruno Exp $
________________________________________________________________________


-*/

#include "uidialog.h"

class CtxtIOObj;
class uiCheckBox;
class uiGenInput;
class uiRayTracer1D;
class uiVelSel;

namespace PreStack
{

class AngleMute;
class Processor;

mClass uiAngleMute : public uiDialog
{
public:

    static void		initClass();
			uiAngleMute(uiParent*,AngleMute*);

protected:

    bool		acceptOK(CallBacker*);
    static uiDialog*	create(uiParent*,Processor*);

    void		createPushedCB(CallBacker*);
    void		editPushedCB(CallBacker*);
    void		selDoneCB(CallBacker*);

    AngleMute*		processor_;
    
    uiVelSel*		velfuncsel_;
    uiRayTracer1D*	raytracerfld_;
    uiGenInput*		cutofffld_;
    uiGenInput*		topfld_;
    uiGenInput*		taperlenfld_;
    uiCheckBox*		blockfld_;
};


}; //namespace

#endif
