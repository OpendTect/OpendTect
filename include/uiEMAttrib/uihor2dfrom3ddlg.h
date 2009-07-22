#ifndef uihor2dfrom3ddlg_h
#define uihor2dfrom3ddlg_h
/*
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Satyaki Maitra
Date:          July 2008
RCS:           $Id: uihor2dfrom3ddlg.h,v 1.4 2009-07-22 16:01:21 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "emposid.h"

namespace EM { class Horizon2D; }
class uiSelection2DParSel;
class uiSurfaceRead;
class uiSurfaceWrite;
class uiCheckBox;

mClass uiHor2DFrom3DDlg : public uiDialog
{
public:
    				uiHor2DFrom3DDlg(uiParent*);

    bool			doDisplay() const;
    EM::ObjectID		getEMObjID() const	    { return emobjid_; }

protected:
    uiSelection2DParSel*	linesetinpsel_;
    uiSurfaceRead*		hor3dsel_;
    uiSurfaceWrite*		out2dfld_;
    uiCheckBox*			displayfld_;
    EM::ObjectID		emobjid_;

    void 			set2DHorizon(EM::Horizon2D&);
    bool			checkFlds();
    EM::Horizon2D* 		create2dHorizon( const char* );
    bool			acceptOK( CallBacker* );
   
};

#endif
