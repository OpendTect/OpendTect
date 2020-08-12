#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id$
________________________________________________________________________


-*/

#include "uiprestackprocessingmod.h"
#include "uidialog.h"
class uiGenInput;

namespace PreStack
{

class LateralStack;
class Processor;

mExpClass(uiPreStackProcessing) uiLateralStack : public uiDialog
{ mODTextTranslationClass(uiLateralStack);
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

