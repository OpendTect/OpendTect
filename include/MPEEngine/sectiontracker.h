#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          23-10-1996
 Contents:      Ranges
________________________________________________________________________

-*/

#include "mpeenginemod.h"
#include "dbkey.h"
#include "task.h"
#include "trckeyzsampling.h"
#include "emposid.h"
#include "geomelement.h"

class BinIDValue;

namespace Attrib { class SelSpec; class SelSpecList; }
namespace EM { class Object; }

namespace MPE
{

class SectionSourceSelector;
class SectionExtender;
class SectionAdjuster;

/*!\brief Tracks sections of EM::Object. */

mExpClass(MPEEngine) SectionTracker
{
public:
				SectionTracker(EM::Object&,
					       SectionSourceSelector* = 0,
					       SectionExtender* = 0,
					       SectionAdjuster* = 0);
    virtual			~SectionTracker();

    EM::Object&			emObject()		{ return emobject_; }
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

    void			setSetupID(const DBKey& id);
    const DBKey&		setupID() const;
    bool			hasInitializedSetup() const;

    void			setSeedOnlyPropagation(bool yn);
    bool			propagatingFromSeedOnly() const;

    const Attrib::SelSpec&	getDisplaySpec() const;
    void			setDisplaySpec(const Attrib::SelSpec&);

    void			getNeededAttribs(Attrib::SelSpecList&) const;
    virtual TrcKeyZSampling	getAttribCube(const Attrib::SelSpec&) const;

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

protected:

    void			getLockedSeeds(TypeSet<EM::PosID>& lockedseeds);

    EM::Object&			emobject_;

    BufferString		errmsg_;
    bool			useadjuster_;
    DBKey			setupid_;
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
