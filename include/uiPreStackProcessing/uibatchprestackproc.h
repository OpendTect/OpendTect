#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiprestackprocessingmod.h"
#include "uibatchprocdlg.h"

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

			uiBatchProcSetup(uiParent*,OD::GeomSystem,
					int openidx=-1,
					const uiStringSet* usemethods=nullptr);
			/*/< param openidx on popup, launch the GUI of
				   this step index
			     param usemethods selection of items from
				PreStack::Processor::factory()
				(using all if not provided)	 */
			~uiBatchProcSetup();

    bool		isOK() const;

protected:

    bool		fillPar(IOPar&) override;
    bool		prepareProcessing() override;
    void		getJobName(BufferString& jobnm) const override;

    void		initDlgCB(CallBacker*);
    void		setupSelCB(CallBacker*);

    uiProcSel*		chainsel_;
    uiSeisSel*		inputsel_;
    uiSeisSel*		outputsel_;
    uiPosSubSel*	possubsel_;
    uiBatchJobDispatcherSel* batchfld_;

    const OD::GeomSystem gs_;
};

} // namespace PreStack
