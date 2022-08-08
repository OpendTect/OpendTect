#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
________________________________________________________________________


-*/

#include "uiprestackprocessingmod.h"
#include "uidialog.h"

class uiGenInput;
class uiIOObjSel;

namespace PreStack
{

class Mute;
class Processor;

mExpClass(uiPreStackProcessing) uiMute : public uiDialog
{ mODTextTranslationClass(uiMute)
public:

    static void		initClass();
			uiMute(uiParent*,Mute*);

protected:

    Mute*		processor_;

    bool		acceptOK(CallBacker*) override;
    static uiDialog*	create(uiParent*,Processor*);

    uiIOObjSel*		mutedeffld_;
    uiGenInput*		topfld_;
    uiGenInput*		taperlenfld_;

};

} // namespace PreStack

