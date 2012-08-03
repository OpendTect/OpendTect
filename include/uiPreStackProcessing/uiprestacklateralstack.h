#ifndef uiprestacklateralstack_h
#define uiprestacklateralstack_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: uiprestacklateralstack.h,v 1.2 2012-08-03 13:01:05 cvskris Exp $
________________________________________________________________________


-*/

#include "uiprestackprocessingmod.h"
#include "uidialog.h"
class uiGenInput;

namespace PreStack
{

class LateralStack;
class Processor;

mClass(uiPreStackProcessing) uiLateralStack : public uiDialog
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

