#ifndef uilistview_h
#define uilistview_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          29/01/2002
 RCS:           $Id: uitreeview.h,v 1.51 2012-08-30 05:49:33 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uiobj.h"
#include "bufstringset.h"
#include "keyenum.h"
#include "draw.h"


mFDQtclass(QTreeWidget)
mFDQtclass(QTreeWidgetItem)
class uiListViewBody;
class uiListViewItem;
class ioPixmap;

mClass(uiBase) uiListView : public uiObject
{
public:
			uiListView(uiParent* parnt,
				   const char* nm="uiListView",
				   int preferredNrLines=0,
				   bool rootdecorated=true);

    virtual		~uiListView();

			// 0: use nr itms in list
    uiListViewBody& 	mkbody(uiParent*,const char*,int);
    void		setNrLines(int);

    enum		ScrollMode { Auto, AlwaysOff, AlwaysOn };
    void		setHScrollBarMode(ScrollMode);
    void		setVScrollBarMode(ScrollMode);

    void		setTreeStepSize(int);

    bool		rootDecorated() const;
    void		setRootDecorated(bool yn);

    // take & insert are meant to MOVE an item to another point in the tree 
    void		takeItem(uiListViewItem*);
    void		insertItem(int,uiListViewItem*);

    void		addColumns(const BufferStringSet&);
    int			nrColumns() const;

    void		removeColumn(int index );
    void		setColumnText(int column,const char* label);
    const char*		columnText(int column) const;
    void		setColumnWidth(int column,int width);
    void		setFixedColumnWidth(int column,int width);
    int			columnWidth(int column) const;

    enum		WidthMode { Manual, Fixed, Stretch, ResizeToContents,
				    Custom };
    void		setColumnWidthMode(int column,WidthMode);
    WidthMode		columnWidthMode(int column) const;

    void		setColumnAlignment(int,Alignment::HPos);
    Alignment::HPos	columnAlignment(int) const;

    void		ensureItemVisible(const uiListViewItem*);

    enum		SelectionMode
    			{ NoSelection=0, Single, Multi, Extended, Contiguous };
    enum		SelectionBehavior 
			{ SelectItems, SelectRows, SelectColumns };
    void		setSelectionMode(SelectionMode);
    SelectionMode	selectionMode() const;
    void		setSelectionBehavior(SelectionBehavior);
    SelectionBehavior	selectionBehavior() const;

    void		clearSelection();
    void		setSelected(uiListViewItem*,bool);
    bool		isSelected(const uiListViewItem*) const;
    uiListViewItem*	selectedItem() const;

    void		setCurrentItem(uiListViewItem*,int column=0);

    uiListViewItem*	currentItem() const;
    int			currentColumn() const;
    uiListViewItem*	getItem(int) const; 
    uiListViewItem*	firstItem() const;
    uiListViewItem*	lastItem() const;

    int			nrItems() const;

    void		setItemMargin(int);
    int			itemMargin() const;

    void		setShowToolTips(bool);
    bool		showToolTips() const;

    int			indexOfItem(uiListViewItem*) const;
    uiListViewItem*	findItem(const char*,int,bool) const;
    uiParent*		parent()		{ return parent_; }

    void		clear();
    void		invertSelection();
    void		selectAll();
    void		expandAll();
    void		collapseAll();
    void		expandTo(int treedepth);

    void		translate();
    bool		handleLongTabletPress();

			//! re-draws at next X-loop
    void		triggerUpdate();

			//! item last notified. See notifiers below
    uiListViewItem*	itemNotified()		{ return lastitemnotified_; }
    int			columnNotified()	{ return column_; }
    void		unNotify()		{ lastitemnotified_ = 0; }

    void		setNotifiedItem(mQtclass(QTreeWidgetItem*));
    void		setNotifiedColumn(int col)	{ column_ = col; }

    Notifier<uiListView> selectionChanged;
    Notifier<uiListView> currentChanged;
    Notifier<uiListView> itemChanged;
    Notifier<uiListView> returnPressed;
    Notifier<uiListView> rightButtonClicked;
    Notifier<uiListView> rightButtonPressed;
    Notifier<uiListView> leftButtonClicked;
    Notifier<uiListView> leftButtonPressed;
    Notifier<uiListView> mouseButtonPressed;
    Notifier<uiListView> mouseButtonClicked;
    Notifier<uiListView> contextMenuRequested;
    Notifier<uiListView> doubleClicked;
    Notifier<uiListView> itemRenamed;
    Notifier<uiListView> expanded;
    Notifier<uiListView> collapsed;
    Notifier<uiListView> unusedKey;

protected:

    mutable BufferString rettxt;
    uiListViewItem*	lastitemnotified_;
    uiParent*		parent_;
    int			column_;
    OD::ButtonState     buttonstate_;

    void 		cursorSelectionChanged(CallBacker*);
    void 		itemChangedCB(CallBacker*);
    void		updateCheckStatus(uiListViewItem*);

    uiListViewBody*		lvbody()	{ return body_; }
    const uiListViewBody*	lvbody() const	{ return body_; }

private:

    friend class		i_listVwMessenger;
    friend class		uiListViewBody;
    friend class		uiListViewItem;

    uiListViewBody*	body_;

};


