#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"
#include "uiioobjselgrp.h"

class uiCheckBox;
class uiComboBox;
class uiFileInput;
class uiGenInput;
class uiListBox;
class uiT2DConvSel;
class uiUnitSel;

class od_ostream;
class SurfaceInfo;
namespace Coords { class uiCoordSystemSel; }


/*! \brief Dialog for 2D horizon export */

mExpClass(uiEarthModel) uiExport2DHorizon : public uiDialog
{ mODTextTranslationClass(uiExport2DHorizon);
public:
			uiExport2DHorizon(uiParent*,
					  const ObjectSet<SurfaceInfo>&,
					  bool isbulk);
			~uiExport2DHorizon();

    bool		isBulk() const		{ return isbulk_; }

protected:

    uiComboBox*		horselfld_		= nullptr;
    uiListBox*		linenmfld_		= nullptr;
    uiCheckBox*		writeudffld_;
    uiGenInput*		udffld_;
    uiCheckBox*		writelinenmfld_;
    uiGenInput*		headerfld_;
    uiGenInput*		doconvfld_;
    uiT2DConvSel*	transfld_;
    uiUnitSel*		unitsel_;
    uiIOObjSelGrp*	bulkinfld_;
    Coords::uiCoordSystemSel* coordsysselfld_	= nullptr;
    uiFileInput*	outfld_;

    ObjectSet<SurfaceInfo>	hinfos_;

    bool		acceptOK(CallBacker*) override;
    void		horChg(CallBacker*);
    void		convCB(CallBacker*);
    void		undefCB(CallBacker*);
    bool		doExport();
    void		writeHeader(od_ostream&);

    bool		isbulk_;
    bool		getInputMultiIDs(TypeSet<MultiID>&);
};
