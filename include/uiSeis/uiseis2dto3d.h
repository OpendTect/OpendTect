#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
