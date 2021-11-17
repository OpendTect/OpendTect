#pragma once
/*
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Satyaki Maitra
Date:          July 2008
RCS:           $Id$
________________________________________________________________________

-*/

#include "uiemattribmod.h"
#include "uidialog.h"
#include "emposid.h"

namespace EM { class Horizon2D; }
class uiSeis2DMultiLineSel;
class uiSurfaceRead;
class uiSurfaceWrite;
class uiCheckBox;
class uiGenInput;
class uiHorizonParSel;

mExpClass(uiEMAttrib) uiHor2DFrom3DDlg : public uiDialog
{ mODTextTranslationClass(uiHor2DFrom3DDlg);
public:
    				uiHor2DFrom3DDlg(uiParent*);
				~uiHor2DFrom3DDlg();

    bool			doDisplay() const;
    EM::ObjectID		getEMObjID() const	    { return emobjid_; }
    const TypeSet<EM::ObjectID>& getEMObjIDs();

protected:
    uiSeis2DMultiLineSel*	linesetinpsel_;
    uiSurfaceRead*		hor3dsel_;
    uiSurfaceWrite*		out2dfld_;
    uiCheckBox*			displayfld_;
    EM::ObjectID		emobjid_;

    uiGenInput*			hor2dnmfld_();
    uiHorizonParSel*		hor3dselfld_();
    TypeSet<EM::ObjectID>&	emobjids_();

    void 			set2DHorizon(EM::Horizon2D&);
    bool			checkFlds();
    bool			checkOutNames(BufferStringSet&);
    EM::Horizon2D* 		create2dHorizon( const char* );
    bool			acceptOK( CallBacker* );

};

