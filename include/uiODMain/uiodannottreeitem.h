#ifndef uiodannottreeitem_h
#define uiodannottreeitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2005
 RCS:           $Id$
________________________________________________________________________


-*/
#include "uiodmainmod.h"

#include "uioddisplaytreeitem.h"
#include "color.h"

namespace Pick { class Set; }

mExpClass(uiODMain) uiODAnnotParentTreeItem : public uiTreeItem
{
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
{
public:
    const char*		name() const   { return getName(); }
    static const char*	getName()      
			{ return typeid(uiODAnnotTreeItemFactory).name();}
    uiTreeItem*		create() const { return new uiODAnnotParentTreeItem; }
    uiTreeItem*		create(int,uiTreeItem*) const;
};


mExpClass(uiODMain) uiODAnnotTreeItem : public uiODTreeItem
{
    typedef uiODTreeItem  inheritedClass;
public:
    				uiODAnnotTreeItem(const char*);
    				~uiODAnnotTreeItem();

				mMenuOnAnyButton;

protected:
    bool			readPicks(Pick::Set&);
    virtual const char*		parentType() const;
    virtual bool		init();
    void			prepareForShutdown();
    virtual bool		showSubMenu();
    virtual int			defScale() const 		{ return -1; }

    virtual uiTreeItem*		createSubItem(int,Pick::Set&)	= 0;
    virtual const char*		managerName() const		= 0;
    virtual const char*		oldSelKey() const		= 0;



   void			setRemovedCB(CallBacker*);
   void			addPickSet(Pick::Set* ps);
   void			removePickSet(Pick::Set* ps);

   BufferString		typestr_;
};



mExpClass(uiODMain) uiODAnnotSubItem : public uiODDisplayTreeItem
{
public:
    static bool		doesNameExist(const char*);
    static char		createIOEntry(const char* nm,bool overwrite,
	    			    MultiID&,const char* mannm);
    			/*!<\retval -1 error
			    \retval 0 name exists and overwrite is not set.
			    \retval 1 success.
			*/
    Pick::Set*		getSet() { return set_; }

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
    void		setColor(Color);
    void		scaleChg(CallBacker*);

    void		store() const;
    void		storeAs( bool trywithoutdlg=false ) const;

    virtual const char*	managerName() const		= 0;

    float		defscale_;

    MenuItem		scalemnuitem_;
    MenuItem		storemnuitem_;
    MenuItem		storeasmnuitem_;
    Pick::Set*		set_;
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


#define mDefineParentItem(type,typestr,defsz) \
class type##ParentItem : public uiODAnnotTreeItem \
{ \
public: \
		type##ParentItem() \
    		    : uiODAnnotTreeItem(typestr) {} \
protected: \
    uiTreeItem*	createSubItem(int di,Pick::Set& pck) \
    		{ return new type##SubItem(pck,di); } \
    const char*	managerName() const { return type##SubItem::sKeyManager(); } \
    const char*	oldSelKey() const { return typestr; } \
    int		defScale() const 	{ return defsz; } \
}


mDefineParentItem(ScaleBar,"ScaleBar",1000);


#endif
