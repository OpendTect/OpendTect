#ifndef uistratselunits_h
#define uistratselunits_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          July 2011
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uigroup.h"
#include "stratunitrefiter.h"
class uiStratSelUnitsListItem;
class uiListView;
class uiComboBox;



mClass uiStratSelUnits : public uiGroup
{
public:

    enum Type	{ Simple, Single, Multi };

    mClass Setup
    {
    public:

			Setup( Type t, Strat::UnitRefIter::Pol p )
			    : type_(t)
			    , pol_(p)
			    , maxnrlines_(12)
			    , autoselchildparent_(true)
			    , selectallinitial_(false)
			    , fldtxt_(t==Multi?"Units":"Unit")	{}

	mDefSetupMemb(Type,type)
	mDefSetupMemb(Strat::UnitRefIter::Pol,pol)
	mDefSetupMemb(int,maxnrlines)		//!< only used for tree
	mDefSetupMemb(BufferString,fldtxt)	//!< text next to Simple combo
	mDefSetupMemb(bool,selectallinitial)	//!< only for Multi
	mDefSetupMemb(bool,autoselchildparent)	//!< only for Multi:
			//!< one child -> parent, parent -> all children
			//!< deselect last child -> unselect parent
    };

			uiStratSelUnits(uiParent*,
					const Strat::NodeUnitRef&,
					const Setup&);
			~uiStratSelUnits();

    bool		isSelected(const Strat::UnitRef&) const;
    bool		isPresent(const Strat::UnitRef&) const;
    const Strat::UnitRef* firstSelected() const;
    void		getSelected(ObjectSet<const Strat::UnitRef>&) const;
    void		setCurrent(const Strat::UnitRef&);
    void		setSelected(const Strat::UnitRef&,bool yn=true);
    void		setExpanded(int dpth=mUdf(int)); // not for Simple

    Notifier<uiStratSelUnits>	currentChanged;
    Notifier<uiStratSelUnits>	selectionChanged; //!< Only issued for Multi
    const Strat::UnitRef*	curunit_;

protected:

    uiComboBox*			combo_;
    uiListView*			tree_;
    ObjectSet<uiStratSelUnitsListItem>	lvitms_;
    
    const Strat::NodeUnitRef&	topnode_;
    Setup			setup_;
    bool			doingautosel_;

    void			mkBoxFld();
    void			mkTreeFld();

    void			curChg(CallBacker*);
    void			selChg(CallBacker*);

    inline bool			isMulti() const { return setup_.type_==Multi; }
    void			selRelated(const Strat::UnitRef*,bool);
    void			checkParent(const Strat::UnitRef*);
    void			checkChildren(const Strat::UnitRef*,bool);
    void			unselParentIfLast(const Strat::UnitRef*);

    uiStratSelUnitsListItem*	find(const Strat::UnitRef*);
    const uiStratSelUnitsListItem* find( const Strat::UnitRef* ur ) const
		{ return const_cast<uiStratSelUnits*>(this)->find(ur); }

};


#endif
