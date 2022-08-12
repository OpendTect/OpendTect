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
#include "trckeyzsampling.h"
#include "emposid.h"
#include "geomelement.h"

class BinIDValue;

namespace Attrib { class SelSpec; }
namespace EM { class EMObject; }

namespace MPE
{

class SectionSourceSelector;
class SectionExtender;
class SectionAdjuster;

/*!
\brief Tracks sections of EM::EMObject with ID EM::SectionID.
*/

mExpClass(MPEEngine) SectionTracker
{
public:
				SectionTracker(EM::EMObject&,
					       SectionSourceSelector* = 0,
					       SectionExtender* = 0,
					       SectionAdjuster* = 0);
    virtual			~SectionTracker();

    EM::EMObject&		emObject()		{ return emobject_; }
    virtual bool		init();

    void			reset();

    SectionSourceSelector*	selector();
    const SectionSourceSelector* selector() const;
    SectionExtender*		extender();
    const SectionExtender*	extender() const;
    SectionAdjuster*		adjuster();
    const SectionAdjuster*	adjuster() const;

    virtual bool		select();
    virtual bool		extend();
    virtual bool		adjust();
    const char*			errMsg() const;

    void			useAdjuster(bool yn);
    bool			adjusterUsed() const;

    void			setSetupID(const MultiID& id);
    const MultiID&		setupID() const;
    bool			hasInitializedSetup() const;

    void			setSeedOnlyPropagation(bool yn);
    bool			propagatingFromSeedOnly() const;

    const Attrib::SelSpec&	getDisplaySpec() const;
    void			setDisplaySpec(const Attrib::SelSpec&);

    void			getNeededAttribs(
					TypeSet<Attrib::SelSpec>&) const;
    virtual TrcKeyZSampling	getAttribCube(const Attrib::SelSpec&) const;

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

    mDeprecated("Use without SectionID")
				SectionTracker(EM::EMObject& emobj,
					       const EM::SectionID&,
					       SectionSourceSelector* sss= 0,
					       SectionExtender* se= 0,
					       SectionAdjuster* sa= 0)
				    : SectionTracker(emobj,sss,se,sa)	{}

    mDeprecatedObs
    EM::SectionID		sectionID() const;

protected:

    void			getLockedSeeds(TypeSet<EM::SubID>& lockedseeds);

    EM::EMObject&		emobject_;
    EM::SectionID		sid_		= EM::SectionID::def();

    BufferString		errmsg_;
    bool			useadjuster_;
    MultiID			setupid_;
    Attrib::SelSpec&		displayas_;
    bool			seedonlypropagation_;

    SectionSourceSelector*	selector_;
    SectionExtender*		extender_;
    SectionAdjuster*		adjuster_;


    static const char*		trackerstr;
    static const char*		useadjusterstr;
    static const char*		seedonlypropstr;
};

} // namespace MPE

