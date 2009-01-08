#ifndef uihor2dfrom3ddlg_h
#define uihor2dfrom3ddlg_h
/*
________________________________________________________________________

CopyRight:     (C) dGB Beheer B.V.
Author:        Satyaki Maitra
Date:          July 2008
RCS:           $Id: uihor2dfrom3ddlg.h,v 1.2 2009-01-08 09:04:20 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

namespace EM { class Horizon2D; }
class uiSelection2DParSel;
class uiSurfaceRead;
class uiSurfaceWrite;

mClass uiHor2DFrom3DDlg : public uiDialog
{
public:
    				uiHor2DFrom3DDlg( uiParent* );

protected:
    uiSelection2DParSel*	linesetinpsel_;
    uiSurfaceRead*		hor3dsel_;
    uiSurfaceWrite*		out2dfld_;

    void 			set2DHorizon(EM::Horizon2D&);
    bool			checkFlds();
    EM::Horizon2D* 		create2dHorizon( const char* );
    bool			acceptOK( CallBacker* );
};

#endif
