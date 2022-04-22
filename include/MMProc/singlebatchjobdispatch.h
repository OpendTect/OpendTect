#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A. Huck
 Date:		Mar 2016
________________________________________________________________________

-*/

#include "mmprocmod.h"

#include "batchjobdispatch.h"
#include "factory.h"
#include "uistring.h"


namespace Batch
{

/*!\brief kicks off OD batch jobs in a single process. */

mExpClass(MMProc) SingleJobDispatcher : public JobDispatcher
{ mODTextTranslationClass(SingleJobDispatcher);
public:

			SingleJobDispatcher();
    virtual		~SingleJobDispatcher()		{}

    uiString		description() const override;
    bool		isSuitedFor(const char*) const override { return true; }

    mDefaultFactoryInstantiation(JobDispatcher,SingleJobDispatcher,
				 "Single Process",tr("Single Process"));

    BufferString	remotehost_;
    BufferString	remoteexec_;

protected:

    bool		init() override;
    bool		launch(ID*) override;

};

} // namespace Batch
