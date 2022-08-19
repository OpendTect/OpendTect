#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "mpeenginemod.h"

#include "emposid.h"
#include "emobject.h"
#include "factory.h"
#include "sets.h"
#include "sortedlist.h"
#include "task.h"
#include "trckeyzsampling.h"

class TrcKeyValue;

namespace MPE
{

class SectionSourceSelector;

/*!
\brief SequentialTask to extend the section of an EM object with ID
EM::SectionID.
*/

mExpClass(MPEEngine) SectionExtender : public SequentialTask
{
public:
    virtual			~SectionExtender();

    virtual void		reset();
    virtual void		setDirection(const TrcKeyValue&);
    virtual const TrcKeyValue*	getDirection() const;

    void			setStartPosition(const TrcKey&);
    void			setStartPositions(const TypeSet<TrcKey>&);
    void			excludePositions(const TypeSet<TrcKey>*);
    bool			isExcludedPos(const TrcKey&) const;
    int				nextStep() override;

    void			extendInVolume(const BinID& bidstep,
					       float zstep);

    const TypeSet<TrcKey>&	getAddedPositions() const;
    const TypeSet<TrcKey>&	getAddedPositionsSource() const;

    virtual const TrcKeyZSampling& getExtBoundary() const;
    void			setExtBoundary(const TrcKeyZSampling&);
    void			unsetExtBoundary();

    virtual int			maxNrPosInExtArea() const { return -1; }
    virtual void		preallocExtArea() {}

    const char*			errMsg() const;
    virtual void		fillPar(IOPar&) const {}
    virtual bool		usePar(const IOPar&)	{ return true; }

    void			setUndo(bool yn)	{ setundo_ = yn; }

    mDeprecatedObs
    EM::SectionID		sectionID() const;

protected:
				SectionExtender();

    void			addTarget(const TrcKey& target,
					  const TrcKey& src);
    virtual float		getDepth(const TrcKey& src,
					 const TrcKey& target) const;
    virtual void		prepareDataIfRequired()	{ return; }

    TypeSet<TrcKey>		addedpos_;
    TypeSet<TrcKey>		addedpossrc_;
    TypeSet<TrcKey>		startpos_;

    const TypeSet<TrcKey>*	excludedpos_	= nullptr;

    TrcKeyZSampling		extboundary_;

    BufferString		errmsg;
    bool			setundo_	= true;
};


mDefineFactory1Param( MPEEngine, SectionExtender, EM::EMObject*,
		      ExtenderFactory );

} // namespace MPE
