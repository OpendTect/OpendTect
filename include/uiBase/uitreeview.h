#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uiobj.h"
#include "bufstringset.h"
#include "keyenum.h"
#include "draw.h"
#include "uistring.h"

mFDQtclass(QTreeWidget)
mFDQtclass(QTreeWidgetItem)
mFDQtclass(QTreeWidgetItemIterator)
mFDQtclass(QString)

class uiPixmap;
class uiTreeViewBody;
class uiTreeViewItem;
namespace ColTab { class Sequence; }

mExpClass(uiBase) uiTreeView : public uiObject
{ mODTextTranslationClass(uiTreeView);
public:
			uiTreeView(uiParent* parnt,
				   const char* nm="uiTreeView",
				   int preferredNrLines=0,
				   bool rootdecorated=true);
    virtual		~uiTreeView();

    bool		isEmpty() const override;
    void		setEmpty();
    void		setNrLines(int);
    void		resizeHeightToContents(int minh=-1,int maxh=-1);

    bool		rootDecorated() const;
    void		setRootDecorated(bool yn);
    void		showHeader(bool yn); // default is true

    void		addColumns(const uiStringSet&);
    void		addColumns(const BufferStringSet&);
    void		setNrColumns(int); // when columns have no name
    int			nrColumns() const;

    void		removeColumn(int index);
    void		setColumnText(int column,const uiString& label);
    uiString		getColumnText(int column) const;
    void		setColumnWidth(int column,int width);
    void		setFixedColumnWidth(int column,int width);
    int			columnWidth(int column) const;

    enum		WidthMode { Manual, Fixed, Stretch, ResizeToContents,
				    Custom };
    void		setColumnWidthMode(WidthMode);
    void		setColumnWidthMode(int column,WidthMode);
    WidthMode		columnWidthMode(int column) const;

    void		setColumnAlignment(Alignment::HPos);
    void		setColumnAlignment(int,Alignment::HPos);
    Alignment::HPos	columnAlignment(int) const;

    enum		ScrollMode { Auto, AlwaysOff, AlwaysOn };
    void		setHScrollBarMode(ScrollMode);
    void		setVScrollBarMode(ScrollMode);

    enum		SelectionMode
			{ NoSelection=0, Single, Multi, Extended, Contiguous };
    enum		SelectionBehavior
			{ SelectItems, SelectRows, SelectColumns };
    void		setSelectionMode(SelectionMode);
    SelectionMode	selectionMode() const;
    void		setSelectionBehavior(SelectionBehavior);
    SelectionBehavior	selectionBehavior() const;

    void		clearSelection();
    void		setSelected(uiTreeViewItem*,bool);
    bool		isSelected(const uiTreeViewItem*) const;
    uiTreeViewItem*	selectedItem() const;
    int			nrSelected() const;
    void		getSelectedItems(ObjectSet<uiTreeViewItem>&) const;
    void		removeSelectedItems();

    int			nrItems(bool recursive=false) const;
    void		setCurrentItem(uiTreeViewItem*,int column=0);
    uiTreeViewItem*	currentItem() const;
    int			currentColumn() const;
    int			indexOfItem(uiTreeViewItem*) const;
    uiTreeViewItem*	getItem(int) const;
    uiTreeViewItem*	firstItem() const;
    uiTreeViewItem*	lastItem() const;
    uiTreeViewItem*	findItem(const char*,int,bool) const;

    void		setItemMargin(int);
    int			itemMargin() const;

    void		setShowToolTips(bool);
    bool		showToolTips() const;

    void		clear();
    void		selectAll();
    void		deselectAll(); // added for compatibility only
    void		checkAll();
    void		uncheckAll();

    void		expandAll();
    void		collapseAll();
    void		expandTo(int treedepth);
    void		ensureItemVisible(const uiTreeViewItem*);

			// For MOVING an item to another point in the tree
    void		takeItem(uiTreeViewItem*);
    void		insertItem(int,uiTreeViewItem*);

    uiParent*		parent()		{ return parent_; }
    void		translateText() override;
    bool		handleLongTabletPress() override;

			//! re-draws at next X-loop
    void		triggerUpdate();

			//! item last notified. See notifiers below
    uiTreeViewItem*	itemNotified()		{ return lastitemnotified_; }
    int			columnNotified()	{ return column_; }
    void		unNotify()		{ lastitemnotified_ = 0; }

    void		setNotifiedItem(mQtclass(QTreeWidgetItem*));
    void		setNotifiedColumn(int col)	{ column_ = col; }

    Notifier<uiTreeView> selectionChanged;
    Notifier<uiTreeView> currentChanged;
    Notifier<uiTreeView> itemChanged;
    Notifier<uiTreeView> itemRenamed;
    Notifier<uiTreeView> returnPressed;
    Notifier<uiTreeView> rightButtonClicked;
    Notifier<uiTreeView> rightButtonPressed;
    Notifier<uiTreeView> leftButtonClicked;
    Notifier<uiTreeView> leftButtonPressed;
    Notifier<uiTreeView> mouseButtonPressed;
    Notifier<uiTreeView> mouseButtonClicked;
    Notifier<uiTreeView> contextMenuRequested;
    Notifier<uiTreeView> doubleClicked;
    Notifier<uiTreeView> expanded;
    Notifier<uiTreeView> collapsed;
    Notifier<uiTreeView> unusedKey;

protected:

    mutable BufferString rettxt;
    uiTreeViewItem*	lastitemnotified_;
    uiParent*		parent_;
    int			column_;
    OD::ButtonState	buttonstate_;
    bool		allowDoubleClick() const;
			// Checks clicked mouseposition

    void		cursorSelectionChanged(CallBacker*);
    void		itemChangedCB(CallBacker*);
    void		updateCheckStatus(uiTreeViewItem*);

    uiTreeViewBody*		lvbody()	{ return body_; }
    const uiTreeViewBody*	lvbody() const	{ return body_; }

    void		updateHeaderLabels();

private:


    friend class	i_treeVwMessenger;
    friend class	uiTreeViewBody;
    friend class	uiTreeViewItem;
    friend class	uiTreeViewItemIterator;

    uiTreeViewBody*	body_;

    uiStringSet		labels_;

			// 0: use nr itms in tree
    uiTreeViewBody&	mkbody(uiParent*,const char*,int);

public:

    const char*		columnText(int column) const;
			//Commandline driver usage only

};


