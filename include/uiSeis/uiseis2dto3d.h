#ifndef uiseis2dto3d_h
#define uiseis2dto3d_h

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Feb 2011
RCS:           $Id$
________________________________________________________________________

-*/


#include "uidialog.h"
#include "uibatchlaunch.h"
#include "uiseismod.h"

class uiCheckBox;
class uiSeisSel;
class uiSeisSubSel;
class uiGenInput;
class CtxtIOObj;
class Seis2DTo3D;

mExpClass(uiSeis) uiSeis2DTo3D : public uiDialog
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



mExpClass(uiSeis) uiSeis2DTo3DFullBatch : public uiFullBatchDialog
{
public:

			uiSeis2DTo3DFullBatch(uiParent*);
			~uiSeis2DTo3DFullBatch();
protected:

    CtxtIOObj&		inctio_;
    CtxtIOObj&		outctio_;

    uiSeisSel*		inpfld_;
    uiSeisSel*		outfld_;
    uiSeisSubSel*	outsubselfld_;

    uiGenInput*		iterfld_;
    uiGenInput*		winfld_;
    uiGenInput*		interpoltypefld_;
    uiCheckBox*		reusetrcsbox_;
    uiGenInput*		velfiltfld_;

    void		typeChg(CallBacker*);
    void		setParFileNameCB(CallBacker*);
    void		setParFileName();
    bool		prepareProcessing();
    bool		checkInpFlds() const;
    bool		fillPar(IOPar&);
    bool		fillSeisPar(IOPar&);
    void		fillParamsPar(IOPar&);

};


#endif

