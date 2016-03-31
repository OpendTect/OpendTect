#ifndef singlebatchjobdispatch_h
#define singlebatchjobdispatch_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A. Huck
 Date:		Mar 2016
 RCS:		$Id$
________________________________________________________________________

-*/

#include "mmprocmod.h"
#include "batchjobdispatch.h"


namespace Batch
{

/*!\brief kicks off OD batch jobs in a single process. */

mExpClass(MMProc) SingleJobDispatcherRemote : public SingleJobDispatcher
{ mODTextTranslationClass(SingleJobDispatcherRemote);
public:
			SingleJobDispatcherRemote();
    virtual		~SingleJobDispatcherRemote()	{}

    mDefaultFactoryInstantiation(JobDispatcher,SingleJobDispatcherRemote,
				 "Single Process Remote",tr("Single Process"));

protected:

    virtual bool	launch();
};

} // namespace Batch


#endif
