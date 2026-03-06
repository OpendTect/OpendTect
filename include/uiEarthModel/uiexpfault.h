#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"

#include "multiid.h"

class uiCheckBox;
class uiCheckList;
class uiFaultSel;
class uiFileInput;
class uiGenInput;
class uiMultiSurfaceRead;
class uiUnitSel;
namespace Coords { class uiCoordSystemSel; }


/*! \brief Dialog for horizon export */

mExpClass(uiEarthModel) uiExportFault : public uiDialog
{ mODTextTranslationClass(uiExportFault);
public:
			uiExportFault(uiParent*,const char* type,bool isbulk);
			~uiExportFault();

    bool		isBulk() const		{ return isbulk_; }

protected:

    uiMultiSurfaceRead* multisurfdepthread_		= nullptr;
    uiMultiSurfaceRead* multisurftimeread_		= nullptr;
    uiFaultSel*		timefld_		= nullptr;
    uiFaultSel*		depthfld_		= nullptr;
    uiGenInput*		zdomypefld_		= nullptr;
    uiGenInput*		coordfld_;
    uiCheckList*	stickidsfld_;
    uiCheckBox*		linenmfld_		= nullptr;
    uiFileInput*	outfld_;
    uiUnitSel*		zunitsel_;

    Coords::uiCoordSystemSel* coordsysselfld_	= nullptr;
    bool		isbulk_;
    uiString		dispstr_;
    StringView		typ_;

    bool		writeAscii();
    bool		getInputMIDs(TypeSet<MultiID>&);

    void		exportCoordSysChgCB(CallBacker*);
    void		inpSelChg(CallBacker*);
    void		zDomainTypeChg(CallBacker*);
    void		initGrpCB(CallBacker*);
    bool		acceptOK(CallBacker*) override;

};
