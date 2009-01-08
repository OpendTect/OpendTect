#ifndef uiprestackmute_h
#define uiprestackmute_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: uiprestackmute.h,v 1.2 2009-01-08 08:56:15 cvsranojay Exp $
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
