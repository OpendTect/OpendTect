#pragma once
/*
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Satyaki Maitra
Date:          July 2008
________________________________________________________________________

-*/

#include "uiemattribmod.h"
#include "uidialog.h"
#include "dbkey.h"

namespace EM { class Horizon2D; }
class uiSeis2DMultiLineSel;
class uiSurfaceRead;
class uiSurfaceWrite;
class uiCheckBox;

mExpClass(uiEMAttrib) uiHor2DFrom3DDlg : public uiDialog
{ mODTextTranslationClass(uiHor2DFrom3DDlg);
public:
				uiHor2DFrom3DDlg(uiParent*);

    bool			doDisplay() const;
    const DBKey&		getEMObjID() const	    { return emobjid_; }

protected:
    uiSeis2DMultiLineSel*	linesetinpsel_;
    uiSurfaceRead*		hor3dsel_;
    uiSurfaceWrite*		out2dfld_;
    uiCheckBox*			displayfld_;
    DBKey			emobjid_;

    void			set2DHorizon(EM::Horizon2D&);
    bool			checkFlds();
    EM::Horizon2D*		create2dHorizon( const char* );
    bool			acceptOK();

};
