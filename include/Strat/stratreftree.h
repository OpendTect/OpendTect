#ifndef stratreftree_h
#define stratreftree_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Dec 2003
 RCS:		$Id: stratreftree.h,v 1.6 2008-10-24 11:39:13 cvshelene Exp $
________________________________________________________________________

-*/

#include "stratunitref.h"
#include "repos.h"

namespace Strat
{

class Lithology;
class Level;



/*!\brief Tree of UnitRef's  */

class RefTree : public NodeUnitRef
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

    bool		addCopyOfUnit(const UnitRef&,bool rev=false);
    bool		addUnit(const char* fullcode,const char* unit_dump,
	    			bool rev=false);
    void		removeEmptyNodes(); //!< recommended after add

    void		addLevel(Level*);
    int			nrLevels() const		{ return lvls_.size(); }
    const Level*	levelFromIdx( int idx ) const	{ return lvls_[idx]; }
    const Level*	levelFromID(int id) const;
    const Level*	getLevel(const char*) const;
    const Level*	getLevel(const UnitRef*,bool top=true) const;
    int			getLevelIdx( const Level* lvl ) const
    						{ return lvls_.indexOf( lvl ); }
    void		remove(const Level*&);
    			//!< the pointer will be set to null if found
    void		untieLvlsFromUnit(const UnitRef*,bool);

    bool		write(std::ostream&) const;
    				//!< for printing, export or something.
    				//!< otherwise, use UnitRepository::write()

protected:

    int			getFreeLevelID() const;

    Repos::Source	src_;
    BufferString	treename_;
    ObjectSet<Level>	lvls_;

};


}; // namespace Strat


#endif
