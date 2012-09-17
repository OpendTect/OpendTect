#ifndef uiprestacklateralstack_h
#define uiprestacklateralstack_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: uiprestacklateralstack.h,v 1.1 2009/11/25 22:22:54 cvskris Exp $
________________________________________________________________________


-*/

#include "uidialog.h"
class uiGenInput;

namespace PreStack
{

class LateralStack;
class Processor;

mClass uiLateralStack : public uiDialog
{
public:
    static void		initClass();
			uiLateralStack(uiParent*,LateralStack*);

protected:
    bool		acceptOK(CallBacker*);
    static uiDialog*	create(uiParent*,Processor*);

    LateralStack*	processor_;
    uiGenInput*		stepoutfld_;
    uiGenInput*		iscrossfld_;
};


}; //namespace

#endif
