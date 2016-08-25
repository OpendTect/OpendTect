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
#include "odpresentationmgr.h"


mExpClass(uiODMain) uiODParentTreeItem : public uiODTreeItem
{ mODTextTranslationClass(uiODParentTreeItem)
public:
			uiODParentTreeItem(const uiString&);
    virtual		~uiODParentTreeItem();
    bool		init();

    void		getLoadedChildren(TypeSet<MultiID>&) const;
    void		showHideChildren(const MultiID&,bool);
    void		removeChildren(const MultiID&);
    void		addChildren(const TypeSet<MultiID>&);
    bool		selectChild(const MultiID&);
    void		emitChildPRRequest(const MultiID&,
					   OD::PresentationRequestType);

    virtual const char* childObjTypeKey() const			=0;
protected:
    virtual void	objAddedCB(CallBacker*);
    virtual void	objVanishedCB(CallBacker*);
    virtual void	objShownCB(CallBacker*);
    virtual void	objHiddenCB(CallBacker*);
    virtual void	objOrphanedCB(CallBacker*);

    virtual void	addChildItem(const MultiID&)		{}
};

#endif
