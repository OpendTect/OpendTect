#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2005
________________________________________________________________________


-*/
#include "uiodmainmod.h"

#include "uioddisplaytreeitem.h"
#include "color.h"
#include "pickset.h"

mExpClass(uiODMain) uiODAnnotParentTreeItem : public uiTreeItem
{ mODTextTranslationClass(uiODAnnotParentTreeItem)
public:
			uiODAnnotParentTreeItem();
			~uiODAnnotParentTreeItem();

    int			sceneID() const;

protected:
    bool		init();
    const char*		parentType() const;
    virtual bool	rightClick(uiTreeViewItem*);
};


mExpClass(uiODMain) uiODAnnotTreeItemFactory : public uiODTreeItemFactory
{ mODTextTranslationClass(uiODAnnotTreeItemFactory)
public:
    const char*		name() const   { return getName(); }
    static const char*	getName()
			{ return typeid(uiODAnnotTreeItemFactory).name();}
    uiTreeItem*		create() const { return new uiODAnnotParentTreeItem; }
    uiTreeItem*		create(int,uiTreeItem*) const;
};


mExpClass(uiODMain) uiODAnnotTreeItem : public uiODParentTreeItem
{ mODTextTranslationClass(uiODAnnotTreeItem)
public:
				~uiODAnnotTreeItem();

protected:
				uiODAnnotTreeItem(const uiString&);

    bool			readPicks(Pick::Set&);
    virtual const char*		parentType() const;
    virtual bool		init();
    void			prepareForShutdown();
    virtual bool		showSubMenu();
    virtual int			defScale() const 		{ return -1; }

    virtual uiTreeItem*		createSubItem(int,Pick::Set&)	= 0;
    virtual const char*		managerName() const		= 0;
    virtual const char*		oldSelKey() const		= 0;



   void				setRemovedCB(CallBacker*);
   void				addPickSet(Pick::Set* ps);
   void				removePickSet(Pick::Set* ps);

   uiString			typestr_;
};



mExpClass(uiODMain) uiODAnnotSubItem : public uiODDisplayTreeItem
{mODTextTranslationClass(uiODAnnotSubItem)
public:
    static bool		doesNameExist(const char*);
    static char		createIOEntry(const char* nm,bool overwrite,
				    MultiID&,const char* mannm);
			/*!<\retval -1 error
			    \retval 0 name exists and overwrite is not set.
			    \retval 1 success.
			*/
    RefMan<Pick::Set>	getSet() { return set_; }

protected:
			uiODAnnotSubItem(Pick::Set&,int displayid=-1);
			//!<Pickset becomes mine, if it's not in the mgr
    virtual		~uiODAnnotSubItem();
    void		prepareForShutdown();
    void		removeStuff();
    bool		init();
    virtual const char*	parentType() const		=0;
    virtual void	fillStoragePar(IOPar&) const	{}

    virtual void	clickCB(CallBacker*)		{}
    virtual void	mouseMoveCB(CallBacker*)	{}
    virtual void	rightclickCB(CallBacker*)	{}

    virtual void	createMenu(MenuHandler*,bool istb);
    virtual void	handleMenuCB(CallBacker*);

    virtual bool	hasScale() const		{ return false; }
    virtual void	setScale(float);
    void		setColor(OD::Color);
    void		scaleChg(CallBacker*);

    void		store() const;
    void		storeAs( bool trywithoutdlg=false ) const;

    virtual const char*	managerName() const		= 0;

    float		defscale_;

    MenuItem		scalemnuitem_;
    MenuItem		storemnuitem_;
    MenuItem		storeasmnuitem_;
    RefMan<Pick::Set>	set_;
};


mExpClass(uiODMain) ScaleBarSubItem : public uiODAnnotSubItem
{mODTextTranslationClass(ScaleBarSubItem);
public:
			ScaleBarSubItem(Pick::Set&,int displayid=-1);
    bool		init();
    static const char*	sKeyManager() 	{ return "ScaleBarAnnotations"; }

protected:
			~ScaleBarSubItem()	{ removeStuff(); }

    const char*		parentType() const;
    void		fillStoragePar(IOPar&) const;

    void		createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);
    void		propertyChange(CallBacker*);

    const char*		managerName() const		{ return sKeyManager();}

    MenuItem		propmnuitem_;

};

mExpClass(uiODMain) ArrowSubItem : public uiODAnnotSubItem
{mODTextTranslationClass(ArrowSubItem);
public:

			ArrowSubItem(Pick::Set& pck,int displayid=-1);
    bool		init();

    static const char*	sKeyManager() 	{ return "ArrowAnnotations"; }

protected:
			~ArrowSubItem()	{ removeStuff(); }
    virtual const char*	parentType() const;

    void		fillStoragePar(IOPar&) const;
    void		createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);
    void		propertyChange(CallBacker*);

    MenuItem		propmnuitem_;
    int			arrowtype_;

    bool		hasScale() const	{ return false; }
    const char*		managerName() const	{ return sKeyManager(); }

    static const char*		sKeyArrowType()	{ return "Arrow Type"; }
    static const char*		sKeyLineWidth()	{ return "Line width"; }

};


mExpClass(uiODMain) ImageSubItem : public uiODAnnotSubItem
{mODTextTranslationClass(ImageSubItem)
public:
			ImageSubItem(Pick::Set&,int displayid=-1);
    bool		init();
    static const char*	sKeyManager() 	{ return "ImageAnnotations"; }

protected:
			~ImageSubItem()	{ removeStuff(); }
    const char*		parentType() const;
    void		fillStoragePar(IOPar&) const;

    void		createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);

    void		retrieveFileName(CallBacker*);

    void		updateColumnText(int col);

    bool		hasScale() const		{ return true; }
    const char*		managerName() const		{ return sKeyManager();}

    void		selectFileName() const;

    MenuItem		filemnuitem_;
};


#define mDefineParentItem(type,typestr,defsz,inm) \
class type##ParentItem : public uiODAnnotTreeItem \
{ \
public: \
		type##ParentItem() \
		    : uiODAnnotTreeItem(typestr)\
		{ \
		 mAttachCB( Pick::Mgr().setToBeRemoved, \
		 type##ParentItem::setRemovedCB ); \
		} \
protected: \
    uiTreeItem*	createSubItem(int di,Pick::Set& pck) \
		{ return new type##SubItem(pck,di); } \
    const char*	managerName() const { return type##SubItem::sKeyManager(); } \
    const char* oldSelKey() const { return typestr.getFullString().buf(); } \
    int		defScale() const 	{ return defsz; } \
    void	setRemovedCB(CallBacker*); \
    const char*		iconName() const	{ return inm; } \
}; \


mDefineParentItem(Arrow,mToUiStringTodo("Arrows"),1000,"tree-arrows");
mDefineParentItem(Image,mToUiStringTodo("Image"),1000,"tree-image");
mDefineParentItem(ScaleBar,mToUiStringTodo("Scale Bar"),1000,"tree-scalebar");


