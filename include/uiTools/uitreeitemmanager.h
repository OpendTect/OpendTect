#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "sets.h"
#include "iopar.h"
#include "callback.h"
#include "bufstring.h"
#include "task.h"
#include "thread.h"

/*!\brief
are helping classes for uiTreeViews, wich makes it easy to bundle inteligence
and data to treeitems. Usage is normally to create the uiTreeTopItem, run
its init() and add Childrens.
*/

class uiTreeViewItem;
class uiTreeView;
class uiParent;

mExpClass(uiTools) uiTreeItem : public CallBacker
{
public:
				uiTreeItem(const uiString& nm=
					   uiString::emptyString() );
    virtual			~uiTreeItem();
				mOD_DisableCopy(uiTreeItem)

    virtual void		prepareForShutdown();
				/*!<Override if you want to popup dlg
				    for saving various things (or similar) */

    virtual bool		askContinueAndSaveIfNeeded(bool withcancel);
    void			setName( const uiString& nm )	{ name_ = nm; }
    uiString			name() const;

    bool			areAllParentsChecked();

    void			entryInEditMode(int col);

    virtual int			selectionKey() const { return -1; }
    virtual bool		select();
				/*!<Selects this item */
    virtual bool		isSelected() const;
    void			setChecked(bool yn,bool trigger=false);
    virtual bool		isChecked() const;
    NotifierAccess*		checkStatusChange();
    NotifierAccess*		keyPressed();
    void			expand();
    bool			isExpanded() const;
    void			collapse();
    bool			isCollapsed() const;

    bool			hasChildren() const;
    bool			hasGrandChildren() const;
    bool			allChildrenExpanded() const;
    bool			allChildrenCollapsed() const;
    bool			allChildrenChecked() const;
    bool			allChildrenUnchecked() const;

    virtual int			siblingIndex() const;
				/*\returns the index of this item among
				   its siblings.
				  \note this index is not neseccarely the same
				        as the item's index in the parent's
					child-list. */

    virtual uiTreeItem*		siblingAbove();
    virtual uiTreeItem*		siblingBelow();
    virtual void		moveItem(uiTreeItem* after);
    virtual void		moveItemToTop();
    uiTreeItem*			lastChild();

    int				nrChildren() const { return children_.size(); }
    const uiTreeItem*		getChild(int) const;
    uiTreeItem*			getChild(int);
    ObjectSet<uiTreeItem>&	getChildren()	{ return children_; }
    const ObjectSet<uiTreeItem>& getChildren() const { return children_; }

    virtual bool		addChild(uiTreeItem* child,bool below);
				/*!<Adds a child. If the child does not fit
				    (i.e. the child's parentType() is not
				    the same as this), it will try to find
				    a valid parent somewhere else in the tree.
				    \param child becomes mine
				    \param below specifies wether the child
				    shoule be added above or below eventual
				    existing siblings.
				*/
    virtual void		removeChild(uiTreeItem*);
    virtual void		removeAllChildren();

    virtual const uiTreeItem*	findChild(const char* name) const;
				/*!<Finds a child in the tree below
				    this item.  */
    virtual const uiTreeItem*	findChild(int selkey) const;
				/*!<Finds a child in the tree below
				    this item.  */
    virtual uiTreeItem*		findChild(const char* name);
				/*!<Finds a child in the tree below
				    this item.  */
    virtual uiTreeItem*		findChild(int selkey);
				/*!<Finds a child in the tree below
				    this item.  */
    virtual void		findChildren(const char*,
					     ObjectSet<uiTreeItem>&);
				/*!<Finds all children in the tree below this
				    item. */

    template<class T> inline void setProperty(const char* key,const T&);
				/*!<Sets a keyed value that has been retrieved
				    with getProperty().
				    \note		Should not be used if T
							is a pointer. Use
							setPropertyPtr(
							const char*, T& )
							instead.
				*/
    inline void			setPropertyPtr(const char* key,void*);
				/*!<Sets a keyed pointer that may have been
				    retrieved with getPropertyPtr().
				*/
    template<class T> inline bool getProperty(const char* key, T& res) const;
				/*!<Gets a keyed value that has been stored
				    with setProperty().
				    \retval true	the key was found and
							res is set
				    \retval false	the key was not found
							and res is not set
				    \note		Should not be used if T
							is a pointer. Use
							getPropertyPtr(
							const char*, T& )
							instead.
				*/
    inline bool			getPropertyPtr(const char* key,void*&) const;
				/*!<Gets a keyed pointer that has been stored
				    with setPropertyPtr().
				    \retval true	the key was found and
							res is set
				    \retval false	the key was not found
							and res is not set
				*/

    virtual void		updateColumnText(int col);
    virtual void		updateCheckStatus();

    virtual void		translateText() { updateColumnText( 0 ); }
    uiTreeViewItem*		getItem()	{ return uitreeviewitem_; }
    const uiTreeViewItem*	getItem() const { return uitreeviewitem_; }

protected:

    virtual int			uiTreeViewItemType() const;
				/*!<\returns the uiTreeViewItem::Type that
				    should be created */
    virtual uiParent*		getUiParent() const;

    virtual bool		addChld(uiTreeItem*,bool below,bool downwards);
				/*!< Adds a child to this item. If the child
				    does not fit (i.e. its parentType() is not
				    equal to this), the object tries to add
				    it to its parent if downwards is false.
				    If downwards is true, it tries to add it
				    to its children if it does not fit.
				    \param below specifies wether the child
				    should be added above or below eventual
				    existing siblings.
				    \param downwards
				*/

    virtual const char*		parentType() const = 0;
				/*!<\returns typeid(parentclass).name() */
    virtual bool		useParentType() const	    { return true; }
    virtual bool		init() { return true; }

    virtual bool		rightClick(uiTreeViewItem*);
    virtual bool		anyButtonClick(uiTreeViewItem*);
    virtual bool		doubleClick(uiTreeViewItem*);
    virtual void		setTreeViewItem(uiTreeViewItem*);
    virtual void		removeItem(uiTreeViewItem*);
    virtual void		renameItem(uiTreeViewItem*);

    virtual bool		showSubMenu() { return true; }
    virtual bool		selectWithKey(int selkey);

    virtual bool		isSelectable() const { return false; }
    virtual bool		isExpandable() const { return true; }

    virtual void		updateSelection(int selectionKey,
						bool dw=false );
				/*!< Does only update the display */
    virtual bool		shouldSelect(int selectionkey) const;
				/*!\returns true if the item should be marked
				    as selected given the selectionkey. */

    IOPar			properties_;

    uiTreeItem*			parent_;
    uiString			name_;

    uiTreeViewItem*		uitreeviewitem_;
    ObjectSet<uiTreeItem>	children_;
    friend			class uiTreeTopItem;
    friend			class uiODTreeTop;

private:

    bool			addChildImpl(CallBacker*,uiTreeItem*,bool,bool);

public:
    void			updateSelTreeColumnText(int col);
    uiTreeItem*			parentTreeItem()	{ return parent_; }
};

