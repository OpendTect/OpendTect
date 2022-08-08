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

namespace PreStack
{

class AGC;
class Processor;

mExpClass(uiPreStackProcessing) uiAGC : public uiDialog
{ mODTextTranslationClass(uiAGC);
public:
    static void		initClass();
			uiAGC(uiParent*,AGC*);

protected:
    bool		acceptOK(CallBacker*) override;
    static uiDialog*	create(uiParent*,Processor*);

    AGC*		processor_;
    uiGenInput*		windowfld_;
    uiGenInput*		lowenergymute_;
};

} // namespace PreStack

