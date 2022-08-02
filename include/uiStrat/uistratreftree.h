#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          June 2007
________________________________________________________________________

-*/

#include "uistratmod.h"

#include "callback.h"
#include "ranges.h"
#include "stratunitref.h"
#include "uistring.h"

class uiPixmap;
class uiParent;
class uiTreeView;
class uiTreeViewItem;
namespace Strat {
    class RefTree;
    class NodeUnitRef;
}


/*!\brief Displays a Strat::RefTree */

mExpClass(uiStrat) uiStratRefTree : public CallBacker
{ mODTextTranslationClass(uiStratRefTree)
public:

			uiStratRefTree(uiParent*);
			~uiStratRefTree();

    void		setTree();
    void		setTree(Strat::RefTree&,bool force =false);
    void		setName(const char*);
    const char*		name() const;

    const Strat::RefTree* tree() const		{ return tree_; }

    uiTreeView*		treeView()		{ return lv_; }
    const uiTreeView*	treeView() const	{ return lv_; }
    uiTreeViewItem*	getLVItFromFullCode(const char*) const;
    void		expand(bool) const;
    void		makeTreeEditable(bool) const;
    void		handleMenu(uiTreeViewItem*);
    void		updateUnitsPixmaps();
    void		updateLithoCol();
    void		moveUnit(bool);
    bool		canMoveUnit(bool);

    bool		anyChg() const		{ return anychange_; }
    void		setNoChg()		{ anychange_ = false; }

    void		setEntranceDefaultTimes();
    bool		haveTimes() const;

protected:

    Strat::RefTree*	tree_;

    uiTreeView*		lv_;
    bool		anychange_;

    void		rClickCB(CallBacker*);
    void		mousePressedCB(CallBacker*);

    void		insertSubUnit(uiTreeViewItem*);
    void		subdivideUnit(uiTreeViewItem*);
    void		updateUnitProperties(uiTreeViewItem*);
    void		removeUnit(uiTreeViewItem*);
    void		assignLevelBoundary(uiTreeViewItem*);
    bool		isLeaved(uiTreeViewItem*) const;

    void		setUnitLvl(const char*);

    BufferString	getFullCodeFromLVIt(const uiTreeViewItem*) const;
    void		insertUnitInLVIT(uiTreeViewItem*,int,
					 const Strat::UnitRef&) const;

    void		addNode(uiTreeViewItem*,const Strat::NodeUnitRef&,bool);
    uiPixmap*		createUnitPixmap(const OD::Color&) const;
			//becomes yours!
    Strat::NodeUnitRef* replaceUnit(Strat::NodeUnitRef&,bool byleaved);
    void		addLithologies(Strat::LeavedUnitRef&,
				       const TypeSet<Strat::LithologyID>&);

    void		setNodesDefaultTimes(const Strat::NodeUnitRef&);
    void		getAvailableTime(const Strat::NodeUnitRef&,
					 Interval<float>&) const;
    void		ensureUnitTimeOK(Strat::NodeUnitRef&);
    int			getChildIdxFromTime(const Strat::NodeUnitRef&,
					    float) const;

    friend class	uiStratDispToTree;
};


