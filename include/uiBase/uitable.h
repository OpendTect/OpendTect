#ifndef uitable_h
#define uitable_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          12/02/2003
 RCS:           $Id: uitable.h,v 1.18 2004-12-15 13:28:13 nanne Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "color.h"

class BufferStringSet;
class uiLabel;
class uiTableBody;
class UserInputObj;
class ioPixmap;
class BufferString;
class uiMouseEvent;


class uiTable : public uiObject
{
friend class		i_tableMessenger;
public:

    class RowCol
    {
    public:
			RowCol(int r=0,int c=0)
			    : row(r), col(c) {}

	int		row;
	int		col;
    };

    typedef Geom::Size2D<int>	Size;

    enum SelectionMode
    {
	NoSelection,	//!< No cell can be selected by the user.
	Single,		//!< The user may only select a single range of cells.
	Multi,		//!< The user may select multiple ranges of cells.
	SingleRow,	//!< The user may select one row at once.
	MultiRow,	//!< The user may select multiple rows.
    };

    class Setup
    {
    public:

		    Setup()
			: size_( -1, -1 )
			, rowgrow_(false)
			, colgrow_(false)
			, rowdesc_("Row")
			, coldesc_("Column")
			, fillrow_(false)
			, fillcol_(false)
			, minrowhgt_( 1 )
			, maxrowhgt_( 3 )
			, mincolwdt_( uiObject::baseFldSize() )
			, maxcolwdt_( 2.3 * uiObject::baseFldSize() )
			, selmode_( NoSelection )
			, snglclkedit_( true )
			, defcollbl_(false)
			, defrowlbl_(false)
			, manualresize_(false)		{}

#define mSetVar(var) { var=s; return *this; }
	Setup& size(const Size& s)		mSetVar(size_)
	Setup& rowdesc(const char* s)		mSetVar(rowdesc_)
	Setup& coldesc(const char* s)		mSetVar(coldesc_)
	Setup& rowcangrow(bool s=true)		mSetVar(rowgrow_)
	Setup& colcangrow(bool s=true)		mSetVar(colgrow_)
	Setup& fillrow(bool s=true)		mSetVar(fillrow_)
	Setup& fillcol(bool s=true)		mSetVar(fillcol_)
	Setup& maxrowhgt(float s)		mSetVar(maxrowhgt_)
	Setup& minrowhgt(float s)		mSetVar(minrowhgt_)
	Setup& maxcolwdt(float s)		mSetVar(maxcolwdt_)
	Setup& mincolwdt(float s)		mSetVar(mincolwdt_)
	Setup& selmode(SelectionMode s)		mSetVar(selmode_)
	Setup& singleclickedit(bool s=true)	mSetVar(snglclkedit_)
	Setup& defrowlbl(bool s=true)		mSetVar(defrowlbl_)
	Setup& defcollbl(bool s=true)		mSetVar(defcollbl_)
	Setup& manualResize(bool s=true)	mSetVar(manualresize_)
#undef mSetVar


	Size		size_;
	BufferString	rowdesc_;
	BufferString	coldesc_;
	bool		rowgrow_;
	bool		colgrow_;
	bool		fillrow_; //!< grow cell heights to fill up avail space
	bool		fillcol_; //!< grow cell widths to fill up avail space
	float		minrowhgt_; //!< expressed in multiples of font height
	float		maxrowhgt_; //!< expressed in multiples of font height
	float		mincolwdt_; //!< times average font width
	float		maxcolwdt_; //!< times average font width
	SelectionMode	selmode_;
	bool		snglclkedit_;
	bool		defrowlbl_;
	bool		defcollbl_;
 	bool		manualresize_; //!< adapt size of cells to avail space
    };

                        uiTable(uiParent*, const Setup&,const char* nm="Table");
    virtual 		~uiTable();


    const char*		text(const RowCol&) const;
    void		setText(const RowCol&,const char*);
    void		clearCell(const RowCol&);
    void		setCurrentCell( const RowCol& );


    int			nrRows() const;
    int			nrCols() const;
    void		setNrRows( int nr);
    void		setNrCols( int nr);

    Size		size() const	{ return Size( nrCols(), nrRows() ); }
    void		setSize( const Size& s)
			{
			    setNrCols( s.width() );
			    setNrRows( s.height() );
			}

    int			columnWidth( int col ) const;
    int			rowHeight( int row ) const;

