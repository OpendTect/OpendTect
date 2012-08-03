#ifndef uiseis2dto3d_h
#define uiseis2dto3d_h

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Feb 2011
RCS:           $Id: uiseis2dto3d.h,v 1.6 2012-08-03 13:01:07 cvskris Exp $
________________________________________________________________________

-*/


#include "uiseismod.h"
#include "uidialog.h"


class uiCheckBox;
class uiSeisSel;
class uiSeisSubSel;
class uiGenInput;
class CtxtIOObj;
class Seis2DTo3D;

mClass(uiSeis) uiSeis2DTo3D : public uiDialog
{
public:

			uiSeis2DTo3D(uiParent*);
			~uiSeis2DTo3D();
protected:

    CtxtIOObj&		inctio_;
    CtxtIOObj&		outctio_;
    Seis2DTo3D&		seis2dto3d_;

    uiSeisSel*		inpfld_;
    uiSeisSubSel*	outsubselfld_;
    uiSeisSel*		outfld_;
    uiGenInput*		iterfld_;
    uiGenInput*		winfld_;
    uiGenInput*		interpoltypefld_;
    uiCheckBox*		reusetrcsbox_;
    uiGenInput*		velfiltfld_;

    bool		acceptOK(CallBacker*);
    void		typeChg( CallBacker* );
};


#endif

