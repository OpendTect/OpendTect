#ifndef stratreftree_h
#define stratreftree_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Dec 2003
 RCS:		$Id$
________________________________________________________________________

-*/

#include "stratmod.h"
#include "stratunitref.h"
#include "stratlith.h"
#include "stratcontent.h"
#include "repos.h"

namespace Strat
{


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

    Notifier<RefTree>	unitAdded;
    Notifier<RefTree>	unitChanged;
    Notifier<RefTree>	unitToBeDeleted;
    const UnitRef*	notifUnit() const		{ return notifun_; }
    			//!< if null, assume everything has changed

    const LeafUnitRef&	undefLeaf() const		{ return udfleaf_; }
    virtual int		level() const			{ return 0; }

    static void		getStdNames(BufferStringSet&);
    static RefTree*	createStd(const char*);

    Notifier<RefTree>	deleteNotif;

protected:

    void		initTree();
    void		setToActualTypes();

    LithologySet	liths_;
    ContentSet		contents_;
    const UnitRef*	notifun_;
    LeafUnitRef&	udfleaf_;

    void 		levelToBeRemoved(CallBacker*);

public:

    Repos::Source	src_;

    				// for printing, export or something.
    				// otherwise, use RepositoryAccess::write()
    bool		read(std::istream&);
    bool		write(std::ostream&) const;

    friend class	RefTreeMgr;

    void		reportChange(const UnitRef*,bool isrem=false);
    void		reportAdd(const UnitRef*);
    bool		addLeavedUnit(const char*,const char*);
    Strat::LeavedUnitRef* getByLevel(int lvlid) const;//first match

};

mGlobal(Strat) const RefTree& RT();
inline RefTree& eRT()	{ return const_cast<RefTree&>( RT() ); }

// Needless to say that if you push, make sure you pop (so afterwards the real
// default RefTree is restored
mGlobal(Strat) void pushRefTree(RefTree*);
mGlobal(Strat) void popRefTree();

mGlobal(Strat) void setRT(RefTree*);
//!< replaces (and deletes) the current RT. No write.
//!< Used by tree manager, and not by *you*. Very very likely not.


}; // namespace Strat


#endif

