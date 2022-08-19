#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "stratmod.h"
#include "stratunitref.h"
#include "stratlith.h"
#include "stratcontent.h"
#include "repos.h"
#include "od_iosfwd.h"

namespace Strat
{
class LevelSet;


/*!\brief Tree of UnitRef's  */

mExpClass(Strat) RefTree : public NodeOnlyUnitRef
{
public:
			RefTree();
			~RefTree();

    Repos::Source	source() const			{ return src_; }

    LithologySet&	lithologies()			{ return liths_; }
    const LithologySet&	lithologies() const		{ return liths_; }
    ContentSet&		contents()			{ return contents_; }
    const ContentSet&	contents() const		{ return contents_; }

    static const char*	sKeyNoCode()			{ return "<no_code>"; }

    Notifier<UnitRef>	unitAdded;
    Notifier<UnitRef>	unitChanged;
    Notifier<UnitRef>	unitToBeDeleted;

    const LeafUnitRef&	undefLeaf() const		{ return udfleaf_; }
    int			level() const override		{ return 0; }

    static void		getStdNames(BufferStringSet&);
    static RefTree*	createStd(const char*);

    void		createFromLevelSet(const LevelSet&);
			//!< keep contents and lithologies
    const LeavedUnitRef* getLevelSetUnit(const char* lvlnm) const;
    void		addLevelUnit(const Level&);
    void		removeLevelUnit(const Level&);
			// adds & removes units from levels on RefTree created
			// using createFromLevelSet()

    Notifier<RefTree>	objectToBeDeleted;

    friend class	UnitRef;
    friend class	NodeUnitRef;

protected:

    void		initTree();
    void		setToActualTypes();

    LithologySet	liths_;
    ContentSet		contents_;
    LeafUnitRef&	udfleaf_;
    bool		beingdeleted_ = false;

    void		levelSetChgCB(CallBacker*);
    void		reportChange(UnitRef&,bool isrem=false);
    void		reportAdd(UnitRef&);

public:

    Repos::Source	src_;
    BufferString	name_;

				// for printing, export or something.
				// otherwise, use RepositoryAccess::write()
    bool		read(od_istream&);
    bool		write(od_ostream&) const;

    friend class	RefTreeMgr;

    bool		addLeavedUnit(const char*,const char*);
    Strat::LeavedUnitRef* getByLevel(LevelID) const;

};

mGlobal(Strat) void init();	// initializes RefTreeMgr and loads default tree
mGlobal(Strat) const RefTree& RT();
inline RefTree& eRT()	{ return const_cast<RefTree&>( RT() ); }

// Needless to say that if you push, make sure you pop (so afterwards the real
// default RefTree is restored
mGlobal(Strat) void pushRefTree(RefTree*);
mGlobal(Strat) void popRefTree();

mGlobal(Strat) void setRT(RefTree*);
//!< replaces (and deletes) the current RT. No write.
//!< Used by tree manager, and not by *you*. Very very likely not.

mGlobal(Strat) const char* sKeyDefaultTree();
mGlobal(Strat) bool loadDefaultTree();


} // namespace Strat
