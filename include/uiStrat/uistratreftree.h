#ifndef uistratreftree_h
#define uistratreftree_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          June 2007
 RCS:           $Id: uistratreftree.h,v 1.2 2007-07-09 10:12:07 cvshelene Exp $
________________________________________________________________________

-*/

class uiParent;
class uiListView;
class uiListViewItem;
namespace Strat {
    class RefTree;
    class NodeUnitRef;
    class UnitRef;
}


/*!\brief Displays a Strat::RefTree */

class uiStratRefTree 
{
public:

			uiStratRefTree(uiParent*,const Strat::RefTree*);
			~uiStratRefTree();

    void		setTree(const Strat::RefTree*);

    uiListView*		listView()		{ return lv_; }
    const uiListView*	listView() const	{ return lv_; }

    const Strat::UnitRef*	findUnit(const char*) const;

protected:

    const Strat::RefTree* tree_;

    uiListView*		lv_;

    void		addNode(uiListViewItem*,const Strat::NodeUnitRef&);

};


#endif
