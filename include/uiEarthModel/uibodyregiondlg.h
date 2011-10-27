#ifndef uibodyregiondlg_h
#define uibodyregiondlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		October 2011
 RCS:		$Id: uibodyregiondlg.h,v 1.1 2011-10-27 21:48:06 cvsyuancheng Exp $
________________________________________________________________________


-*/

#include "uidialog.h"
#include "cubesampling.h"
#include "arraynd.h"
#include "ctxtioobj.h"

class MultiID;
class IOObj;
class uiIOObjSel;
class uiTable;
class uiPosSubSel;
class uiPushButton;

namespace EM { class MarchingCubesSurface; }


mClass uiBodyRegionDlg : public uiDialog
{
public: 
    				uiBodyRegionDlg(uiParent*,
					EM::MarchingCubesSurface&);
				~uiBodyRegionDlg();
protected:
    bool			acceptOK(CallBacker*);
    void			addSurfaceCB(CallBacker*);
    void			removeSurfaceCB(CallBacker*);
    void			addSurfaceTableEntry(const IOObj&,
						     bool isfault,char side);
    bool			createImplicitBody();

    EM::MarchingCubesSurface&	emcs_;
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
