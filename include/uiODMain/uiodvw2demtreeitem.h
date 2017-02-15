#pragma once

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	DZH
 Date:		Apr 2016
________________________________________________________________________

-*/



#include "uiodmainmod.h"
#include "uiodvw2dtreeitem.h"
#include "emposid.h"

mExpClass(uiODMain) uiODVw2DEMTreeItem : public uiODVw2DTreeItem
{mODTextTranslationClass(uiODVw2DEMTreeItem);
public:
			uiODVw2DEMTreeItem(const EM::ObjectID&);
			~uiODVw2DEMTreeItem();

    EM::ObjectID	emObjectID() const	{ return emid_; }

protected:

    EM::ObjectID        emid_;
    void		doSave();
    void		doSaveAs();
    void		renameVisObj();

private:

    void		doStoreObject(bool);

};
