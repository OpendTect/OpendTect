#ifndef uiprestackanglemute_h
#define uiprestackanglemute_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y. Liu
 Date:		January 2011
 RCS:		$Id: uiprestackanglemute.h,v 1.7 2012-08-03 13:01:05 cvskris Exp $
________________________________________________________________________


-*/

#include "uiprestackprocessingmod.h"
#include "uidialog.h"
#include "uigroup.h"
#include "prestackanglemute.h"

class CtxtIOObj;
class uiCheckBox;
class uiGenInput;
class uiRayTracerSel;
class uiVelSel;

namespace PreStack
{

class Processor;

mClass(uiPreStackProcessing) uiAngleMuteGrp : public uiGroup
{
public:
			uiAngleMuteGrp(uiParent*,AngleMuteBase::Params&,
					bool dooffset = false);

    bool		acceptOK();

protected:
    AngleMuteBase::Params& params_;

    uiRayTracerSel*	raytracerfld_;
    uiVelSel*		velfuncsel_;
    uiGenInput*		cutofffld_;
};


mClass(uiPreStackProcessing) uiAngleMute : public uiDialog
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

