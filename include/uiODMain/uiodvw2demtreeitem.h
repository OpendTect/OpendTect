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
			uiODVw2DEMTreeItem(const DBKey&);
			~uiODVw2DEMTreeItem();

    const DBKey&	emObjectID() const	{ return emid_; }

protected:

    DBKey		emid_;
    void		doSave();
    void		doSaveAs();
    void		renameVisObj();

private:
    void		doStoreObject(bool);

};
