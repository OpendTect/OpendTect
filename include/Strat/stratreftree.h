#ifndef stratreftree_h
#define stratreftree_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Dec 2003
 RCS:		$Id: stratreftree.h,v 1.13 2010-07-05 16:08:07 cvsbruno Exp $
________________________________________________________________________

-*/

#include "stratunitref.h"
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
    

    Strat::UnitRef*     getFromID(int id) 		{ return fnd(id); }
    const Strat::UnitRef* getFromID(int id) const 	{ return fnd(id); } 

    int			getID(const char* code) const;
    void		getUnitIDs(TypeSet<int>&) const;
    const char*		getUnitLvlName(int unid) const;
    Color		getUnitColor(int unid) const;
    
    void		gatherLeavesByTime(const NodeUnitRef&,
	    					ObjectSet<UnitRef>&) const;
    void		getLeavesTimeGaps(const NodeUnitRef&,
					   TypeSet< Interval<float> >&) const;

    bool		addCopyOfUnit(const UnitRef&,bool rev=false);
    bool		addUnit(const char*,const UnitRef::Props&,
	    			bool rev=false);
    bool		addUnit(const char*,const char* unit_dump,
	    			bool rev=false);
    void		setUnitProps(int id,const UnitRef::Props&);
    void		setUnitProps(const char*,const UnitRef::Props&);
    void		removeEmptyNodes(); //!< recommended after add
    bool		write(std::ostream&) const;
    				//!< for printing, export or something.
    				//!< otherwise, use UnitRepository::write()

protected:

    int			getFreeLevelID() const;
    void		constraintUnits(Strat::UnitRef&);
    void		constraintUnitTimes(Strat::NodeUnitRef&);
    void		constraintUnitLvlNames(Strat::UnitRef&);
    void		resetChildrenNames(const NodeUnitRef&); 
    
    Strat::UnitRef*     fnd(int id) const;

    Repos::Source	src_;
    BufferString	treename_;

};


}; // namespace Strat


#endif
