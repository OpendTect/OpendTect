#ifndef uistratreftree_h
#define uistratreftree_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          June 2007
 RCS:           $Id: uistratreftree.h,v 1.1 2007-06-21 16:17:28 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "bufstringset.h"

class uiListView;
class uiListViewItem;
namespace Strat {
    class RefTree;
    class NodeUnitRef;
}


/*!\brief Displays a Strat::RefTree */

class uiStratRefTree : public uiGroup
{
public:

			uiStratRefTree(uiParent*,const Strat::RefTree*);

    void		setTree(const Strat::RefTree*);

    uiListView*		listView()	{ return lv_; }

protected:

    const Strat::RefTree* tree_;

    uiListView*		lv_;

    void		addNode(uiListViewItem*,const Strat::NodeUnitRef&);

};


#endif
