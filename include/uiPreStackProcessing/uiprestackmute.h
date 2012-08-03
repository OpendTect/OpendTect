#ifndef uiprestackmute_h
#define uiprestackmute_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: uiprestackmute.h,v 1.4 2012-08-03 13:01:06 cvskris Exp $
________________________________________________________________________


-*/

#include "uiprestackprocessingmod.h"
#include "uidialog.h"
class CtxtIOObj;
class uiGenInput;
class uiIOObjSel;

namespace PreStack
{

class Mute;
class Processor;

mClass(uiPreStackProcessing) uiMute : public uiDialog
{
public:

    static void		initClass();
			uiMute(uiParent*,Mute*);

protected:

    Mute*		processor_;
    CtxtIOObj&		ctio_;

    bool		acceptOK(CallBacker*);
    static uiDialog*	create(uiParent*,Processor*);

    uiIOObjSel*		mutedeffld_;
    uiGenInput*		topfld_;
    uiGenInput*		taperlenfld_;

};


}; //namespace

#endif

