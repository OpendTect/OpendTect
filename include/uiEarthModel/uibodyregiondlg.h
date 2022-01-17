#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		October 2011
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
    bool			acceptOK(CallBacker*);
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


