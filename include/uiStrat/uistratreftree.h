#ifndef uistratreftree_h
#define uistratreftree_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          June 2007
 RCS:           $Id: uistratreftree.h,v 1.8 2007-09-12 09:16:17 cvshelene Exp $
________________________________________________________________________

-*/

#include "callback.h"

class ioPixmap;
class uiParent;
class uiListView;
class uiListViewItem;
namespace Strat {
    class RefTree;
    class NodeUnitRef;
    class UnitRef;
}


/*!\brief Displays a Strat::RefTree */

class uiStratRefTree : public CallBacker 
{
public:

			uiStratRefTree(uiParent*,const Strat::RefTree*);
			~uiStratRefTree();

    void		setTree(const Strat::RefTree*,bool force =false);

    uiListView*		listView()		{ return lv_; }
    const uiListView*	listView() const	{ return lv_; }
    void                expand(bool) const;
    void                makeTreeEditable(bool) const;

    const Strat::UnitRef*	findUnit(const char*) const;
    
    Notifier<uiStratRefTree>	itemAdded_;
    Notifier<uiStratRefTree>	itemToBeRemoved_;

protected:

    const Strat::RefTree* tree_;

    uiListView*		lv_;

    void		rClickCB(CallBacker*);

    void		insertSubUnit(uiListViewItem*);
    void		removeUnit(uiListViewItem*);
    void		addNode(uiListViewItem*,const Strat::NodeUnitRef&,bool);
    ioPixmap*		createLevelPixmap(const Strat::UnitRef*) const;
    			//becomes yours!
};


#endif
