#ifndef uiprestackmute_h
#define uiprestackmute_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: uiprestackmute.h,v 1.3 2009/07/22 16:01:22 cvsbert Exp $
________________________________________________________________________


-*/

#include "uidialog.h"
class CtxtIOObj;
class uiGenInput;
class uiIOObjSel;

namespace PreStack
{

class Mute;
class Processor;

mClass uiMute : public uiDialog
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
