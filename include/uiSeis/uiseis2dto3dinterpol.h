#pragma once

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:	       Feb 2011
________________________________________________________________________

-*/

#include "uidialog.h"
#include "uiseismod.h"

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
