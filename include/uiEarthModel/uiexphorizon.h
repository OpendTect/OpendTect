#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"
#include "stringview.h"

class uiFileInput;
class uiGenInput;
class uiIOObjSelGrp;
class uiSurfaceRead;
class uiUnitSel;
class uiPushButton;
class uiT2DConvSel;
namespace Coords { class uiCoordSystemSel; }

/*! \brief Dialog for horizon export */

mExpClass(uiEarthModel) uiExportHorizon : public uiDialog
{ mODTextTranslationClass(uiExportHorizon);
public:
			uiExportHorizon(uiParent*,bool isbulk);
			~uiExportHorizon();

    bool		isBulk() const		{ return isbulk_; }

protected:

    uiSurfaceRead*	infld_;
    uiFileInput*	outfld_;
    uiGenInput*		headerfld_;
    uiGenInput*		typfld_;
    uiGenInput*		zfld_;
    uiPushButton*	settingsbutt_;
    uiUnitSel*		unitsel_;
    uiGenInput*		udffld_;
    uiT2DConvSel*	transfld_;
    uiIOObjSelGrp*	bulkinfld_;
    Coords::uiCoordSystemSel* coordsysselfld_;

    BufferString	gfname_;
    BufferString	gfcomment_;

    bool		acceptOK(CallBacker*) override;
    void		typChg(CallBacker*);
    void		addZChg(CallBacker*);
    void		attrSel(CallBacker*);
    void		settingsCB(CallBacker*);
    void		inpSel(CallBacker*);
    void		writeHeader(od_ostream&);
    bool		writeAscii();
    bool		getInputMIDs(TypeSet<MultiID>&);

    bool		isbulk_;

    StringView		getZDomain() const;
};
