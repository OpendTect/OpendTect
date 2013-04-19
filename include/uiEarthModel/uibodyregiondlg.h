#ifndef uibodyregiondlg_h
#define uibodyregiondlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		October 2011
 RCS:		$Id$
________________________________________________________________________


-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"
#include "uiioobjsel.h"


class MultiID;
class IOObj;
class uiGenInput;
class uiPosSubSel;
class uiPushButton;
class uiTable;


mExpClass(uiEarthModel) uiBodyRegionDlg : public uiDialog
{
public: 
    				uiBodyRegionDlg(uiParent*);
				~uiBodyRegionDlg();

    const MultiID		getBodyMid() const { return outputfld_->key(); }

protected:
    bool			acceptOK(CallBacker*);
    void			addSurfaceCB(CallBacker*);
    void			removeSurfaceCB(CallBacker*);
    void			addSurfaceTableEntry(const IOObj&,
						     bool isfault,char side);
    bool			createImplicitBody();
    void			horModChg(CallBacker*);

    TypeSet<MultiID>		surfacelist_;

    uiIOObjSel*			outputfld_;
    uiPosSubSel*		subvolfld_; 

    uiTable*			table_;
    uiPushButton*		addhorbutton_;
    uiPushButton*		addfltbutton_;
    uiPushButton*		removebutton_;
    uiGenInput*			singlehorfld_;
    bool			singlehoradded_;
};


#endif

