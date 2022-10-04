#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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

    virtual		~ClusterProgDef();

    virtual bool	isSuitedFor(const char* prognm) const	= 0;
    virtual bool	canHandle( const JobSpec& js ) const
			{ return isSuitedFor( js.prognm_ ); }
    virtual bool	canResume( const JobSpec& js ) const
			{ return false; }

protected:
			ClusterProgDef();

};

/*!\brief kicks off Cluster job dispatcher. */

mExpClass(MMProc) ClusterJobDispatcher : public JobDispatcher
{ mODTextTranslationClass(ClusterJobDispatcher);
public:

			ClusterJobDispatcher();
			~ClusterJobDispatcher();

    uiString		description() const override;
    bool		isSuitedFor(const char*) const override;
    bool		canHandle(const JobSpec&) const override;
    bool		canResume(const JobSpec&) const override;

    mDefaultFactoryInstantiation(JobDispatcher,SingleJobDispatcher,
				 "Cluster Process",tr("Cluster Process"));

    static void         addDef(ClusterProgDef*);

protected:

    bool		launch(ID*) override;

    int			defIdx(const char* pnm=0) const;

};

} // namespace Batch
