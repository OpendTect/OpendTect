#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          Jan 2016
 RCS:           $Id $
________________________________________________________________________

-*/

#include "mmprocmod.h"
#include "singlebatchjobdispatch.h"


namespace Batch
{

/*!\brief Base class for Cluster prog definitions. */

mExpClass(MMProc) ClusterProgDef
{ mODTextTranslationClass(ClusterProgDef);
public:

			ClusterProgDef() {}

    virtual bool	isSuitedFor(const char* prognm) const	= 0;
    virtual bool	canHandle( const JobSpec& js ) const
			{ return isSuitedFor( js.prognm_ ); }
    virtual bool	canResume( const JobSpec& js ) const
			{ return false; }

};

/*!\brief kicks off Cluster job dispatcher. */

mExpClass(MMProc) ClusterJobDispatcher : public JobDispatcher
{ mODTextTranslationClass(ClusterJobDispatcher);
public:

			ClusterJobDispatcher();
    virtual		~ClusterJobDispatcher()		{}

    virtual uiString	description() const;
    virtual bool	isSuitedFor(const char*) const;
    virtual bool	canHandle(const JobSpec&) const;
    virtual bool	canResume(const JobSpec&) const;

    mDefaultFactoryInstantiation(JobDispatcher,SingleJobDispatcher,
				 "Cluster Process",tr("Cluster Process"));

    static void         addDef(ClusterProgDef*);

protected:

    virtual bool	launch(ID*);

    int			defIdx(const char* pnm=0) const;

};


} // namespace Batch
