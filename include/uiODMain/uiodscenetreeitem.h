#ifndef uiodscenetreeitem_h
#define uiodscenetreeitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		May 2006
 RCS:		$Id$
________________________________________________________________________


-*/

#include "uiodtreeitem.h"


mClass uiODSceneTreeItem : public uiODTreeItem
{
public:
    			uiODSceneTreeItem(const char*,int);
    void		updateColumnText(int);

protected:

    bool		showSubMenu();
    bool		isSelectable() const		{ return false; }
    bool		isExpandable() const		{ return false; }
    const char*		parentType() const
			{ return typeid(uiODTreeTop).name(); }
    int			selectionKey() const		{ return displayid_; }

    int			displayid_;
};


#endif
