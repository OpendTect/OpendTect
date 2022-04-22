#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
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
    				SectionSourceSelector(
					const EM::SectionID& sid = -1);

    EM::SectionID		sectionID() const;
    virtual void		reset();

    int				nextStep() override;
    const char*			errMsg() const;

    virtual void		fillPar(IOPar&) const {}
    virtual bool		usePar(const IOPar&) { return true; }

    const TypeSet<TrcKey>&	selectedPositions() const;

protected:
    EM::SectionID		sectionid_;
    TypeSet<TrcKey>		selpos_;
    BufferString		errmsg_;
};

} // namespace MPE

