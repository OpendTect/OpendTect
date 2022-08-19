#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "mpeenginemod.h"
#include "task.h"
#include "bufstring.h"
#include "emposid.h"
#include "sets.h"
#include "trckey.h"


namespace MPE
{

/*!
\brief SequentialTask to select source of an EM section with ID EM::SectionID.
*/

mExpClass(MPEEngine) SectionSourceSelector : public SequentialTask
{
public:
    virtual void		reset();

    int				nextStep() override;
    const char*			errMsg() const;

    virtual void		fillPar(IOPar&) const {}
    virtual bool		usePar(const IOPar&) { return true; }

    const TypeSet<TrcKey>&	selectedPositions() const;

    mDeprecatedObs
    EM::SectionID		sectionID() const;
protected:
				SectionSourceSelector();

    TypeSet<TrcKey>		selpos_;
    BufferString		errmsg_;
};

} // namespace MPE
