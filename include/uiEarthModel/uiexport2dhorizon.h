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

class SurfaceInfo;
class uiListBox;
class uiComboBox;
class uiGenInput;
class uiCheckList;
class uiFileInput;
class od_ostream;
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

    uiComboBox*		horselfld_;
    uiListBox*		linenmfld_;
    uiGenInput*		udffld_;
    uiFileInput*	outfld_;
    uiCheckList*	optsfld_;
    uiGenInput*		headerfld_;

    ObjectSet<SurfaceInfo>	hinfos_;

    bool		acceptOK(CallBacker*) override;
    void		horChg(CallBacker*);
    bool		doExport();
    void		writeHeader(od_ostream&);

protected:

    uiIOObjSelGrp*	bulkinfld_;
    Coords::uiCoordSystemSel* coordsysselfld_ = nullptr;

    bool		isbulk_;
    bool		getInputMultiIDs(TypeSet<MultiID>&);
};
