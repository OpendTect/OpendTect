#ifndef uilistview_h
#define uilistview_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          29/01/2002
 RCS:           $Id: uitreeview.h,v 1.9 2004-07-14 15:48:05 nanne Exp $
________________________________________________________________________

-*/

#include "uiobj.h"
#include "bufstringset.h"
class uiListViewBody;
class uiListViewItem;
class QListViewItem;
class ioPixmap;


class uiListView : public uiObject
{
    friend class		i_listVwMessenger;
    friend class		uiListViewBody;
    friend class		uiListViewItemBody;

public:

			uiListView( uiParent* parnt,
				    const char* nm = "uiListView",
				    int preferredNrLines=0,
				    bool rootdecorated = true );

    virtual		~uiListView()			{}

			// 0: use nr itms in list
    void		setLines( int prefNrLines );

    int			treeStepSize() const;
    void		setTreeStepSize( int );

    bool		rootDecorated() const;
    void		setRootDecorated( bool yn );

    // take & insert are meant to MOVE an item to another point in the tree 
    void		takeItem( uiListViewItem* );
    void		insertItem( uiListViewItem* );

			// returns index of new column
    int			addColumn( const char* label, int size = -1);

    void		removeColumn( int index );
    void		setColumnText( int column, const char* label );

    const char*		columnText( int column ) const;
    void		setColumnWidth( int column, int width );
    int			columnWidth( int column ) const;

    enum		WidthMode { Manual, Maximum };
    void		setColumnWidthMode( int column, WidthMode );
    WidthMode		columnWidthMode( int column ) const;
    int			columns() const;

    void		setColumnAlignment( int, int );
    int			columnAlignment( int ) const;

    void		ensureItemVisible( const uiListViewItem*  );

    void		setMultiSelection( bool yn );
    bool		isMultiSelection() const;

    enum		SelectionMode { Single, Multi, Extended, NoSelection };
    void		setSelectionMode( SelectionMode mode );
    SelectionMode	selectionMode() const;

    void		clearSelection();
    void		setSelected( uiListViewItem*, bool );
    bool		isSelected( const uiListViewItem* ) const;
    uiListViewItem*	selectedItem() const;

			//! shows or hides an item's children
    void		setOpen( uiListViewItem* , bool );
    bool		isOpen( const uiListViewItem* ) const;

    void		setCurrentItem( uiListViewItem* );

    uiListViewItem*	currentItem() const;
    uiListViewItem*	firstChild() const;
    uiListViewItem*	lastItem() const;

    int			childCount() const;

    void		setItemMargin( int );
    int			itemMargin() const;

    void		setSorting( int column, bool increasing = true );
    void		sort();

    void		setShowSortIndicator( bool yn );
    bool		showSortIndicator() const;
    void		setShowToolTips( bool yn );
    bool		showToolTips() const;

    uiListViewItem*	findItem( const char* text, int column ) const;

    void		clear();
    void		invertSelection();
    void		selectAll( bool yn );

			//! re-draws at next X-loop
    void		triggerUpdate();

			//! item last notified. See notifiers below
    uiListViewItem*	itemNotified()		{ return lastitemnotified; }
    void		unNotify()		{ lastitemnotified = 0; }

    Notifier<uiListView> selectionChanged;
    Notifier<uiListView> currentChanged;
    Notifier<uiListView> clicked;
    Notifier<uiListView> pressed;
    Notifier<uiListView> doubleClicked;
    Notifier<uiListView> returnPressed;
    Notifier<uiListView> spacePressed;
    Notifier<uiListView> rightButtonClicked;
    Notifier<uiListView> rightButtonPressed;
    Notifier<uiListView> mouseButtonPressed;
    Notifier<uiListView> mouseButtonClicked;
    Notifier<uiListView> contextMenuRequested;
    //! the user moves the mouse cursor onto the item
    Notifier<uiListView> onItem;
    Notifier<uiListView> itemRenamed;
    Notifier<uiListView> expanded;
    Notifier<uiListView> collapsed;

protected:

    mutable BufferString rettxt;
    uiListViewItem*	lastitemnotified;

    void		setNotifiedItem( QListViewItem* );

    uiListViewBody*	lvbody()	{ return body_; }

private:

    uiListViewBody*	body_;
    uiListViewBody&	mkbody(uiParent*,const char*,int);

};


