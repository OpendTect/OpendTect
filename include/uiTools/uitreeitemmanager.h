#ifndef uitreeitemmanager_h
#define uitreeitemmanager_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: uitreeitemmanager.h,v 1.9 2004-09-14 09:07:22 nanne Exp $
________________________________________________________________________


-*/

#include "sets.h"
#include "iopar.h"
#include "callback.h"
#include "bufstring.h"

/*!\brief
are helping classes for uiListViews, wich makes it easy to bundle inteligence
and data to treeitems. Usage is normally to create the uiTreeTopItem, run the
its init() and add Childrens.
*/

class uiListViewItem;
class uiListView;
class uiParent;

class uiTreeItem	: public CallBacker
{
public:

    				uiTreeItem( const char* name__ );
    virtual			~uiTreeItem();

    const char*			name() const;

    virtual int			selectionKey() const { return -1; }
    virtual bool		select();
    				/*!<Selects this item */
    virtual void		setChecked(bool yn);
				 
    virtual bool		addChild( uiTreeItem* child );
    				/*!<Adds a child. If the child does not fit
				    (i.e. the child's parentType() is not
				    the same as this), it will try to find
				    a valid parent somewhere else in the tree.
				    \note child becomes mine regardless of
				    	  return value.
				*/
    virtual void		removeChild( uiTreeItem* );
    virtual const uiTreeItem*	findChild( const char* name ) const;
    				/*!<Finds a child in the tree below
				    this item.
				*/
    virtual const uiTreeItem*	findChild( int selkey ) const;
    				/*!<Finds a child in the tree below
				    this item.
				*/

    template<class T> inline void setProperty(const char* key, const T&);
    				/*!<Sets a keyed value that has been retrieved
				    with getProperty().
				    \note		Should not be used if T
				    			is a pointer. Use
							setPropertyPtr(
							const char*, T& )
							instead.
				*/
    template<class T> inline void setPropertyPtr(const char* key, const T&);
    				/*!<Sets a keyed pointer that has been retrieved
				    with getPropertyPtr().
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
    template<class T> inline bool getPropertyPtr(const char* key, T& res) const;
    				/*!<Gets a keyed pointer that has been stored
				    with setPropertyPtr().
				    \retval true	the key was found and
				    			res is set
				    \retval false	the key was not found
				    			and res is not set
				*/

protected:
    virtual int			uiListViewItemType() const;
    				/*!<\returns the uiListViewItem::Type that
				    should be created */
    virtual uiParent*		getUiParent();

    virtual bool		addChild( uiTreeItem*, bool downwards );
    				/*!< Adds a child to this item. If the child
				    does not fit (i.e. its parentType() is not
				    equal to this), the object tries to add
				    it to its parent if downwards is false.
				    If downwards is true, it tries to add it
				    to its children if it does not fit.
				*/

    virtual const char*		parentType() const = 0;
    				/*!<\returns typeid(parentclass).name() */
    virtual bool		init() { return true; }
				
    virtual bool		rightClick(uiListViewItem* item);
    virtual bool		anyButtonClick(uiListViewItem* item);
    virtual void		setListViewItem( uiListViewItem* );
    uiListViewItem*		getItem() { return uilistviewitem; }

    virtual bool		showSubMenu() { return true; }
    virtual bool		select(int selkey);

    virtual bool		isSelectable() const { return false; }
    virtual bool		isExpandable() const { return true; }
    
    virtual void		updateSelection(int selectionKey,
	    					bool dw=false );
    				/*!< Does only update the display */
    virtual void		updateColumnText(int col);


    IOPar			properties;

    uiTreeItem*			parent;
    BufferString		name_;

    uiListViewItem*		uilistviewitem;
    ObjectSet<uiTreeItem>	children;
    friend			class uiTreeTopItem;
};


class uiTreeTopItem : public uiTreeItem
{
public:
    			uiTreeTopItem(uiListView*);
    virtual bool	addChild( uiTreeItem*);
    virtual void	updateSelection(int selectionkey, bool=false );
    			/*!< Does only update the display */
    virtual void	updateColumnText(int col);

    void		disabRightClick(bool yn) 	{ disabrightclick=yn; }
    void		disabAnyClick(bool yn) 		{ disabanyclick=yn; }


			~uiTreeTopItem();
protected:
    virtual bool	addChild( uiTreeItem*, bool downwards);
    uiListView*		listview;
    void		rightClickCB(CallBacker*);
    void		anyButtonClickCB(CallBacker*);
    virtual const char*	parentType() const { return 0; } 
    virtual uiParent*	getUiParent();

    bool		disabrightclick;
    bool		disabanyclick;
};


class uiTreeItemCreater
{
public:
    virtual		~uiTreeItemCreater()		{}
    virtual const char*	name() const			= 0;
    virtual uiTreeItem*	create() const			= 0;
};


class uiTreeCreaterSet : public CallBacker
{
public:
					uiTreeCreaterSet();
					~uiTreeCreaterSet();
    void				addCreater(uiTreeItemCreater* ptr,
	    					   int placementindex=-1 );
					/*!<\param ptr	pointer to new creater.
							Object is managed by me.
					    \param placementindex
					    		Indicates how the
							created treeitems should
							be placed when making
							a new tree.
					*/
    void				remove( const char* );

    int					nrCreaters() const;
    const uiTreeItemCreater*		getCreater( int idx ) const;
    int					getPlacementIdx(int idx) const;

    CNotifier<uiTreeCreaterSet,int>	addnotifier;
    CNotifier<uiTreeCreaterSet,int>	removenotifier;

protected:

    ObjectSet<uiTreeItemCreater>	creaters;
    TypeSet<int>			placementidxs;

};


template<class T>
bool inline uiTreeItem::getProperty( const char* propertykey, T& res ) const
{
    if ( properties.get( propertykey, res ))
	return true;

    return parent ? parent->getProperty( propertykey, res ) : false;
}


template<class T>
bool inline uiTreeItem::getPropertyPtr( const char* propertykey, T& res ) const
{
    int tres = 0;
    if ( properties.get( propertykey, tres ))
    {
	res = (T) tres;
	return true;
    }

    return parent ? parent->getPropertyPtr( propertykey, res ) : false;
}


template<class T>
void inline uiTreeItem::setProperty( const char* propertykey, const T& val )
{
    if ( typeid(T)==typeid(void*) )
	properties.set( propertykey, (long long) val );
    else
	properties.set( propertykey, val );
}


template<class T>
void inline uiTreeItem::setPropertyPtr( const char* propertykey, const T& val )
{
    properties.set( propertykey, (int) val );
}

#endif
