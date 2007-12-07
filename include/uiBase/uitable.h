#ifndef uitable_h
#define uitable_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          12/02/2003
 RCS:           $Id: uitable.h,v 1.33 2007-12-07 08:11:22 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "color.h"
#include "rowcol.h"
#include "uigroup.h"

class uiLabel;
class ioPixmap;
class uiTableBody;
class UserInputObj;
class MouseEvent;
class BufferStringSet;


class uiTable : public uiObject
{
friend class		i_tableMessenger;
public:

    typedef Geom::Size2D<int>	Size;

    enum SelectionMode
    {
	NoSelection,	//!< No cell can be selected by the user.
	Single,		//!< a single range of cells.
	Multi,		//!< multiple ranges of cells.
	SingleRow,	//!< one row at once.
	MultiRow,	//!< multiple rows.
    };

    class Setup
    {
    public:

		    Setup()
			: size_(-1,-1)
			, rowdesc_("Row")	
			, coldesc_("Column")
			, rowgrow_(false) //!< can extra rows be added by user?
			, colgrow_(false) //!< can extra cols be added by user?
			, fillrow_(false) //!< adjust cell height to avail space
			, fillcol_(false) //!< adjust cell width to avail space
			, minrowhgt_( 1 ) //!< units of font height
			, maxrowhgt_( 3 ) //!< units of font height
			, mincolwdt_(uiObject::baseFldSize())
					  //!< units of font
			, maxcolwdt_(2.3*uiObject::baseFldSize())
					  //!< units of font
			, selmode_( NoSelection )
			, snglclkedit_( true )
			, defcollbl_(false)
			, defrowlbl_(false)
			, manualresize_(false)
					//!< if not, adapt size of cells auto
				{}

	mDefSetupMemb(Size,size)
	mDefSetupMemb(BufferString,rowdesc)
	mDefSetupMemb(BufferString,coldesc)
	mDefSetupMemb(bool,rowgrow)
	mDefSetupMemb(bool,colgrow)
	mDefSetupMemb(bool,fillrow)
	mDefSetupMemb(bool,fillcol)
	mDefSetupMemb(float,maxrowhgt)
	mDefSetupMemb(float,minrowhgt)
	mDefSetupMemb(float,maxcolwdt)
	mDefSetupMemb(float,mincolwdt)
	mDefSetupMemb(SelectionMode,selmode)
	mDefSetupMemb(bool,snglclkedit)
	mDefSetupMemb(bool,defrowlbl)
	mDefSetupMemb(bool,defcollbl)
	mDefSetupMemb(bool,manualresize)

    };

                        uiTable(uiParent*, const Setup&,const char* nm="Table");
    virtual 		~uiTable();


    const char*		text(const RowCol&) const;
    void		setText(const RowCol&,const char*);
    void		clearCell(const RowCol&);
    void		clearTable();
    void		showGrid(bool);
    bool		gridShown() const;
    void		setCurrentCell(const RowCol&);
    void		setCellObject(const RowCol&,uiObject*);
    uiObject*		getCellObject(const RowCol&) const;
    void		clearCellObject(const RowCol&);

    int			nrRows() const;
    int			nrCols() const;
    void		setNrRows(int);
    void		setNrCols(int);

    Size		size() const	{ return Size( nrCols(), nrRows() ); }
    inline void		setSize( const Size& s)
			{ setNrCols( s.width() ); setNrRows( s.height() ); }

    int			columnWidth(int) const;
    int			rowHeight(int) const;

    void		setLeftMargin(int);
    void		setColumnWidth(int col,int w);
    void		setColumnWidthInChar(int col,float w);

    void		setTopMargin(int);
    void		setRowHeight(int row,int h);
    void		setRowHeightInChar(int row,float h);

    void		setColumnStretchable(int,bool);
    void		setRowStretchable(int,bool);
    bool		isColumnStretchable(int) const;
    bool		isRowStretchable(int) const;

    void		setTableReadOnly(bool);
    bool		isTableReadOnly() const;

