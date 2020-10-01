#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Dec 2013
________________________________________________________________________

-*/

#include "mmprocmod.h"
#include "batchjobdispatch.h"


namespace Batch
{

mExpClass(MMProc) MMProgDef
{ mODTextTranslationClass(MMProgDef);
public:

			MMProgDef( const char* mmpnm )
			    : mmprognm_(mmpnm)			{}

    virtual bool	isSuitedFor(const char* prognm) const	= 0;
    virtual bool	canHandle( const JobSpec& js ) const
			{ return isSuitedFor( js.prognm_ ); }
    virtual bool	canResume( const JobSpec& js ) const
			{ return false; }

    BufferString	mmprognm_;

};


/*!\brief kicks off MM batch job dispatcher. */

mExpClass(MMProc) MMJobDispatcher : public JobDispatcher
{ mODTextTranslationClass(MMJobDispatcher);
public:

			MMJobDispatcher();
    virtual		~MMJobDispatcher()		{}

    virtual uiString	description() const;
    virtual bool	isSuitedFor(const char*) const;
    virtual bool	canHandle(const JobSpec&) const;
    virtual bool	canResume(const JobSpec&) const;

    mDefaultFactoryInstantiation(JobDispatcher,MMJobDispatcher,
				 "Distributed",tr("Distributed"));

    static void		addDef(MMProgDef*);

protected:

    virtual bool	init();
    virtual bool	launch(ID*);

    int			defIdx(const char* pnm=0) const;

};


} // namespace Batch
