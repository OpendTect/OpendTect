#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		March 2009
________________________________________________________________________

-*/

#include "uiprestackprocessingmod.h"
#include "uibatchprocdlg.h"

class CtxtIOObj;
class uiSeisSel;
class uiPosSubSel;
class uiBatchJobDispatcherSel;


namespace PreStack
{
class uiProcSel;

/*!Dialog to setup a time->depth conversion for volumes on disk. */

mExpClass(uiPreStackProcessing) uiBatchProcSetup : public uiBatchProcDlg
{ mODTextTranslationClass(uiBatchProcSetup);
public:

			uiBatchProcSetup(uiParent*,bool is2d);
			~uiBatchProcSetup();

protected:

    bool		fillPar(IOPar& iop) override;
    bool		prepareProcessing() override;
    void		getJobName(BufferString& jobnm) const override;
    void		setupSelCB(CallBacker*);

    uiProcSel*		chainsel_;
    uiSeisSel*		inputsel_;
    uiSeisSel*		outputsel_;
    uiPosSubSel*	possubsel_;
    uiBatchJobDispatcherSel* batchfld_;

    const bool		is2d_;
};

} // namespace PreStack