mExpClass(uiTools) uiTreeItemRemover : public SequentialTask
{
public:
			uiTreeItemRemover(uiTreeItem* parent,uiTreeItem* child);
			~uiTreeItemRemover();

    int			nextStep() override;

protected:
    uiTreeItem*		parent_;
    uiTreeItem*		child_;
};



mExpClass(uiTools) uiTreeTopItem : public uiTreeItem
{
public:
			uiTreeTopItem(uiTreeView*, bool=false );
			~uiTreeTopItem();

    bool		addChild(uiTreeItem*,bool below) override;
    void		updateSelection(int selectionkey, bool=false ) override;
			/*!< Does only update the display */
    void		updateColumnText(int col) override;
    bool		isChecked() const override		{ return true; }

    void		disabRightClick(bool yn)	{ disabrightclick_=yn; }
    void		disabAnyClick(bool yn)		{ disabanyclick_=yn; }
    uiTreeView*		getTreeView() const		{ return listview_; }

protected:

    bool		addChld(uiTreeItem*,bool below,bool downwards) override;
    void		removeItem(uiTreeViewItem*) override;

    void		selectionChanged(CallBacker*);
    void		rightClickCB(CallBacker*);
    void		anyButtonClickCB(CallBacker*);
    void		doubleClickCB(CallBacker*);
    void		itemRenamed(CallBacker*);
    void		handleSelectionChanged(bool frmbtclk);

    const char*		parentType() const override { return 0; }
    uiParent*		getUiParent() const override;

    uiTreeView*		listview_;
    bool		disabrightclick_;
    bool		disabanyclick_;
    bool		disabselcngresp_;

};


mExpClass(uiTools) uiTreeItemFactory
{
public:
    virtual		~uiTreeItemFactory();
    virtual const char*	name() const			= 0;
    virtual uiTreeItem*	create() const			= 0;

protected:
			uiTreeItemFactory();
};


mExpClass(uiTools) uiTreeFactorySet : public CallBacker
{
public:
				uiTreeFactorySet();
				~uiTreeFactorySet();

    void			addFactory(uiTreeItemFactory* ptr,
					int placementindex=-1,
					OD::Pol2D3D pol2d=OD::Both2DAnd3D);
					/*!<\param ptr	pointer to new factory.
							Object is managed by me.
					    \param placementindex
							Indicates how the
							created treeitems should
							be placed when making
							a new tree.
					   \param pol2d
					*/
    void			remove( const char* );

    int				nrFactories() const;
    const uiTreeItemFactory*	getFactory(int) const;
    int				getPlacementIdx(int) const;
    OD::Pol2D3D			getPol2D3D(int) const;

    CNotifier<uiTreeFactorySet,int>	addnotifier;
    CNotifier<uiTreeFactorySet,int>	removenotifier;

    mDeprecated("Use getPol2D3D")
    OD::Pol2D3D			getPol2D( int idx ) const
				{ return getPol2D3D(idx); }

protected:

    ObjectSet<uiTreeItemFactory>	factories_;
    TypeSet<int>			placementidxs_;
    TypeSet<int>			pol2ds_;

};


template<class T>
bool inline uiTreeItem::getProperty( const char* propertykey, T& res ) const
{
    if ( properties_.get( propertykey, res ))
	return true;

    return parent_ ? parent_->getProperty( propertykey, res ) : false;
}


inline bool uiTreeItem::getPropertyPtr(const char* propertykey,void*& res) const
{
    if ( properties_.getPtr( propertykey, res ))
	return true;

    return parent_ ? parent_->getPropertyPtr( propertykey, res ) : false;
}


#include <typeinfo>

template<class T>
void inline uiTreeItem::setProperty( const char* propertykey, const T& val )
{
    if ( typeid(T)==typeid(void*) )
	properties_.set( propertykey, (od_int64)val );
    else
	properties_.set( propertykey, val );
}


void inline uiTreeItem::setPropertyPtr( const char* propertykey, void* val )
{
    properties_.setPtr( propertykey, val );
}
