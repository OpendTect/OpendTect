#ifndef uiodparenttreeitem_h
#define uiodparenttreeitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		August 2016
________________________________________________________________________


-*/

#include "uiodtreeitem.h"

class SaveableManager;


mExpClass(uiODMain) uiODParentTreeItem : public uiODTreeItem
{ mODTextTranslationClass(uiODParentTreeItem)
public:
			uiODParentTreeItem(const uiString&);
    void		setObjectManager(SaveableManager*);

    void		getLoadedChildren(TypeSet<MultiID>&) const;
    void		showHideChildren(const MultiID&,bool);
    void		removeChildren(const MultiID&);
    void		addChildren(const TypeSet<MultiID>&);
    bool		selectChild(const MultiID&);
protected:
    virtual void	objAddedCB(CallBacker*);
    virtual void	objVanishedCB(CallBacker*);
    virtual void	objShownCB(CallBacker*);
    virtual void	objHiddenCB(CallBacker*);
    virtual void	objOrphanedCB(CallBacker*);

    virtual void	addChildItem(const MultiID&)		{}

    SaveableManager*	objectmgr_;
};

#endif
