#ifndef uibodyregiondlg_h
#define uibodyregiondlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		October 2011
 RCS:		$Id: uibodyregiondlg.h,v 1.2 2012-01-06 19:50:34 cvsyuancheng Exp $
________________________________________________________________________


-*/

#include "uidialog.h"
#include "ctxtioobj.h"

class MultiID;
class IOObj;
class uiIOObjSel;
class uiTable;
class uiPosSubSel;
class uiPushButton;


mClass uiBodyRegionDlg : public uiDialog
{
public: 
    				uiBodyRegionDlg(uiParent*);
				~uiBodyRegionDlg();
protected:
    bool			acceptOK(CallBacker*);
    void			addSurfaceCB(CallBacker*);
    void			removeSurfaceCB(CallBacker*);
    void			addSurfaceTableEntry(const IOObj&,
						     bool isfault,char side);
    bool			createImplicitBody();

    TypeSet<MultiID>		surfacelist_;

    uiIOObjSel*			outputfld_;
    CtxtIOObj			ctio_;
    uiPosSubSel*		subvolfld_; 

    uiTable*			table_;
    uiPushButton*		addhorbutton_;
    uiPushButton*		addfltbutton_;
    uiPushButton*		removebutton_;
};


#endif
