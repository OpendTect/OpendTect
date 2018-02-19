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

mExpClass(uiODMain) uiODVw2DEMTreeItem : public uiODVw2DTreeItem
{mODTextTranslationClass(uiODVw2DEMTreeItem)
public:
			~uiODVw2DEMTreeItem();

    const DBKey&	emObjectID() const	{ return emid_; }

protected:
			uiODVw2DEMTreeItem(const DBKey&);

    DBKey		emid_;
    void		doSave();
    void		doSaveAs();
    void		renameVisObj();

    void		displayMiniCtab();
    void		emobjChangeCB(CallBacker*);

    void		showPropDlg();
    void		propChgCB(CallBacker*);

private:

    void		doStoreObject(bool);

};
