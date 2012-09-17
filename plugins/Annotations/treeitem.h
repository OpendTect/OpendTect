#ifndef treeitem_h
#define treeitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2005
 RCS:           $Id: treeitem.h,v 1.19 2009/07/22 16:01:25 cvsbert Exp $
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
    virtual bool	rightClick(uiListViewItem*);
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
    typedef uiODTreeItem  inheritedClass;
public:
    				AnnotTreeItem(const char*);
    				~AnnotTreeItem();

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



   void			setAddedCB(CallBacker*);
   void			setRemovedCB(CallBacker*);

   BufferString		typestr_;
};



class SubItem : public uiODDisplayTreeItem
{
public:
    static bool		doesNameExist(const char*);
    static char		createIOEntry(const char* nm,bool overwrite,
	    			    MultiID&,const char* mannm);
    			/*!<\retval -1 error
			    \retval 0 name exists and overwrite is not set.
			    \retval 1 success.
			*/

protected:
    			SubItem(Pick::Set&,int displayid=-1);
			//!<Pickset becomes mine, if it's not in the mgr
    virtual		~SubItem();
    void		prepareForShutdown();
    void		removeStuff();
    bool		init();
    virtual const char*	parentType() const		=0;
    virtual void	fillStoragePar(IOPar&) const	{}

    virtual void	clickCB(CallBacker*)		{}
    virtual void	mouseMoveCB(CallBacker*)	{}
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
			~TextSubItem()	{ removeStuff(); }

    virtual const char*	parentType() const;
    virtual void	pickAddedCB(CallBacker*);
    const char*		managerName() const	{ return sKeyManager(); }

    void		createMenuCB(CallBacker*);
    void		handleMenuCB(CallBacker*);

    bool		editText(BufferString& str, BufferString& url,
	    			 bool& enab);
    void		fillStoragePar(IOPar&) const;
    BufferString	prevtxt_;
    Color		boxcolor_;

    static const char*	sKeyBoxColor()		{ return "Box Color"; }
    
    bool		hasScale() const		{ return true; }
    void		setScale(float);
    
    MenuItem		changetextmnuitem_;
    MenuItem		changecolormnuitem_;
};


class ArrowSubItem : public SubItem
{
public:

    			ArrowSubItem(Pick::Set& pck,int displayid=-1);
    bool		init();

    static const char*	sKeyManager() 	{ return "ArrowAnnotations"; }

protected:
			~ArrowSubItem()	{ removeStuff(); }
    virtual const char*	parentType() const;

    void		fillStoragePar(IOPar&) const;
    void		createMenuCB(CallBacker*);
    void		handleMenuCB(CallBacker*);
    void		propertyChange(CallBacker*);

    MenuItem		propmnuitem_;
    int			arrowtype_;

    bool		hasScale() const	{ return false; }
    const char*		managerName() const	{ return sKeyManager(); }

    static const char*		sKeyArrowType()	{ return "Arrow Type"; }
    static const char*		sKeyLineWidth()	{ return "Line width"; }

};

class ImageSubItem : public SubItem
{
public:
    			ImageSubItem(Pick::Set&,int displayid=-1);
    bool		init();
    static const char*	sKeyManager() 	{ return "ImageAnnotations"; }

protected:
			~ImageSubItem()	{ removeStuff(); }
    const char*		parentType() const;
    void		fillStoragePar(IOPar&) const;

    void		createMenuCB(CallBacker*);
    void		handleMenuCB(CallBacker*);

    void		retrieveFileName(CallBacker*);

    void		updateColumnText(int col);

    bool		hasScale() const		{ return true; }
    const char*		managerName() const		{ return sKeyManager();}

    void		selectFileName() const;

    MenuItem		filemnuitem_;
};



#define mDefineParentItem(type,typestr,defsz) \
class type##ParentItem : public AnnotTreeItem \
{ \
public: \
		type##ParentItem() \
    		    : AnnotTreeItem(typestr) {} \
protected: \
    uiTreeItem*	createSubItem(int di,Pick::Set& pck) \
    		{ return new type##SubItem(pck,di); } \
    const char*	managerName() const { return type##SubItem::sKeyManager(); } \
    const char*	oldSelKey() const { return typestr; } \
    int		defScale() const 	{ return defsz; } \
}

mDefineParentItem(Text,"Text",25);
mDefineParentItem(Arrow,"Arrows",1000);
mDefineParentItem(Image,"Image",1000);


}; // namespace Annotations

#endif
