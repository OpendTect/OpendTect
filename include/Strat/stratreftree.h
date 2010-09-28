#ifndef stratreftree_h
#define stratreftree_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Dec 2003
 RCS:		$Id: stratreftree.h,v 1.21 2010-09-28 04:59:01 cvsranojay Exp $
________________________________________________________________________

-*/

#include "stratunitref.h"
#include "stratlith.h"
#include "repos.h"

namespace Strat
{

class Lithology;


/*!\brief Tree of UnitRef's  */

mClass RefTree : public NodeOnlyUnitRef
{
public:

			RefTree();

    Repos::Source	source() const			{ return src_; }
    
    LithologySet&	lithologies()			{ return liths_; }
    const LithologySet&	lithologies() const		{ return liths_; }

    static const char*	sKeyNoCode()			{ return "<no_code>"; }

    Notifier<RefTree>	unitAdded;
    Notifier<RefTree>	unitChanged;
    Notifier<RefTree>	unitToBeDeleted;

    const UnitRef*	notifUnit() const		{ return notifun_; }
    			//!< if null, assume everything has changed

protected:

    void		initTree();
    bool		addUnit(const char*,const char* dump_str,UnitRef::Type);
    void		setToActualTypes();

    LithologySet	liths_;
    const UnitRef*	notifun_;

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
