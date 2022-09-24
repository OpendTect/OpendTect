#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiprestackprocessingmod.h"
#include "uidialog.h"
#include "uicoordsystem.h"

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
    Coords::uiCoordSystemSel*	coordsysselfld_;

    bool		acceptOK(CallBacker*) override;
    void		coordTypChngCB(CallBacker*);
    bool		writeAscii();
};

} // namespace PreStack