class uiListViewItemBody;

/*!

*/
class uiListViewItem : public uiHandle<uiListViewItemBody>
{
friend class			uiListViewItemBody;
public:

    enum			Type { Standard, Controller, RadioButton, 
				       CheckBox };

    class Setup
    {
    public:
				Setup( const char* txt=0, 
				       uiListViewItem::Type tp = 
					    uiListViewItem::Standard  )
				: type_(tp), after_(0)
				, pixmap_(0)
				{ label( txt ); }

	Setup&			type( uiListViewItem::Type tp )
				    { type_ = tp;  return *this; }
	Setup& 			after( uiListViewItem* aft ) 
				    { after_ = aft; return *this; }
	Setup& 			label( const char* txt ) 
				{ 
				    if( txt ) 
					labels_ += new BufferString( txt ); 
				    return *this; 
				}
	Setup& 			pixmap( const ioPixmap* pm )
				    { pixmap_ = pm; return *this; }

	uiListViewItem::Type	type_;
	uiListViewItem*		after_;
	BufferStringSet		labels_;
	const ioPixmap*		pixmap_;

    };


			uiListViewItem( uiListViewItem* prnt, const char* txt);
			uiListViewItem( uiListView* prnt, const char* txt);

			uiListViewItem( uiListViewItem* parent, const Setup& ); 
			uiListViewItem( uiListView* parent, const Setup& );


    //Type		type() const		{ return type_; }

    bool		isCheckable() const;
    void		setChecked( bool ); //!< does nothing if not checkable
    bool		isChecked() const;  //!< returns false if not checkable

    virtual		~uiListViewItem() {}

    void		insertItem( uiListViewItem* );
    void		removeItem( uiListViewItem* );

    int			depth() const; // depth in the tree.

    void		setText( const char*, int column=0 );
    void		setText( int i, int column=0 )
			{ setText( toString(i), column ); }
    void		setText( float f, int column=0 )
			{ setText( toString(f), column ); }
    void		setText( double d, int column=0 )
			{ setText( toString(d), column ); }

    const char*		text( int column=0 ) const;

    void		setPixmap( int column, const ioPixmap& );
			//! returns NEW ioPixmap, constructed from Qt's pixmap
    const ioPixmap* 	pixmap( int column ) const; // becomes yours

    virtual const char* key( int, bool ) const		{ return 0; }
    virtual int		compare( uiListViewItem* , int column, bool ) const
			    { return mUndefIntVal; }
    //virtual void sortChildItems( int, bool );

    int			childCount() const;

    bool		isOpen() const;
    void		setOpen( bool yn=true );

    void		setSelected( bool yn=true );
    bool		isSelected() const;

    uiListViewItem*	firstChild() const;
    uiListViewItem*	nextSibling() const;
    uiListViewItem*	parent() const;

    uiListViewItem*	itemAbove();
    uiListViewItem*	itemBelow();

    uiListView*		listView() const;

    void		setSelectable( bool yn=true );
    bool		isSelectable() const;

    void		setExpandable( bool yn=true );
    bool		isExpandable() const;

    void		moveItem( uiListViewItem* after );


    void		setDragEnabled( bool yn=true );
    void		setDropEnabled( bool yn=true );
    bool		dragEnabled() const;
    bool		dropEnabled() const;

    void		setVisible( bool yn=true );
    bool		isVisible() const;

    void		setRenameEnabled( int column, bool yn=true );
    bool		renameEnabled( int column ) const;

    void		setEnabled( bool yn=true );
    bool		isEnabled() const;

    void		setMultiLinesEnabled( bool yn=true );
    bool		multiLinesEnabled() const;

    Notifier<uiListViewItem> stateChanged; //!< only works for CheckBox type

    static QListViewItem*	 qitemFor( uiListViewItem* );
    static const QListViewItem*  qitemFor( const uiListViewItem* );

    static uiListViewItem* 	 itemFor( QListViewItem* );
    static const uiListViewItem* itemFor( const QListViewItem* );

protected:

 //   Type			type_;
    mutable BufferString	rettxt;

    uiListViewItemBody*		itmbody()	{ return body_; }

private:

    uiListViewItemBody*		body_;
    uiListViewItemBody&		mkbody(uiListView*,uiListViewItem*,
								const Setup&);
    void			init(const Setup&);

};

#endif
