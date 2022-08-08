#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Mar 2014
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

