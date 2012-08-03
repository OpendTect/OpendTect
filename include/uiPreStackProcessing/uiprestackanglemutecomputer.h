#ifndef uiprestackanglemutecomputer_h
#define uiprestackanglemutecomputer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		July 2011
 RCS:		$Id: uiprestackanglemutecomputer.h,v 1.2 2012-08-03 13:01:05 cvskris Exp $
________________________________________________________________________


-*/

#include "uiprestackprocessingmod.h"
#include "uidialog.h"

class CtxtIOObj;
class uiGenInput;
class uiIOObjSel;
class uiSeisSubSel;
class uiVelSel;

namespace PreStack
{
    class uiAngleMuteGrp;
    class AngleMuteComputer;

mClass(uiPreStackProcessing) uiAngleMuteComputer : public uiDialog
{
public:
			uiAngleMuteComputer(uiParent*);
			~uiAngleMuteComputer();
protected:

    CtxtIOObj&		outctio_;

    uiAngleMuteGrp*     anglemutegrp_;
    AngleMuteComputer*	processor_;
    uiSeisSubSel*	subsel_;
    uiIOObjSel*		mutedeffld_;

    bool		acceptOK(CallBacker*);
};


}; //namespace

#endif