/*!

*/

mExpClass(uiBase) uiTreeViewItem : public CallBacker
{ mODTextTranslationClass(uiTreeViewItem);
public:
    friend class	uiTreeView;

    enum			Type { Standard, CheckBox };

    mExpClass(uiBase) Setup
    { mODTextTranslationClass(Setup);
    public:
			Setup( const uiString& txt=uiString::emptyString(),
				       uiTreeViewItem::Type tp=
						uiTreeViewItem::Standard,
				       bool setchecked=true )
				: type_(tp)
				, after_(0)
				, setcheck_(setchecked)
				{ label( txt ); }

	mDefSetupMemb(uiTreeViewItem::Type,type)
	mDefSetupMemb(uiTreeViewItem*,after)
	mDefSetupMemb(BufferString,iconname)
	mDefSetupMemb(bool,setcheck)
	uiStringSet		labels_;

	Setup&			label( const uiString& txt )
				{
				    if ( !txt.isEmpty() )
					labels_ += txt;
				    return *this;
				}
    };

			uiTreeViewItem(uiTreeViewItem* parent,const Setup&);
			uiTreeViewItem(uiTreeView* parent,const Setup&);
    virtual		~uiTreeViewItem();

    mQtclass(QTreeWidgetItem*)	qItem()			{ return qtreeitem_; }
    const mQtclass(QTreeWidgetItem*) qItem() const	{ return qtreeitem_; }

    int			nrChildren() const;

    void		setBGColor(int column,const OD::Color&);

    void		edit(int col);

    void		setCheckable(bool);
    bool		isCheckable() const;
    bool		isChecked(bool qtstatus=true) const;
			//!< returns false if not checkable
    void		setChecked(bool,bool trigger=false);
			//!< does nothing if not checkable
    void		checkAll(bool yn,bool trigger=false);

    void		setToolTip(int column,const uiString&);

    void		translateText();

    static void		updateToolTips();

    void		insertItem(int,uiTreeViewItem*);
    void		takeItem(uiTreeViewItem*);
    void		removeItem(uiTreeViewItem*);
    void		moveItem(uiTreeViewItem* after);
    int			siblingIndex() const;
			/*!<\returns this items index of it's siblings. */

    void		setText( const uiString&,
				 int column=0 );
    void		setText( int i, int column=0 )
			{ setText( toUiString(i), column ); }
    void		setText( float f, int column=0 )
			{ setText( toUiString(f), column ); }
    void		setText( double d, int column=0 )
			{ setText( toUiString(d), column ); }

    const char*		text(int column=0) const;

    void		setIcon(int column,const char* iconname);
    void		removeIcon(int column);
    void		setPixmap(int column,const uiPixmap&);
    void		setPixmap(int column,const OD::Color&,
				  int width=16,int height=10);
    void		setPixmap(int column,const ColTab::Sequence&,
				  int width=16,int height=10);
    void		setBold(int column,bool yn);

    virtual const char* key(int,bool) const		{ return 0; }
    virtual int		compare( uiTreeViewItem*,int column,bool) const
							{ return mUdf(int); }

    void		setOpen(bool yn=true);
    bool		isOpen() const;

    void		setSelected(bool yn);
    bool		isSelected() const;

    uiTreeViewItem*	getChild(int) const;
    uiTreeViewItem*	firstChild() const;
    uiTreeViewItem*	lastChild() const;
    uiTreeViewItem*	nextSibling() const;
    uiTreeViewItem*	prevSibling() const;
    uiTreeViewItem*	parent() const;

    uiTreeViewItem*	itemAbove();
    uiTreeViewItem*	itemBelow();

    uiTreeView*		treeView() const;

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


    Notifier<uiTreeViewItem> stateChanged; //!< only works for CheckBox type
    Notifier<uiTreeViewItem> keyPressed;
			//!< passes CBCapsule<const char*>* cb
			//!< If you handle it, set cb->data = 0;

    static mQtclass(QTreeWidgetItem*)	 qitemFor(uiTreeViewItem*);
    static const mQtclass(QTreeWidgetItem*)  qitemFor(const uiTreeViewItem*);

    static uiTreeViewItem*	 itemFor(mQtclass(QTreeWidgetItem*));
    static const uiTreeViewItem* itemFor(const mQtclass(QTreeWidgetItem*));

protected:

    mQtclass(QTreeWidgetItem*)	qtreeitem_;

    void			init(const Setup&);
    void			updateFlags();
    bool			updateToolTip(int column);

    uiStringSet			texts_;
    uiStringSet			tooltips_;

    bool			isselectable_;
    bool			iseditable_;
    bool			isdragenabled_;
    bool			isdropenabled_;
    bool			ischeckable_;
    bool			isenabled_;
    bool			checked_;
};


mExpClass(uiBase) uiTreeViewItemIterator : public CallBacker
{
public:
				uiTreeViewItemIterator(uiTreeView&);
				~uiTreeViewItemIterator();

    uiTreeViewItem*		next();

protected:
    uiTreeView&			view_;

private:
    mQtclass(QTreeWidgetItemIterator*)	iter_;
};
