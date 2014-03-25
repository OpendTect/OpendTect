#ifndef mmbatchjobdispatch_h
#define mmbatchjobdispatch_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Dec 2013
 RCS:           $Id$
________________________________________________________________________

-*/

#include "mmprocmod.h"
#include "batchjobdispatch.h"


namespace Batch
{

mExpClass(MMProc) MMProgDef
{
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
{
public:

			MMJobDispatcher();
    virtual		~MMJobDispatcher()		{}

    virtual uiString	description() const;
    virtual bool	isSuitedFor(const char*) const;
    virtual bool	canHandle(const JobSpec&) const;
    virtual bool	canResume(const JobSpec&) const;

    mDefaultFactoryInstantiation(JobDispatcher,MMJobDispatcher,
				 "Multi-Machine","Multi-Machine");

    static void		addDef(MMProgDef*);

protected:

    virtual bool	init();
    virtual bool	launch();

    int			defIdx(const char* pnm=0) const;

};


} // namespace Batch


#endif
