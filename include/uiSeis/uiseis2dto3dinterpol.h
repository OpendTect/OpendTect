#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"

namespace Batch		{ class JobSpec; }

class uiBatchJobDispatcherSel;
class uiGenInput;
class uiPosSubSel;
class uiSeisSel;
class uiCheckBox;

/*! \brief Dialog for 2D to 3D interpolation */

mExpClass(uiSeis) uiSeis2DTo3DInterPol : public uiDialog
{ mODTextTranslationClass(uiSeis2DTo3DInterPol);
public:
	mDefineFactoryInClass( uiSeis2DTo3DInterPol, factory );

			uiSeis2DTo3DInterPol(uiParent*,uiString&);
			~uiSeis2DTo3DInterPol();

protected:
    uiSeisSel*		inpfld_;
    uiPosSubSel*	possubsel_;
    uiSeisSel*		outfld_;
    uiGenInput*		powfld_;
    uiGenInput*		taperfld_;
    uiCheckBox*		smrtscalebox_;

    uiBatchJobDispatcherSel*	batchfld_;

    Batch::JobSpec&	jobSpec();
    bool		prepareProcessing();
    bool		fillSeisPar();

    virtual bool	fillPar();
    bool		acceptOK(CallBacker*) override;
};