/*!

*/

mClass(uiBase) uiListViewItem : public CallBacker
{
public:

    enum			Type { Standard, CheckBox };

    mClass(uiBase) Setup
    {
    public:
				Setup( const char* txt=0, 
				       uiListViewItem::Type tp=
						uiListViewItem::Standard,
				       bool setchecked=true )
				: type_(tp)
				, after_(0)
				, pixmap_(0)
				, setcheck_(setchecked)
				{ label( txt ); }

	mDefSetupMemb(uiListViewItem::Type,type)
	mDefSetupMemb(uiListViewItem*,after)
	mDefSetupMemb(const ioPixmap*,pixmap)
	mDefSetupMemb(bool,setcheck)
	BufferStringSet		labels_;

	Setup& 			label( const char* txt ) 
				{ 
				    if( txt ) 
					labels_ += new BufferString( txt ); 
				    return *this; 
				}
    };

			uiListViewItem(uiListViewItem* parent,const Setup&); 
			uiListViewItem(uiListView* parent,const Setup&);
			~uiListViewItem();

    mQtclass(QTreeWidgetItem*)	qItem()			{ return qtreeitem_; }
    const mQtclass(QTreeWidgetItem*) qItem() const	{ return qtreeitem_; }

    int			nrChildren() const;

    void		setBGColor(int column,const Color&);

    void		setCheckable(bool);
    bool		isCheckable() const;
    void		setChecked(bool,bool trigger=false);
    			//!< does nothing if not checkable
    bool		isChecked(bool qtstatus=true) const;
			//!< returns false if not checkable

    void		setToolTip(int column,const char*);
    void		translate(int column);

    void		insertItem(int,uiListViewItem*);
    void		takeItem(uiListViewItem*);
    void		removeItem(uiListViewItem*);
    void		moveItem(uiListViewItem* after);
    int			siblingIndex() const;
    			/*!<\returns this items index of it's siblings. */

    void		setText( const char*, int column=0 );
    void		setText( int i, int column=0 )
			{ setText( toString(i), column ); }
    void		setText( float f, int column=0 )
			{ setText( toString(f), column ); }
    void		setText( double d, int column=0 )
			{ setText( toString(d), column ); }

    const char*		text( int column=0 ) const;

    void		setPixmap(int column,const ioPixmap&);

    virtual const char* key(int,bool) const		{ return 0; }
    virtual int		compare( uiListViewItem*,int column,bool) const
							{ return mUdf(int); }

    void		setOpen(bool yn=true);
    bool		isOpen() const;

    void		setSelected(bool yn);
    bool		isSelected() const;

    uiListViewItem*	getChild(int) const;
    uiListViewItem*	firstChild() const;
    uiListViewItem*	lastChild() const;
    uiListViewItem*	nextSibling() const;
    uiListViewItem*	prevSibling() const;
    uiListViewItem*	parent() const;

    uiListViewItem*	itemAbove();
    uiListViewItem*	itemBelow();

    uiListView*		listView() const;

    void		setSelectable(bool yn);
    bool		isSelectable() const;

    void		setDragEnabled(bool);
    void		setDropEnabled(bool);
    bool		dragEnabled() const;
    bool		dropEnabled() const;

    void		setVisible(bool yn);
    bool		isVisible() const;

    void		setRenameEnabled(int column,bool);
    bool		renameEnabled(int column) const;

    void		setEnabled(bool);
    bool		isEnabled() const;


    Notifier<uiListViewItem> stateChanged; //!< only works for CheckBox type
    Notifier<uiListViewItem> keyPressed;
    			//!< passes CBCapsule<const char*>* cb
    			//!< If you handle it, set cb->data = 0;

    static mQtclass(QTreeWidgetItem*)	 qitemFor(uiListViewItem*);
    static const mQtclass(QTreeWidgetItem*)  qitemFor(const uiListViewItem*);

    static uiListViewItem* 	 itemFor(mQtclass(QTreeWidgetItem*));
    static const uiListViewItem* itemFor(const mQtclass(QTreeWidgetItem*));

protected:

    mQtclass(QTreeWidgetItem*)		qtreeitem_;
    mutable BufferString	rettxt;

    void			init(const Setup&);

    bool			isselectable_;
    bool			iseditable_;
    bool			isdragenabled_;
    bool			isdropenabled_;
    bool			ischeckable_;
    bool			isenabled_;
    void			updateFlags();
    bool			checked_;

    void			trlReady(CallBacker*);
    int				translateid_;
};

#endif

