#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          May 2008
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"
#include "uicoordsystem.h"
#include "emobject.h"
#include "multiid.h"

class CtxtIOObj;
class StreamData;
class uiCheckBox;
class uiCheckList;
class uiFileInput;
class uiGenInput;
class uiIOObjSel;
class uiIOObjSelGrp;
class uiT2DConvSel;
class uiUnitSel;


/*! \brief Dialog for horizon export */

mExpClass(uiEarthModel) uiExportFault : public uiDialog
{ mODTextTranslationClass(uiExportFault);
public:
			uiExportFault(uiParent*,const char* type,bool isbulk);
			~uiExportFault();

    bool		isBulk() const		{ return isbulk_; }

protected:

    uiIOObjSelGrp*	bulkinfld_;
    uiIOObjSel*		infld_;
    uiGenInput*		coordfld_;
    uiCheckList*	stickidsfld_;
    uiCheckBox*		linenmfld_;
    uiFileInput*	outfld_;
    uiUnitSel*		zunitsel_;
    uiGenInput*		zfld_;
    uiT2DConvSel*	transfld_;
    Coords::uiCoordSystemSel* coordsysselfld_;

    CtxtIOObj&		ctio_;
    bool		getInputMIDs(TypeSet<MultiID>&);

    void		addZChg(CallBacker*);
    void		exportCoordSysChgCB(CallBacker*);
    StringView		getZDomain() const;

    bool		acceptOK(CallBacker*) override;

    bool		writeAscii();
    bool		isbulk_;
    uiString		dispstr_;
    StringView		typ_;
};

