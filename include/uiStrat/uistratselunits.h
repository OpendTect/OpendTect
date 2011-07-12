#ifndef uistratselunits_h
#define uistratselunits_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          July 2011
 RCS:           $Id: uistratselunits.h,v 1.1 2011-07-12 11:46:16 cvsbert Exp $
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
			    , fldtxt_("Unit")		{}

	mDefSetupMemb(Type,type)
	mDefSetupMemb(Strat::UnitRefIter::Pol,pol)
	mDefSetupMemb(int,maxnrlines)		//!< only used for tree
	mDefSetupMemb(BufferString,fldtxt)	//!< text next to combo
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
    const Strat::UnitRef* firstSelected() const;
    void		getSelected(ObjectSet<const Strat::UnitRef>&) const;
    void		setCurrent(const Strat::UnitRef&);
    void		setSelected(const Strat::UnitRef&,bool yn=true);

    Notifier<uiStratSelUnits>	currentChanged;
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
    void			selRelated(const Strat::UnitRef*);
    void			checkParent(const Strat::UnitRef*);
    void			checkChildren(const Strat::UnitRef*);
    void			unselParentIfLast(const Strat::UnitRef*);

    uiStratSelUnitsListItem*	find(const Strat::UnitRef*);
    const uiStratSelUnitsListItem* find( const Strat::UnitRef* ur ) const
		{ return const_cast<uiStratSelUnits*>(this)->find(ur); }

};


#endif
