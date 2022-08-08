#pragma once

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Feb 2011
________________________________________________________________________

-*/


#include "uidialog.h"
#include "uiseismod.h"

namespace Batch		{ class JobSpec; }

class uiBatchJobDispatcherSel;
class uiCheckBox;
class uiGenInput;
class uiPosSubSel;
class uiSeisSel;


/*! \brief Dialog for 2D to 3D interpolation */

mExpClass(uiSeis) uiSeis2DTo3D : public uiDialog
{ mODTextTranslationClass(uiSeis2DTo3D);
public:

			uiSeis2DTo3D(uiParent*);

protected:

    uiSeisSel*		inpfld_;
    uiPosSubSel*	possubsel_;
    uiSeisSel*		outfld_;
    uiBatchJobDispatcherSel*	batchfld_;

    uiGenInput*		iterfld_;
    uiGenInput*		winfld_;
    uiGenInput*		interpoltypefld_;
    uiCheckBox*		reusetrcsbox_;
    uiGenInput*		velfiltfld_;

    void		mkParamsGrp();
    Batch::JobSpec&	jobSpec();
    bool		prepareProcessing();
    bool		fillSeisPar();
    void		fillParamsPar(IOPar&);
    bool		fillPar();
    bool		acceptOK(CallBacker*) override;
    void		typeChg( CallBacker* );
};


