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

#include "uistratmod.h"
#include "uigroup.h"
#include "stratunitrefiter.h"
class uiStratSelUnitsListItem;
class uiTreeView;
class uiComboBox;



mExpClass(uiStrat) uiStratSelUnits : public uiGroup
{
public:

    enum Type	{ Simple, Single, Multi };

    mExpClass(uiStrat) Setup
    {
    public:

			Setup( Type t, Strat::UnitRefIter::Pol p )
			    : type_(t)
			    , pol_(p)
			    , maxnrlines_(12)
			    , autochoosechildparent_(true)
			    , chooseallinitial_(false)
			    , fldtxt_(t==Multi?"Units":"Unit")	{}

	mDefSetupMemb(Type,type)
	mDefSetupMemb(Strat::UnitRefIter::Pol,pol)
	mDefSetupMemb(int,maxnrlines)		//!< only used for tree
	mDefSetupMemb(BufferString,fldtxt)	//!< text next to Simple combo
	mDefSetupMemb(bool,chooseallinitial)	//!< only for Multi
	mDefSetupMemb(bool,autochoosechildparent) //!< only for Multi:
			//!< one child -> parent, parent -> all children
			//!< deselect last child -> unselect parent
    };

			uiStratSelUnits(uiParent*,
					const Strat::NodeUnitRef&,
					const Setup&);
			~uiStratSelUnits();

    bool		isChosen(const Strat::UnitRef&) const;
    bool		isPresent(const Strat::UnitRef&) const;
    const Strat::UnitRef* firstChosen() const;
    void		getChosen(ObjectSet<const Strat::UnitRef>&) const;
    void		setCurrent(const Strat::UnitRef&);
    void		setChosen(const Strat::UnitRef&,bool yn=true);
    void		setExpanded(int dpth=mUdf(int)); // not for Simple

    Notifier<uiStratSelUnits>	currentChanged;
    Notifier<uiStratSelUnits>	unitChosen; //!< Only issued for Multi
    Notifier<uiStratSelUnits>	unitPicked; //!< Only for Single
    const Strat::UnitRef*	curunit_;

protected:

    uiComboBox*			combo_;
    uiTreeView*			tree_;
    ObjectSet<uiStratSelUnitsListItem>	lvitms_;

    const Strat::NodeUnitRef&	topnode_;
    Setup			setup_;
    bool			doingautochoose_;

    void			mkBoxFld();
    void			mkTreeFld();

    void			curChg(CallBacker*);
    void			choiceChg(CallBacker*);
    void			treeFinalSel(CallBacker*);

    inline bool			isMulti() const { return setup_.type_==Multi; }
    void			checkParent(const Strat::UnitRef*);
    void			checkChildren(const Strat::UnitRef*,bool);
    void			chooseRelated(const Strat::UnitRef*,bool);
    void			unChooseParentIfLast(const Strat::UnitRef*);

    uiStratSelUnitsListItem*	find(const Strat::UnitRef*);
    const uiStratSelUnitsListItem* find( const Strat::UnitRef* ur ) const
		{ return const_cast<uiStratSelUnits*>(this)->find(ur); }

};


#endif

