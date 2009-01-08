#ifndef uiodscenetreeitem_h
#define uiodscenetreeitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		May 2006
 RCS:		$Id: uiodscenetreeitem.h,v 1.2 2009-01-08 10:47:25 cvsranojay Exp $
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
