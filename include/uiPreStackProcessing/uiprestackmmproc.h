#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiprestackprocessingmod.h"
#include "uimmbatchjobdispatch.h"



mExpClass(uiPreStackProcessing) uiPreStackMMProc : public uiMMBatchJobDispatcher
{ mODTextTranslationClass(uiPreStackMMProc);
public:

                        uiPreStackMMProc(uiParent*,const IOPar&);
			~uiPreStackMMProc();

protected:

    const bool		is2d_;

    bool		initWork(bool) override;

};
