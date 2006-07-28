#ifndef treeitem_h
#define treeitem_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          January 2005
 RCS:           $Id: treeitem.h,v 1.2 2006-07-28 21:58:28 cvskris Exp $
________________________________________________________________________


-*/

#include "uioddisplaytreeitem.h"
#include "color.h"

class uiPopupMenu;
namespace Pick { class Set; }

namespace Annotations
{

class ParentTreeItem : public uiTreeItem
{
public:
			ParentTreeItem();
			~ParentTreeItem();

    int			sceneID() const;

protected:
    bool		init();
    const char*		parentType() const;
};


class TreeItemFactory : public uiODTreeItemFactory
{
public:
    const char*		name() const   { return getName(); }
    static const char*	getName()      { return typeid(TreeItemFactory).name();}
    uiTreeItem*		create() const { return new ParentTreeItem; }
    uiTreeItem*		create(int,uiTreeItem*) const;
};


class AnnotTreeItem : public uiODTreeItem
{
public:
    			AnnotTreeItem(const char*);
    			~AnnotTreeItem();

protected:
    virtual const char*	parentType() const;
    virtual bool	init();
    void		prepareForShutdown();
    virtual bool	showSubMenu();

    virtual uiTreeItem*	createSubItem(int,Pick::Set&)	= 0;
    virtual const char*	managerName() const		= 0;


    bool		readPicks(Pick::Set&);

   void			setAddedCB(CallBacker*);
   void			setRemovedCB(CallBacker*);

   BufferString		typestr_;
   static int		defcolnr;

};



class SubItem : public uiODDisplayTreeItem
{
public:
    static bool		doesNameExist(const char*,const char* mannm);
    static char		createIOEntry(const char* nm,bool overwrite,
	    			    MultiID&,const char* mannm);
    			/*!<\retval -1 error
			    \retval 0 name exists and overwrite is not set.
			    \retval 1 success.
			*/

protected:
    			SubItem(Pick::Set&,int displayid=-1);
			//!<Pickset becomes mine, if it's not in the mgr
			~SubItem();
    void		prepareForShutdown();
    bool		init();
    virtual const char*	parentType() const		=0;

    virtual void	clickCB(CallBacker*)		{}
    virtual void	mouseMoveCB(CallBacker*)		{}
    virtual void	rightclickCB(CallBacker*)	{}

    virtual void	createMenuCB(CallBacker*);
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


class TextSubItem : public SubItem
{
public:
    			TextSubItem(Pick::Set& pck,int displayid=-1);
    bool		init();
    static const char*	sKeyManager() 	{ return "TextAnnotations"; }

protected:
			~TextSubItem()			{}
    virtual const char*	parentType() const;
    virtual void	pickAddedCB(CallBacker*);
    virtual void	rightclickCB(CallBacker*);
    const char*		managerName() const	{ return sKeyManager(); }

//  virtual void	createMenuCB(CallBacker*)	{}
//  virtual void	handleMenuCB(CallBacker*)	{}

    bool		editText(BufferString&);
    BufferString	prevtxt_;

    bool		hasScale() const		{ return true; }
    void		setScale(float);
};


class SymbolSubItem : public SubItem
{
public:

    			SymbolSubItem(Pick::Set& pck,int displayid=-1);
    bool		init();

    static const char*	sKeyManager() 	{ return "ArrowAnnotations"; }

protected:
			~SymbolSubItem()		{}
    virtual const char*	parentType() const;

    virtual void	clickCB(CallBacker*);
    virtual void	mouseMoveCB(CallBacker*);
    virtual void	createMenuCB(CallBacker*);
    virtual void	handleMenuCB(CallBacker*);
    void		propertyChange(CallBacker*);

    MenuItem		propmnuitem_;
    int			arrowtype_;

    bool		hasScale() const	{ return false; }
    const char*		managerName() const	{ return sKeyManager(); }

};

/*
class ImageSubItem : public SubItem
{
public:
    			ImageSubItem(const char* nm,int displayid=-1)
			    : SubItem(nm,displayid)
			    , filemnuitem_("Select image ...")
			    { defscale_ = 100; }

protected:
			~ImageSubItem()			{}
    virtual const char*	parentType() const;

    virtual void	clickCB(CallBacker*);
    virtual void	createMenuCB(CallBacker*);
    virtual void	handleMenuCB(CallBacker*);

    bool		hasScale() const		{ return true; }
    void		setScale(float);

    bool		selectFileName(BufferString&);

    MenuItem		filemnuitem_;
    BufferString	imgfilenm_;
};

*/


#define mDefineParentItem(type,typestr) \
class type##ParentItem : public AnnotTreeItem \
{ \
public: \
		type##ParentItem() \
    		    : AnnotTreeItem(typestr) {} \
protected: \
    uiTreeItem*	createSubItem(int di,Pick::Set& pck) \
    		{ return new type##SubItem(pck,di); } \
    const char*	managerName() const { return type##SubItem::sKeyManager(); } \
};

mDefineParentItem(Text,"Text")
mDefineParentItem(Symbol,"Arrows")
// mDefineParentItem(Image,"Image")


}; // namespace Annotations

#endif
