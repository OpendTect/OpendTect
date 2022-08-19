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

class IOObj;
class uiBodySel;
class uiGenInput;
class uiPosSubSel;
class uiPushButton;
class uiTable;


mExpClass(uiEarthModel) uiBodyRegionDlg : public uiDialog
{ mODTextTranslationClass(uiBodyRegionDlg);
public:
				uiBodyRegionDlg(uiParent*);
				~uiBodyRegionDlg();

    MultiID			getBodyMid() const;

protected:
    bool			acceptOK(CallBacker*) override;
    void			addSurfaceCB(CallBacker*);
    void			removeSurfaceCB(CallBacker*);
    void			addSurfaceTableEntry(const IOObj&,
						     bool isfault,char side);
    bool			createImplicitBody();
    void			horModChg(CallBacker*);

    TypeSet<MultiID>		surfacelist_;

    uiBodySel*			outputfld_;
    uiPosSubSel*		subvolfld_;

    uiTable*			table_;
    uiPushButton*		addhorbutton_;
    uiPushButton*		addfltbutton_;
    uiPushButton*		removebutton_;
    uiGenInput*			singlehorfld_;
    bool			singlehoradded_;
};
