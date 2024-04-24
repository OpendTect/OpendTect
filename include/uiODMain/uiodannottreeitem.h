#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"

#include "uioddisplaytreeitem.h"
#include "uistrings.h"
#include "color.h"
#include "pickset.h"

mExpClass(uiODMain) uiODAnnotParentTreeItem : public uiTreeItem
{ mODTextTranslationClass(uiODAnnotParentTreeItem)
public:
			uiODAnnotParentTreeItem();
			~uiODAnnotParentTreeItem();

    SceneID		sceneID() const;

protected:
    bool		init() override;
    const char*		parentType() const override;
    bool		rightClick(uiTreeViewItem*) override;
};


mExpClass(uiODMain) uiODAnnotTreeItemFactory : public uiODTreeItemFactory
{ mODTextTranslationClass(uiODAnnotTreeItemFactory)
public:
    const char*		name() const override	{ return getName(); }
    static const char*	getName()
			{ return typeid(uiODAnnotTreeItemFactory).name();}

    uiTreeItem*		create() const override
			{ return new uiODAnnotParentTreeItem; }

    uiTreeItem*		create(VisID,uiTreeItem*) const;
};


mExpClass(uiODMain) uiODAnnotTreeItem : public uiODParentTreeItem
{ mODTextTranslationClass(uiODAnnotTreeItem)
public:
				~uiODAnnotTreeItem();

protected:
				uiODAnnotTreeItem(const uiString&);

    bool			readPicks(Pick::Set&);
    const char*			parentType() const override;
    bool			init() override;
    void			prepareForShutdown() override;
    bool			showSubMenu() override;
    virtual int			defScale() const 		{ return -1; }

    virtual uiTreeItem*		createSubItem(VisID,Pick::Set&)	= 0;
    virtual const char*		managerName() const		= 0;
    virtual const char*		oldSelKey() const		= 0;



   void				setRemovedCB(CallBacker*);
   void				addPickSet(Pick::Set*);
   void				removePickSet(Pick::Set*);

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
			uiODAnnotSubItem(Pick::Set&,VisID displayid);
			//!<Pickset becomes mine, if it's not in the mgr
    virtual		~uiODAnnotSubItem();

    void		prepareForShutdown() override;
    void		removeStuff();
    bool		init() override;
    virtual void	fillStoragePar(IOPar&) const	{}

    virtual void	clickCB(CallBacker*)		{}
    virtual void	mouseMoveCB(CallBacker*)	{}
    virtual void	rightclickCB(CallBacker*)	{}

    void		createMenu(MenuHandler*,bool istb) override;
    void		handleMenuCB(CallBacker*) override;

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
			ScaleBarSubItem(Pick::Set&,VisID displayid);
    bool		init() override;
    static const char*	sKeyManager() 	{ return "ScaleBarAnnotations"; }

protected:
			~ScaleBarSubItem()	{ removeStuff(); }

    const char*		parentType() const override;
    void		fillStoragePar(IOPar&) const override;

    void		createMenu(MenuHandler*,bool istb) override;
    void		handleMenuCB(CallBacker*) override;
    void		propertyChange(CallBacker*);

    const char*		managerName() const override	{ return sKeyManager();}

    MenuItem		propmnuitem_;

};

mExpClass(uiODMain) ArrowSubItem : public uiODAnnotSubItem
{mODTextTranslationClass(ArrowSubItem);
public:

			ArrowSubItem(Pick::Set& pck,VisID displayid);
    bool		init() override;

    static const char*	sKeyManager() 	{ return "ArrowAnnotations"; }

protected:
			~ArrowSubItem()	{ removeStuff(); }
    const char*		parentType() const override;

    void		fillStoragePar(IOPar&) const override;
    void		createMenu(MenuHandler*,bool istb) override;
    void		handleMenuCB(CallBacker*) override;
    void		propertyChange(CallBacker*);

    MenuItem		propmnuitem_;
    int			arrowtype_;

    bool		hasScale() const override	{ return false; }
    const char*		managerName() const override   { return sKeyManager(); }

    static const char*		sKeyArrowType()	{ return "Arrow Type"; }
    static const char*		sKeyLineWidth()	{ return "Line width"; }

};


mExpClass(uiODMain) ImageSubItem : public uiODAnnotSubItem
{mODTextTranslationClass(ImageSubItem)
public:
			ImageSubItem(Pick::Set&,VisID displayid);
    bool		init() override;
    static const char*	sKeyManager() 	{ return "ImageAnnotations"; }

protected:
			~ImageSubItem()	{ removeStuff(); }
    const char*		parentType() const override;
    void		fillStoragePar(IOPar&) const override;

    void		createMenu(MenuHandler*,bool istb) override;
    void		handleMenuCB(CallBacker*) override;

    void		retrieveFileName(CallBacker*);

    void		updateColumnText(int col) override;

    bool		hasScale() const override	{ return true; }
    const char*		managerName() const override	{ return sKeyManager();}

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
    uiTreeItem* createSubItem(VisID di,Pick::Set& pck) override \
		{ return new type##SubItem(pck,di); } \
    const char* managerName() const override \
		{ return type##SubItem::sKeyManager(); } \
    const char* oldSelKey() const override \
		{ return typestr.getFullString().buf(); } \
    int		defScale() const override	{ return defsz; } \
    void	setRemovedCB(CallBacker*); \
    const char*		iconName() const override	{ return inm; } \
}; \


mDefineParentItem(Arrow,uiStrings::sArrows(), 1000, "tree-arrows");
mDefineParentItem(Image,uiStrings::sImage(),1000,"tree-image");
mDefineParentItem(ScaleBar,uiStrings::sScaleBar(),1000,"tree-scalebar");