    void		setColumnReadOnly(int,bool);
    bool		isColumnReadOnly(int) const;
    void		setRowReadOnly(int,bool);
    bool		isRowReadOnly(int) const;

    void		hideColumn(int,bool);
    void		hideRow(int,bool);
    bool		isColumnHidden(int) const;
    bool		isRowHidden(int) const;

    void		insertRows(int row,int count);
    inline void		insertRows( const RowCol& rc, int count )
			    { insertRows( rc.row, count ); }
    void		insertColumns(int col,int count);
    inline void		insertColumns( const RowCol& rc, int count )
			    { insertColumns( rc.col, count ); }
    void		removeRow(int);
    void		removeRow( const RowCol& rc )
    				{ removeRow( rc.row ); }
    void		removeRows(const TypeSet<int>&);
    void		removeColumn(int);
    void		removeColumn( const RowCol& rc )
    				{ removeColumn( rc.col ); }
    void		removeColumns(const TypeSet<int>&);

    bool		isRowSelected(int) const;
    bool		isColumnSelected(int) const;
    int			currentRow() const;
    int			currentCol() const;
    RowCol		currentCell() const
    			{ return RowCol( currentRow(), currentCol() ); }
    void		selectRow(int row);
    void 		selectColumn(int col);
    void		removeAllSelections();
    void 		ensureCellVisible(const RowCol&);

    const char*		rowLabel(int) const;
    const char*		rowLabel( const RowCol& rc ) const
			    { return rowLabel(rc.row); }
    void		setRowLabel(int,const char*);
    void		setRowLabels(const char**);
    void		setRowLabels(const BufferStringSet&);
    void		setRowLabel( const RowCol& rc, const char* lbl )
			    { setRowLabel( rc.row, lbl ); }

    const char*		columnLabel(int) const;
    const char*		columnLabel( const RowCol& rc ) const
			    { return columnLabel(rc.col); }
    void		setColumnLabel(int,const char*);
    void		setColumnLabels(const char**);
    void		setColumnLabels(const BufferStringSet&);
    void		setColumnLabel( const RowCol& rc, const char* lbl )
			    { setColumnLabel( rc.col, lbl ); }
    void		setDefaultRowLabels();
    void		setDefaultColLabels();

    Setup&		setup() 		{ return setup_; }
    const Setup&	setup() const		{ return setup_; }

    const RowCol&	notifiedCell() const	{ return notifcell_; }
    Notifier<uiTable>	valueChanged;
    Notifier<uiTable>	leftClicked;
    Notifier<uiTable>	rightClicked;
    Notifier<uiTable>	doubleClicked;

    const RowCol&	newCell() const		{ return newcell_; }
    Notifier<uiTable>	rowInserted;
    Notifier<uiTable>	rowDeleted;
    Notifier<uiTable>	colInserted;
    Notifier<uiTable>	colDeleted;

    void		setPixmap(const RowCol&,const ioPixmap&);
    void		setColor(const RowCol&,const Color&);
    const Color		getColor(const RowCol&) const;

    int			getIntValue(const RowCol&) const;
    double		getValue(const RowCol&) const;
    float		getfValue(const RowCol&) const;
    void		setValue(const RowCol&,int);
    void		setValue(const RowCol&,float);
    void		setValue(const RowCol&,double);

    void		setSelectionMode(SelectionMode);
    void		editCell(const RowCol&,bool replace=false);

protected:

    RowCol		notifcell_;
    RowCol		newcell_;

    mutable Setup	setup_;

    CNotifier<uiTable,const MouseEvent&>	clicked;
    void		clicked_(CallBacker*);
    void		rightClk();

    void		geometrySet_(CallBacker*);
    void		updateCellSizes(const uiSize* sz=0);

    void		update(bool row,int nr);
    void		removeRCs(const TypeSet<int>&,bool);

private:

    uiTableBody*	body_;
    uiTableBody&	mkbody(uiParent*,const char*,int,int);

    mutable uiSize	lastsz;

};

#endif
