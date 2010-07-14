#ifndef stratreftree_h
#define stratreftree_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Dec 2003
 RCS:		$Id: stratreftree.h,v 1.16 2010-07-14 10:05:13 cvsbruno Exp $
________________________________________________________________________

-*/

#include "stratunitref.h"
#include "stratlevel.h"
#include "repos.h"

namespace Strat
{

class Lithology;


/*!\brief Tree of UnitRef's  */

mClass RefTree : public NodeUnitRef
{
public:

    			RefTree( const char* nm, Repos::Source src )
			: NodeUnitRef(0,"","Top Node")
			, treename_(nm)
			, src_(src)			{}
			~RefTree();

    const BufferString&	treeName() const		{ return treename_; }
    void		setTreeName( const char* nm )	{ treename_ = nm; }
    Repos::Source	source() const			{ return src_; }
    

    Strat::UnitRef*     getByID(int id) 		{ return fnd(id); }
    const Strat::UnitRef* getByID(int id) const 	{ return fnd(id); } 
     
    const LevelSet&     levels() const          	{ return lvls_; }
    LevelSet&           levels()                	{ return lvls_; }

    int			getID(const char* code) const;
    void		getUnitIDs(TypeSet<int>&) const;
    
    void		gatherLeavesByTime(const NodeUnitRef&,
	    					ObjectSet<UnitRef>&) const;
    void		getLeavesTimeGaps(const NodeUnitRef&,
					   TypeSet< Interval<float> >&) const;
    void		assignEqualTimesToUnits(Interval<float>) const;

    bool		addCopyOfUnit(const UnitRef&,bool rev=false);
    bool		addUnit(const char*,const UnitRef::Props&,
	    			bool rev=false);
    bool		addUnit(const char*,const char* unit_dump,
	    			bool rev=false);
    void		setUnitProps(int id,const UnitRef::Props&);
    void		setUnitProps(const char*,const UnitRef::Props&);
    void		setUnitProps(Strat::UnitRef&,const UnitRef::Props&);
    void		removeEmptyNodes(); //!< recommended after add
    bool		write(std::ostream&) const;
    				//!< for printing, export or something.
    				//!< otherwise, use UnitRepository::write()

protected:

    void		constrainUnits(Strat::UnitRef&) const;
    void		constrainUnitTimes(Strat::NodeUnitRef&) const;
    void		constrainUnitLvls(Strat::UnitRef&) const;
    
    Strat::UnitRef*     fnd(int id) const;
    LevelSet            lvls_;

    Repos::Source	src_;
    BufferString	treename_;

};


}; // namespace Strat


#endif