			//! col == -1 : set left margin width
    void		setColumnWidth( int col, int w );
    void		setColumnWidthInChar( int col, float w );

			//! row == -1 : set top margin height
    void		setRowHeight( int row, int h );
    void		setRowHeightInChar( int row, float h );

    void		setColumnStretchable( int col, bool stretch );
    void		setRowStretchable( int row, bool stretch );
    bool		isColumnStretchable( int col ) const;
    bool		isRowStretchable( int row ) const;

    void		setColumnReadOnly(int,bool);
    bool		isColumnReadOnly(int) const;
    void		setRowReadOnly(int,bool);
    bool		isRowReadOnly(int) const;

    void		hideColumn(int,bool);
    void		hideRow(int,bool);
    bool		isColumnHidden(int) const;
    bool		isRowHidden(int) const;

    void		insertRows( int row, int count = 1 );
    void		insertRows( const RowCol& rc, int count = 1 )
			    { insertRows( rc.row, count ); }
    void		insertColumns( int col, int count = 1 );
    void		insertColumns( const RowCol& rc, int count = 1 )
			    { insertColumns( rc.col, count ); }
    void		removeRow( int row );
    void		removeRow( const RowCol& rc )
			    { removeRow( rc.row ); }
    void		removeColumn( int col );
    void		removeColumn( const RowCol& rc )
			    { removeColumn( rc.col ); }

    const char*		rowLabel(int) const;
    const char*		rowLabel( const RowCol& rc ) const
			    { return rowLabel(rc.row); }
    void		setRowLabel( int row, const char* label );
    void		setRowLabels( const char** labels );
    void		setRowLabels( const BufferStringSet& labels );
    void		setRowLabel( const RowCol& rc, const char* label )
			    { setRowLabel( rc.row, label ); }

    const char*		columnLabel(int) const;
    const char*		columnLabel( const RowCol& rc ) const
			    { return columnLabel(rc.col); }
    void		setColumnLabel( int col, const char* label );
    void		setColumnLabels( const char** labels );
    void		setColumnLabels( const BufferStringSet& labels);
    void		setColumnLabel( const RowCol& rc, const char* label )
			    { setColumnLabel( rc.col, label ); }
    void		setDefaultRowLabels();
    void		setDefaultColLabels();

    Setup&		setup() 		{ return setup_; }
    const Setup&	setup() const		{ return setup_; }

    RowCol		notifiedCell() const	{ return notifcell_; }
    Notifier<uiTable>	valueChanged;
    Notifier<uiTable>	leftClicked;
    Notifier<uiTable>	rightClicked;
    Notifier<uiTable>	doubleClicked;

    RowCol		newCell() const		{ return newcell_; }
    Notifier<uiTable>	rowInserted;
    Notifier<uiTable>	rowDeleted;
    Notifier<uiTable>	colInserted;
    Notifier<uiTable>	colDeleted;

    UserInputObj*	mkUsrInputObj(const RowCol&);
    void		delUsrInputObj(const RowCol&);
    UserInputObj*	usrInputObj(const RowCol&);
    const UserInputObj*	usrInputObj(const RowCol& p ) const
			  { return const_cast<uiTable*>(this)->usrInputObj(p); }

    void		setPixmap(const RowCol&,const ioPixmap&);
    void		setColor(const RowCol&,const Color&);
    const Color		getColor(const RowCol&) const;

    int			getIntValue(const RowCol&) const;
    double		getValue(const RowCol&) const;
    float		getfValue(const RowCol&) const;
    void		setValue(const RowCol&,int);
    void		setValue(const RowCol&,float);
    void		setValue(const RowCol&,double);

    void		setSelectionMode( SelectionMode );
    void		editCell(const RowCol&,bool replace=false);

protected:

    RowCol		notifcell_;
    RowCol		newcell_;

    mutable Setup	setup_;
    mutable BufferString rettxt_;

    CNotifier<uiTable,const uiMouseEvent&>	clicked;
    void		clicked_(CallBacker*);
    void		rightClk();

    void		geometrySet_(CallBacker*);
    void		updateCellSizes(uiSize* sz=0);

    void		update( bool row, int rc );

private:

    uiTableBody*	body_;
    uiTableBody&	mkbody(uiParent*, const char*, int, int);

    uiSize		lastsz;

};

#endif
