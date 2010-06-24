#ifndef stratreftree_h
#define stratreftree_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Dec 2003
 RCS:		$Id: stratreftree.h,v 1.10 2010-06-24 11:54:00 cvsbruno Exp $
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

    int			getID(const char* code) const;
    void		getUnitIDs(TypeSet<int>&) const;
    void		getLeavesUnitIDs(TypeSet<int>&,
	    				 const NodeUnitRef&,bool) const;
    const char*		getUnitLvlName(int unid) const;
    Color		getUnitColor(int unid) const;

    bool		addCopyOfUnit(const UnitRef&,bool rev=false);
    bool		addUnit(const UnitRef::Props&,bool rev=false);
    bool		addUnit(const char*,const char* unit_dump,
	    			bool rev=false);
    void		setUnitProps(const UnitRef::Props&);
    void		gatherChildrenByTime(const NodeUnitRef&,
	    					ObjectSet<UnitRef>&) const;
    void		removeEmptyNodes(); //!< recommended after add
    bool		write(std::ostream&) const;
    				//!< for printing, export or something.
    				//!< otherwise, use UnitRepository::write()

protected:

    int			getFreeLevelID() const;
    void		constraintUnitTimes(Strat::UnitRef&);

    Repos::Source	src_;
    BufferString	treename_;

};


}; // namespace Strat


#endif
