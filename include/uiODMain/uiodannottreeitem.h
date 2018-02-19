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
#include "uistrings.h"

namespace Pick { class Set; }

mExpClass(uiODMain) uiODAnnotParentTreeItem : public uiTreeItem
{ mODTextTranslationClass(uiODAnnotParentTreeItem);
public:
			uiODAnnotParentTreeItem();
			~uiODAnnotParentTreeItem();

    int			sceneID() const;
    static uiString	sImage() { return uiStrings::sImage(); }
    static uiString	sArrows() { return tr("Arrows"); }
    static uiString	sScalebar() { return tr("Scale Bar"); }

protected:

    virtual bool	init();
    const char*		parentType() const;
    virtual bool	rightClick(uiTreeViewItem*);

};


mExpClass(uiODMain) uiODAnnotTreeItemFactory : public uiODSceneTreeItemFactory
{ mODTextTranslationClass(uiODAnnotTreeItemFactory);
public:
    const char*		name() const   { return getName(); }
    static const char*	getName()
			{ return typeid(uiODAnnotTreeItemFactory).name();}
    uiTreeItem*		create() const { return new uiODAnnotParentTreeItem; }
    uiTreeItem*		create(int,uiTreeItem*) const;
};


mExpClass(uiODMain) uiODAnnotTreeItem : public uiODSceneTreeItem
{ mODTextTranslationClass(uiODAnnotTreeItem);
    typedef uiODSceneTreeItem  inheritedClass;
public:
				uiODAnnotTreeItem(const uiString&);
				~uiODAnnotTreeItem();

				mMenuOnAnyButton;

protected:

    virtual const char*		parentType() const;
    virtual bool		init();
    virtual bool		showSubMenu();
    virtual int			defScale() const		{ return -1; }

    virtual uiTreeItem*		createSubItem(int,Pick::Set&)	= 0;
    virtual const char*		getCategory() const		= 0;

    Pick::Set*			makeNewSet(const char*) const;
    Pick::Set*			readExistingSet() const;

   uiString			typestr_;
};



mExpClass(uiODMain) uiODAnnotSubItem : public uiODDisplayTreeItem
{mODTextTranslationClass(uiODAnnotSubItem)
public:

    DBKey		getSetID() const;
    Pick::Set&		getSet()			{ return set_; }

protected:

			uiODAnnotSubItem(Pick::Set&,int displayid=-1);
    virtual		~uiODAnnotSubItem();

    virtual bool	init();
    virtual const char*	parentType() const		= 0;
    virtual void	fillStoragePar(IOPar&) const	{}

    virtual void	clickCB(CallBacker*)		{}
    virtual void	mouseMoveCB(CallBacker*)	{}
    virtual void	rightclickCB(CallBacker*)	{}

    virtual void	createMenu(MenuHandler*,bool istb);
    virtual void	handleMenuCB(CallBacker*);

    virtual bool	hasScale() const		= 0;
    virtual void	setScale(float);
    void		setColor(Color);
    void		scaleChg(CallBacker*);

    void		store() const;
    void		storeAs() const;

    virtual const char*	getCategory() const		= 0;

    float		defscale_;

    MenuItem		scalemnuitem_;
    MenuItem		storemnuitem_;
    MenuItem		storeasmnuitem_;

    Pick::Set&		set_;

};


mExpClass(uiODMain) ScaleBarSubItem : public uiODAnnotSubItem
{mODTextTranslationClass(ScaleBarSubItem);
public:
			ScaleBarSubItem(Pick::Set&,int displayid=-1);
    static const char*	sKeyCategory()		{ return "ScaleBarAnnotations";}

protected:

			~ScaleBarSubItem()	{}

    const char*		parentType() const;
    void		fillStoragePar(IOPar&) const;
    virtual bool	init();

    void		createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);
    void		propertyChange(CallBacker*);

    virtual bool	hasScale() const	{ return false; }
    virtual const char*	getCategory() const	{ return sKeyCategory();}

    MenuItem		propmnuitem_;

};

mExpClass(uiODMain) ArrowSubItem : public uiODAnnotSubItem
{mODTextTranslationClass(ArrowSubItem);
public:

			ArrowSubItem(Pick::Set& pck,int displayid=-1);

    static const char*	sKeyCategory()	{ return "ArrowAnnotations"; }

protected:

			~ArrowSubItem()	{}
    virtual const char*	parentType() const;
    virtual bool	init();

    void		fillStoragePar(IOPar&) const;
    void		createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);
    void		propertyChange(CallBacker*);

    MenuItem		propmnuitem_;
    int			arrowtype_;

    virtual bool	hasScale() const	{ return false; }
    virtual const char*	getCategory() const	{ return sKeyCategory();}

    static const char*	sKeyArrowType()	{ return "Arrow Type"; }
    static const char*	sKeyLineWidth()	{ return "Line width"; }

};


mExpClass(uiODMain) ImageSubItem : public uiODAnnotSubItem
{mODTextTranslationClass(ImageSubItem)
public:
			ImageSubItem(Pick::Set&,int displayid=-1);

    static const char*	sKeyCategory()	{ return "ImageAnnotations"; }

protected:

			~ImageSubItem()	{}
    const char*		parentType() const;
    virtual bool	init();
    void		fillStoragePar(IOPar&) const;

    void		createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);

    void		retrieveFileName(CallBacker*);

    void		updateColumnText(int col);

    virtual bool	hasScale() const	{ return true; }
    virtual const char*	getCategory() const	{ return sKeyCategory();}

    void		selectFileName() const;

    MenuItem		filemnuitem_;
};


#define mDefineParentItem(type,typestr,defsz,icnm) \
class type##ParentItem : public uiODAnnotTreeItem \
{ \
public: \
		type##ParentItem() \
		    : uiODAnnotTreeItem(uiODAnnotParentTreeItem::typestr()) {} \
protected: \
    virtual const char*	iconName() const	{ return icnm; } \
    uiTreeItem*	createSubItem(int di,Pick::Set& pck) \
		{ return new type##SubItem(pck,di); } \
    const char*	getCategory() const { return type##SubItem::sKeyCategory(); } \
    int		defScale() const	{ return defsz; } \
}


mDefineParentItem( Arrow,sArrows,1000,"tree-arrows");
mDefineParentItem( Image,sImage,1000,"tree-image");
mDefineParentItem( ScaleBar,sScalebar,1000,"tree-scalebar");
