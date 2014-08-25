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


#include "uiseismod.h"
#include "uidialog.h"

namespace Batch		{ class JobSpec; }

class uiBatchJobDispatcherSel;
class uiCheckBox;
class uiSeisSel;
class uiSeisSubSel;
class uiGenInput;
class CtxtIOObj;
class Seis2DTo3D;
class uiPosSubSel;


/*! \brief Dialog for 2D to 3D interpolation */

mExpClass(uiSeis) uiSeis2DTo3D : public uiDialog
{ mODTextTranslationClass(uiSeis2DTo3D);
public:

			uiSeis2DTo3D(uiParent*);
			~uiSeis2DTo3D();
protected:

    Seis2DTo3D&		seis2dto3d_;

    uiSeisSel*		inpfld_;
    uiSeisSubSel*	outsubselfld_;
    uiSeisSel*		outfld_;
    uiGenInput*		iterfld_;
    uiGenInput*		winfld_;
    uiGenInput*		interpoltypefld_;
    uiCheckBox*		reusetrcsbox_;
    uiGenInput*		velfiltfld_;

    uiPosSubSel*	possubsel_;
    uiBatchJobDispatcherSel*	batchfld_;

    void		mkParamsGrp();
    Batch::JobSpec&	jobSpec();
    bool		prepareProcessing();
    bool		fillSeisPar();
    void		fillParamsPar(IOPar&);
    bool		fillPar();
    bool		acceptOK(CallBacker*);
    void		typeChg( CallBacker* );
};


#endif

