#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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

    static uiString	sMMProcDesc()
			    { return tr("Distributed Computing Service"); }

};


/*!\brief kicks off MM batch job dispatcher. */

mExpClass(MMProc) MMJobDispatcher : public JobDispatcher
{ mODTextTranslationClass(MMJobDispatcher);
public:

			MMJobDispatcher();
    virtual		~MMJobDispatcher()		{}

    uiString		description() const override;
    bool		isSuitedFor(const char*) const override;
    bool		canHandle(const JobSpec&) const override;
    bool		canResume(const JobSpec&) const override;

    mDefaultFactoryInstantiation(JobDispatcher,MMJobDispatcher,
				 "Distributed",tr("Distributed"));

    static void		addDef(MMProgDef*);

protected:

    bool		init() override;
    bool		launch(ID*) override;

    int			defIdx(const char* pnm=0) const;

};

} // namespace Batch
