#ifndef uitreeitemmanager_h
#define uitreeitemmanager_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
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
and data to treeitems. Usage is normally to create the uiTreeTopItem, run the
its init() and add Childrens.
*/

class uiTreeViewItem;
class uiTreeView;
class uiParent;

mClass(uiTools) uiTreeItem	: public CallBacker
{
public:
    				uiTreeItem(const char* nm);
    virtual			~uiTreeItem();
    virtual void		prepareForShutdown();
    				/*!<Override if you want to popup dlg
				    for saving various things (or similar) */
    				
    virtual bool		askContinueAndSaveIfNeeded(bool withcancel);
    const char*			name() const;

    virtual int			selectionKey() const { return -1; }
    virtual bool		select();
    				/*!<Selects this item */
    virtual bool		isSelected() const;
    void			setChecked(bool yn,bool trigger=false);
    bool			isChecked() const;
    NotifierAccess*		checkStatusChange();
    void			expand();
    void			collapse();

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
				 
    virtual bool		addChild(uiTreeItem* child,bool below);
    				/*!<Adds a child. If the child does not fit
				    (i.e. the child's parentType() is not
				    the same as this), it will try to find
				    a valid parent somewhere else in the tree.
				    \param below specifies wether the child
				    shoule be added above or below eventual
				    existing siblings.
				    \note child becomes mine regardless of
				    	  return value.  */
    virtual void		removeChild( uiTreeItem* );
    virtual const uiTreeItem*	findChild( const char* name ) const;
    				/*!<Finds a child in the tree below
				    this item.  */
    virtual const uiTreeItem*	findChild( int selkey ) const;
    				/*!<Finds a child in the tree below
				    this item.  */
    virtual uiTreeItem*		findChild( const char* name );
    				/*!<Finds a child in the tree below
				    this item.  */
    virtual uiTreeItem*		findChild( int selkey );
    				/*!<Finds a child in the tree below
				    this item.  */
    virtual void		findChildren(const char*,
	    				     ObjectSet<uiTreeItem>&);
    				/*!<Finds all children in the tree below this
				    item. */

    template<class T> inline void setProperty(const char* key, const T&);
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
				    shoule be added above or below eventual
				    existing siblings.
				*/

    virtual const char*		parentType() const = 0;
    				/*!<\returns typeid(parentclass).name() */
    virtual bool		init() { return true; }

    virtual bool		rightClick(uiTreeViewItem* item);
    virtual bool		anyButtonClick(uiTreeViewItem* item);
    virtual void		setTreeViewItem( uiTreeViewItem* );
    uiTreeViewItem*		getItem()	{ return uitreeviewitem_; }
    const uiTreeViewItem*	getItem() const { return uitreeviewitem_; }

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
    BufferString		name_;

    uiTreeViewItem*		uitreeviewitem_;
    ObjectSet<uiTreeItem>	children_;
    friend			class uiTreeTopItem;
    friend			class uiODTreeTop;

private:

    bool			addChildImpl(CallBacker*,uiTreeItem*,bool,bool);

};

mClass(uiTools) uiTreeItemRemover : public SequentialTask
{
public:
    uiTreeItemRemover(uiTreeItem* parent,uiTreeItem* child);
    int nextStep();
			            
protected:
    uiTreeItem* parent_;
    uiTreeItem* child_;
};



mClass(uiTools) uiTreeTopItem : public uiTreeItem
{
public:
                        uiTreeTopItem(uiTreeView*, bool=false );
    virtual bool	addChild(uiTreeItem*,bool below);
    virtual void	updateSelection(int selectionkey, bool=false );
    			/*!< Does only update the display */
    virtual void	updateColumnText(int col);

    void		disabRightClick(bool yn) 	{ disabrightclick_=yn; }
    void		disabAnyClick(bool yn) 		{ disabanyclick_=yn; }

			~uiTreeTopItem();
protected:

    virtual bool	addChld(uiTreeItem*,bool below,bool downwards);

    void		selectionChanged(CallBacker*);
    void		rightClickCB(CallBacker*);
    void		anyButtonClickCB(CallBacker*);
    void		handleSelectionChanged(bool frmbtclk);

    virtual const char*	parentType() const { return 0; } 
    virtual uiParent*	getUiParent() const;

    uiTreeView*		listview_;
    bool		disabrightclick_;
    bool		disabanyclick_;
    bool		disabselcngresp_;

};


mClass(uiTools) uiTreeItemFactory
{
public:
    virtual		~uiTreeItemFactory()		{}
    virtual const char*	name() const			= 0;
    virtual uiTreeItem*	create() const			= 0;
};


mClass(uiTools) uiTreeFactorySet : public CallBacker
{
public:
					uiTreeFactorySet();
					~uiTreeFactorySet();
    void				addFactory(uiTreeItemFactory* ptr,
	    					   int placementindex=-1,
						   int pol2d=1);
					/*!<\param ptr	pointer to new factory.
							Object is managed by me.
					    \param placementindex
					    		Indicates how the
							created treeitems should
							be placed when making
							a new tree.
					*/
    void				remove( const char* );

    int					nrFactories() const;
    const uiTreeItemFactory*		getFactory(int) const;
    int					getPlacementIdx(int) const;
    int					getPol2D(int) const;

    CNotifier<uiTreeFactorySet,int>	addnotifier;
    CNotifier<uiTreeFactorySet,int>	removenotifier;

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


inline bool uiTreeItem::getPropertyPtr( const char* propertykey, void*& res ) const
{
    if ( properties_.getPtr( propertykey, res ))
	return true;

    return parent_ ? parent_->getPropertyPtr( propertykey, res ) : false;
}


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

#endif

