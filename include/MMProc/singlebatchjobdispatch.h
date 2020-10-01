#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Dec 2013
 RCS:		$Id$
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

    virtual uiString	description() const;
    virtual bool	isSuitedFor(const char*) const	{ return true; }

    mDefaultFactoryInstantiation(JobDispatcher,SingleJobDispatcher,
				 "Single Process",tr("Single Process"));

    BufferString	remotehost_;
    BufferString	remoteexec_;

protected:

    virtual bool	init();
    virtual bool	launch(ID*);

};

} // namespace Batch
