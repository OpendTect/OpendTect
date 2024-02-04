#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"
#include "uicoordsystem.h"
#include "emobject.h"
#include "multiid.h"

class StreamData;
class uiCheckBox;
class uiCheckList;
class uiFileInput;
class uiGenInput;
class uiIOObjSel;
class uiIOObjSelGrp;
class uiMultiSurfaceRead;
class uiUnitSel;


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
    uiIOObjSel*		infld_			= nullptr;
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
