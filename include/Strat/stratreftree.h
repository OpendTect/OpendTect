#ifndef stratreftree_h
#define stratreftree_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Dec 2003
 RCS:		$Id: stratreftree.h,v 1.25 2011-07-11 13:30:11 cvsbert Exp $
________________________________________________________________________

-*/

#include "stratunitref.h"
#include "stratlith.h"
#include "repos.h"

namespace Strat
{

class Lithology;
class LeafUnitRef;


/*!\brief Tree of UnitRef's  */

mClass RefTree : public NodeOnlyUnitRef
{
public:

			RefTree();
			~RefTree();

    Repos::Source	source() const			{ return src_; }
    
    LithologySet&	lithologies()			{ return liths_; }
    const LithologySet&	lithologies() const		{ return liths_; }

    static const char*	sKeyNoCode()			{ return "<no_code>"; }

    Notifier<RefTree>	unitAdded;
    Notifier<RefTree>	unitChanged;
    Notifier<RefTree>	unitToBeDeleted;
    const UnitRef*	notifUnit() const		{ return notifun_; }
    			//!< if null, assume everything has changed

    const LeafUnitRef&	undefLeaf() const		{ return udfleaf_; }
    virtual int		level() const			{ return 0; }

protected:

    void		initTree();
    bool		addLeavedUnit(const char*,const char*);
    void		setToActualTypes();

    LithologySet	liths_;
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

};

mGlobal const RefTree& RT();
inline RefTree& eRT()	{ return const_cast<RefTree&>( RT() ); }


}; // namespace Strat


#endif
