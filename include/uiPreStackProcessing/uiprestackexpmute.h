#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		June 2008
________________________________________________________________________

-*/

#include "uiprestackprocessingmod.h"
#include "uidialog.h"
#include "uicoordsystem.h"

class CtxtIOObj;
class uiFileInput;
class uiGenInput;
class uiIOObjSel;

namespace PreStack
{

mExpClass(uiPreStackProcessing) uiExportMute : public uiDialog
{ mODTextTranslationClass(uiExportMute);
public:
			uiExportMute(uiParent*);
			~uiExportMute();

protected:

    uiIOObjSel*		infld_;
    uiGenInput*		coordfld_;
    uiFileInput*	outfld_;
    Coords::uiCoordSystemSel*	    coordsysselfld_;

    CtxtIOObj&		ctio_;

    bool		acceptOK(CallBacker*) override;
    void		coordTypChngCB(CallBacker*);
    bool		writeAscii();
};

} // namespace PreStack
