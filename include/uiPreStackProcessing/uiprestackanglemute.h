#ifndef uiprestackanglemute_h
#define uiprestackanglemute_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y. Liu
 Date:		January 2011
 RCS:		$Id: uiprestackanglemute.h,v 1.4 2011-07-12 10:51:55 cvsbruno Exp $
________________________________________________________________________


-*/

#include "uidialog.h"
#include "uigroup.h"
#include "prestackanglemute.h"

class CtxtIOObj;
class uiCheckBox;
class uiGenInput;
class uiRayTracer1D;
class uiVelSel;

namespace PreStack
{

class Processor;

mClass uiAngleMuteGrp : public uiGroup
{
public:
			uiAngleMuteGrp(uiParent*,AngleMuteBase::Params&,
					bool dooffset = false);

    bool		acceptOK();

    const uiRayTracer1D* rayTracer() const	 { return raytracerfld_; }

protected:
    AngleMuteBase::Params& params_;

    uiRayTracer1D*	raytracerfld_;
    uiVelSel*		velfuncsel_;
    uiGenInput*		cutofffld_;
    uiCheckBox*		blockfld_;
};


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
    
    uiAngleMuteGrp*	anglemutegrp_;
    uiGenInput*		taperlenfld_;
    uiGenInput*		topfld_;
};


}; //namespace

#endif
