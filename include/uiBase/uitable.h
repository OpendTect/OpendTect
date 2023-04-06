#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uiobj.h"

#include "color.h"
#include "keyenum.h"
#include "rowcol.h"
#include "draw.h"
#include "uistrings.h"

class BufferStringSet;
class uiPixmap;
class uiGroup;
class uiTableBody;


mExpClass(uiBase) uiTable : public uiObject
{ mODTextTranslationClass(uiTable)
friend class		i_tableMessenger;
friend class		uiTableBody;
public:

    enum SelectionMode
    {
	NoSelection,	//!< No cell can be selected by the user.
	Single,		//!< a single range of cells.
	Multi,		//!< multiple ranges of cells.
	SingleRow,
	SingleColumn
    };

    enum SelectionBehavior
    {
	SelectItems, SelectRows, SelectColumns
    };

    enum ResizeMode
    {
	Interactive, Stretch, Fixed, ResizeToContents
    };

    mExpClass(uiBase) Setup
    {
    public:

		    Setup(int nrrows=-1,int nrcols=-1)
			: size_(nrrows,nrcols)
			, rowdesc_(uiStrings::sRow())
			, coldesc_(uiStrings::sColumn())
			, insertrowallowed_(true)
			, removerowallowed_(true)
			, rowgrow_(false) //!< can extra rows be added by user?
			, insertcolallowed_(true)
			, removecolallowed_(true)
			, colgrow_(false) //!< can extra cols be added by user?
			, fillrow_(false) //!< adjust cell height to avail space
			, fillcol_(false) //!< adjust cell width to avail space
			, minrowhgt_(1.f) //!< units of font height
			, maxrowhgt_(3.f) //!< units of font height
			, mincolwdt_(1.f*uiObject::baseFldSize())
					  //!< units of font
			, maxcolwdt_(2.3f*uiObject::baseFldSize())
					  //!< units of font
			, selmode_(NoSelection)
			, removeselallowed_(true)
			, snglclkedit_(true)
			, defcollbl_(false)
			, defrowlbl_(false)
			, manualresize_(false)
					//!< if not, adapt size of cells auto
			, defrowstartidx_(1)
				//!< default row label: start counting at 1
			, rightclickdisabled_(false)
				//!<default enables right click popup
			, enablecopytext_(false)
				{}
			~Setup()
			{}

	mDefSetupMemb(RowCol,size)
	mDefSetupMemb(uiString,rowdesc)
	mDefSetupMemb(uiString,coldesc)
	mDefSetupMemb(bool,rowgrow)
	mDefSetupMemb(bool,colgrow)
	mDefSetupMemb(bool,insertrowallowed)
	mDefSetupMemb(bool,removerowallowed)
	mDefSetupMemb(bool,insertcolallowed)
	mDefSetupMemb(bool,removecolallowed)
	mDefSetupMemb(bool,fillrow)
	mDefSetupMemb(bool,fillcol)
	mDefSetupMemb(float,maxrowhgt)
	mDefSetupMemb(float,minrowhgt)
	mDefSetupMemb(float,maxcolwdt)
	mDefSetupMemb(float,mincolwdt)
	mDefSetupMemb(SelectionMode,selmode)
	mDefSetupMemb(bool,removeselallowed)
	mDefSetupMemb(bool,snglclkedit)
	mDefSetupMemb(bool,defrowlbl)
	mDefSetupMemb(bool,defcollbl)
	mDefSetupMemb(bool,manualresize)
	mDefSetupMemb(int,defrowstartidx)
	mDefSetupMemb(bool,rightclickdisabled)
	mDefSetupMemb(bool,enablecopytext)

	Setup& sizesFixed( bool yn )
	{
	    insertrowallowed_ = removerowallowed_ = rowgrow_ =
	    insertcolallowed_ = removecolallowed_ = colgrow_ = !yn;
	    return *this;
	}

	Setup& rowdesc( const char* desc )
			{ rowdesc_ = toUiString(desc); return *this; }
	Setup& coldesc( const char* desc )
			{ coldesc_ = toUiString(desc); return *this; }
    };

			uiTable(uiParent*,const Setup&,const char* nm);
			uiTable(uiParent*,const char* nm);
    virtual		~uiTable();
			mOD_DisableCopy(uiTable)

    const char*		text(const RowCol&) const;
    void		setText(const RowCol&,const char*);
    void		setText(const RowCol&,const OD::String&);
    void		setText(const RowCol&,const uiString&);
    void		clearCell(const RowCol&);
    void		clearTable();
    void		showGrid(bool);
    bool		gridShown() const;
    void		setCurrentCell(const RowCol&,bool noselection=false);
    void		setCellObject(const RowCol&,uiObject*);
			/*!<\note The uiObject should be given a NULL pointer
			     as uiParent* at construction. */
    uiObject*		getCellObject(const RowCol&) const;
    void		clearCellObject(const RowCol&);
    RowCol		getCell(uiObject*);
    void		setCellGroup(const RowCol&,uiGroup*);
			/*!<\note The uiObject should be given a NULL pointer
			     as uiParent* at construction. */
    uiGroup*		getCellGroup(const RowCol&) const;
    RowCol		getCell(uiGroup*);
    void		setCellChecked(const RowCol&,bool yn);
    bool		isCellChecked(const RowCol&) const;

    int			nrRows() const;
    int			nrCols() const;
    void		setNrRows(int);
    void		setNrCols(int);
    void		setPrefHeightInRows(int);

    int			columnWidth(int) const;
    int			rowHeight(int) const;

    void		setLeftMargin(int);
    void		setColumnWidth(int col,int w);
    void		setColumnWidthInChar(int col,float w);

    void		setTopMargin(int);
    void		setRowHeight(int row,int h);
    void		setRowHeightInChar(int row,float h);

    void		resizeHeaderToContents(bool hor);
    void		resizeColumnToContents(int);
    void		resizeColumnsToContents();
    void		resizeRowToContents(int);
    void		resizeRowsToContents();
    void		setColumnResizeMode(ResizeMode);
			//!<Default is Stretch
    void		setRowResizeMode(ResizeMode);
			//!<Default is ResizeToContents
    void		setColumnStretchable(int,bool);
    void		setRowStretchable(int,bool);
    bool		isColumnStretchable(int) const;
    bool		isRowStretchable(int) const;
    void		setSortable(bool);

    void		setTableReadOnly(bool);
    bool		isTableReadOnly() const;

    void		setColumnReadOnly(int,bool);
    bool		isColumnReadOnly(int) const;
    void		setRowReadOnly(int,bool);
    bool		isRowReadOnly(int) const;

    void		setCellReadOnly(const RowCol&,bool);
    bool		isCellReadOnly(const RowCol&) const;
    bool		isCellVisible(const RowCol&) const;

    void		hideColumn(int,bool);
    void		hideRow(int,bool);
    bool		isColumnHidden(int) const;
    bool		isRowHidden(int) const;

    bool		isTopHeaderHidden() const;
    bool		isLeftHeaderHidden() const;
    void		setTopHeaderHidden(bool);
    void		setLeftHeaderHidden(bool);

    void		insertRows(int row,int count);
    inline void		insertRows( const RowCol& rc, int count )
			    { insertRows( rc.row(), count ); }
    void		insertColumns(int col,int count);
    inline void		insertColumns( const RowCol& rc, int count )
			    { insertColumns( rc.col(), count ); }
    void		removeRow(int);
    void		removeRow( const RowCol& rc )
				{ removeRow( rc.row() ); }
    void		removeRows(const TypeSet<int>&);
    void		removeColumn(int);
    void		removeColumn( const RowCol& rc )
				{ removeColumn( rc.col() ); }
    void		removeColumns(const TypeSet<int>&);

    bool		isSelected(const RowCol&) const;
    bool		isRowSelected(int) const;
    bool		isColumnSelected(int) const;
			// next 3 return in selected order
    bool		getSelectedRows(TypeSet<int>&) const;
    bool		getSelectedCols(TypeSet<int>&) const;
    bool		getSelectedCells(TypeSet<RowCol>&) const;
    int			currentRow() const;
    int			currentCol() const;
    RowCol		currentCell() const
			{ return RowCol( currentRow(), currentCol() ); }
    void		setSelected(const RowCol&,bool yn=true);
    void		selectRow(int row);
    void		selectColumn(int col);
    void		selectItems(const TypeSet<RowCol>&,bool);
    void		removeAllSelections();
    void		ensureCellVisible(const RowCol&);

    const char*		rowLabel(int) const;
    const char*		rowLabel( const RowCol& rc ) const
			    { return rowLabel(rc.row()); }
    void		setRowLabel(int,const uiString&); // also sets tooltip
    void		setRowLabels(const uiStringSet&);
    void		setRowLabels(const BufferStringSet&);
    void		setRowLabel( const RowCol& rc, const uiString& lbl )
			    { setRowLabel( rc.row(), lbl ); }
    void		setRowToolTip(int,const uiString&);

    const char*		columnLabel(int) const;
    const char*		columnLabel( const RowCol& rc ) const
			    { return columnLabel(rc.col()); }
    void		setColumnLabel(int,const uiString&); //also sets tooltip
    void		setColumnLabels(const uiStringSet&);
    void		setColumnLabels(const BufferStringSet&);
    void		setColumnLabel( const RowCol& rc, const uiString& lbl )
			    { setColumnLabel( rc.col(), lbl ); }
    void		setColumnToolTip(int,const uiString&);
    void		setColumnSortIndicator(int,bool asc);

    void		setCellToolTip(const RowCol&,const uiString&);

    void		setDefaultRowLabels();
    void		setDefaultColLabels();
    void		setLabelAlignment(Alignment::HPos,bool cols);
    void		setLabelBGColor(int,OD::Color,bool isrow);


    Setup&		setup()		{ return setup_; }
    const Setup&	setup() const		{ return setup_; }

    const RowCol&	notifiedCell() const	{ return notifcell_; }
    void		setNotifiedCell(const RowCol& rc)
						{ notifcell_=rc; }
    const TypeSet<int>&	getNotifRCs() const
			{ return seliscols_ ? notifcols_ : notifrows_; }
    const TypeSet<RowCol>& getNotifCells() const { return notifcells_; }

    Notifier<uiTable>	valueChanged;
    Notifier<uiTable>	leftClicked;
    Notifier<uiTable>	rightClicked;
    Notifier<uiTable>	doubleClicked;
    Notifier<uiTable>	selectionChanged;

    const RowCol&	newCell() const		{ return newcell_; }
    Notifier<uiTable>	rowInserted;
    Notifier<uiTable>	rowDeleted;
    Notifier<uiTable>	selectionDeleted;
    Notifier<uiTable>	colInserted;
    Notifier<uiTable>	colDeleted;
    CNotifier<uiTable,int> rowClicked;
    CNotifier<uiTable,int> columnClicked;

    void		setPixmap(const RowCol&,const uiPixmap&);
    void		setColor(const RowCol&,const OD::Color&);
    OD::Color		getColor(const RowCol&) const;
    void		setHeaderBackground(int,const OD::Color&,bool isrow);
    OD::Color		getHeaderBackground(int,bool isrow) const;
    void		showOuterFrame(bool);

    int			getIntValue(const RowCol&) const;
    od_int64		getInt64Value(const RowCol&) const;
    double		getDValue(const RowCol&) const;
    float		getFValue(const RowCol&) const;
    void		setValue(const RowCol&,int);
    void		setValue(const RowCol&,od_int64);
    void		setValue(const RowCol&,float);
    void		setValue(const RowCol&,float,int nrdec);
    void		setValue(const RowCol&,double);
    void		setValue(const RowCol&,double,int nrdec);

    void		setSelectionMode(SelectionMode);
    void		setSelectionBehavior(SelectionBehavior);
    void		editCell(const RowCol&,bool replace=false);
    void		setMouseTracking(bool);

    mExpClass(uiBase) SelectionRange
    {
    public:
			SelectionRange()
			    : firstrow_(-1), lastrow_(-1)
			    , firstcol_(-1), lastcol_(-1)	{}
			~SelectionRange()
			{}

	int		nrRows() const	{ return lastrow_-firstrow_+1; }
	int		nrCols() const	{ return lastcol_-firstcol_+1; }

	int		firstrow_;
	int		lastrow_;

	int		firstcol_;
	int		lastcol_;
    };


    const ObjectSet<SelectionRange>&	selectedRanges() const;

    SelectionBehavior	getSelBehavior() const;
    int			maxNrOfSelections() const;

    bool		handleLongTabletPress() override;
    bool		needOfVirtualKeyboard() const;
    void		popupVirtualKeyboard(int globalx=-1,int globaly=-1);

protected:

    mutable ObjectSet<SelectionRange> selranges_;
    RowCol		notifcell_;
    TypeSet<RowCol>	notifcells_;
    TypeSet<int>	notifrows_;
    TypeSet<int>	notifcols_;
    bool		seliscols_;
    RowCol		newcell_;

    mutable Setup	setup_;

    virtual void	popupMenu(CallBacker*);
    OD::ButtonState	buttonstate_;

    void		updateCellSizes(const uiSize* sz=0);
    void		cellObjChangedCB(CallBacker*);

    bool		getSelected();
    void		removeRCs(const TypeSet<int>&,bool col);
    void		update(bool row,int nr);

    bool		istablereadonly_;

private:

    uiTableBody*	body_;
    uiTableBody&	mkbody(uiParent*,const char*,int,int);

    mutable uiSize	lastsz;

public:
    mDeprecated		("Use getDValue")
    double		getdValue( const RowCol& rc ) const
			{ return getDValue( rc ); }
    mDeprecated		("Use getFValue")
    float		getfValue( const RowCol& rc ) const
			{ return getFValue( rc ); }

    void	       setPrefWidthInChars(int);
};
