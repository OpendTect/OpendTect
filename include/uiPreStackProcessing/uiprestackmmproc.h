#ifndef uiprestackmmproc_h
#define uiprestackmmproc_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Mar 2014
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiprestackprocessingmod.h"
#include "uimmbatchjobdispatch.h"



mExpClass(uiPreStackProcessing) uiPreStackMMProc : public uiMMBatchJobDispatcher
{
public:

                        uiPreStackMMProc(uiParent*,const IOPar&);
			~uiPreStackMMProc();

protected:

    const bool		is2d_;

    virtual bool	initWork(bool);
    virtual bool	prepareCurrentJob();
    virtual Executor*	getPostProcessor() const;
    virtual bool	needConfirmEarlyStop() const;

};

#endif

