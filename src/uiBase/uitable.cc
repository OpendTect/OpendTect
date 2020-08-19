/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          12/02/2003
________________________________________________________________________

-*/


#include "uitable.h"
#include "i_qtable.h"

#include "uiclipboard.h"
#include "uicolor.h"
#include "uicombobox.h"
#include "uifont.h"
#include "uilabel.h"
#include "uimenu.h"
#include "uiobjbodyimpl.h"
#include "uipixmap.h"
#include "uistrings.h"
#include "uivirtualkeyboard.h"

#include "bufstringset.h"
#include "convert.h"
#include "staticstring.h"
#include "i_layoutitem.h"

#include "q_uiimpl.h"

#include <QApplication>
#include <QCursor>
#include <QHeaderView>
#include <QMouseEvent>
#include <QLabel>
#include <QScreen>

mUseQtnamespace


static inline QColor getQCol( const Color& col )
{
    return QColor( col.r(), col.g(), col.b(), 255-col.t() );
}


static inline Color getODCol( const QColor& qcol )
{
    return Color( qcol.red(), qcol.green(), qcol.blue(), 255-qcol.alpha() );
}


class CellObject
{
    public:
			CellObject( QWidget* qw, uiObject* obj,
				    const RowCol& rc )
			    : qwidget_(qw)
			    , uiobj_(obj)
			    , rowcol_(rc)    {}
			~CellObject();

    uiObject*		uiobj_;
    QWidget*		qwidget_;
    RowCol		rowcol_;
};


CellObject::~CellObject()
{
    mDynamicCastGet(uiGroupObj*,grpobj,uiobj_);

    if ( grpobj && grpobj->group() )
	delete grpobj->group();
    else
	delete uiobj_;
}


class uiTableBody : public uiObjBodyImpl<uiTable,QTableWidget>
{
public:
			uiTableBody(uiTable&,uiParent*,const char*,int,int);
			~uiTableBody();

    void		setNrLines(int);
    void		setPrefHeightInRows(int nrrows,int maxheight);
    void		setPrefWidthInChars(int nrchars,int maxwidth);
    virtual int		nrTxtLines() const;

    QTableWidgetItem*	getItem(const RowCol&,bool createnew=true);

    void		clearTable();
    void		clearCellObject(const RowCol&);
    uiObject*		getCellObject(const RowCol&) const;
    void		setCellObject(const RowCol&,uiObject*);
    RowCol		getCell(uiObject*);

    virtual void	finalise();

    int			maxNrOfSelections() const;
    uiTable::SelectionBehavior getSelBehavior() const;


    QTableWidgetItem&	getRCItem(int,bool isrow);

protected:

    virtual void	contextMenuEvent(QContextMenuEvent*);
    virtual void	mousePressEvent(QMouseEvent*);
    virtual void	mouseReleaseEvent(QMouseEvent*);
    virtual void	keyPressEvent(QKeyEvent*);

    void		copy();
    void		paste();
    void		cut();

    ObjectSet<CellObject> cellobjects_;

    BoolTypeSet		columnsreadonly;
    BoolTypeSet		rowsreadonly;

private:

    i_tableMessenger&	messenger_;

};



static void setResizeMode( QHeaderView* hdr, QHeaderView::ResizeMode mode,
			   int idx=-1 )
{
    if ( idx<0 )
	hdr->setSectionResizeMode( mode );
    else
	hdr->setSectionResizeMode( idx, mode );
}


uiTableBody::uiTableBody( uiTable& hndl, uiParent* parnt, const char* nm,
			  int nrows, int ncols )
    : uiObjBodyImpl<uiTable,QTableWidget>(hndl,parnt,nm)
    , messenger_ (*new i_tableMessenger(this,&hndl))
{
    if ( nrows >= 0 )
	setNrLines( nrows );
    if ( ncols >= 0 )
	setColumnCount( ncols );

// TODO: Causes tremendous performance delay in Qt 4.4.1;
//       For now use uiTable::resizeRowsToContents() in stead.
//    QHeaderView* vhdr = verticalHeader();
//    vhdr->setResizeMode( QHeaderView::ResizeToContents );

    setResizeMode( horizontalHeader(), QHeaderView::Stretch );
    setHorizontalScrollMode( QAbstractItemView::ScrollPerPixel );
    setMouseTracking( true );
    setCornerButtonEnabled( false );
}


uiTableBody::~uiTableBody()
{
    deepErase( cellobjects_ );
    delete &messenger_;
}


void uiTableBody::clearTable()
{
    deepErase( cellobjects_ );
    clearContents();
}


QTableWidgetItem& uiTableBody::getRCItem( int idx, bool isrow )
{
    QTableWidgetItem* itm = isrow ? verticalHeaderItem( idx )
				  : horizontalHeaderItem( idx );
    if ( !itm )
    {
	itm = new QTableWidgetItem;
	if ( isrow )
	    setVerticalHeaderItem( idx, itm );
	else
	    setHorizontalHeaderItem( idx, itm );
    }

    return *itm;
}


void uiTableBody::contextMenuEvent( QContextMenuEvent* ev )
{
    if ( !ev ) return;

    const QPoint evpos = ev->pos();
    QTableWidgetItem* itm = itemAt( evpos );
    handle_.setNotifiedCell( itm ? RowCol(itm->row(),itm->column())
				 : RowCol(-1,-1) );
    handle_.popupMenu(0);
}


void uiTableBody::mousePressEvent( QMouseEvent* ev )
{
    if ( !ev ) return;

    if ( ev->button() == Qt::RightButton )
	handle_.buttonstate_ = OD::RightButton;
    else if ( ev->button() == Qt::LeftButton )
	handle_.buttonstate_ = OD::LeftButton;
    else
	handle_.buttonstate_ = OD::NoButton;

    QTableWidget::mousePressEvent( ev );
    handle_.buttonstate_ = OD::NoButton;
}


void uiTableBody::mouseReleaseEvent( QMouseEvent* ev )
{
    if ( !ev ) return;

    if ( ev->button() == Qt::RightButton )
	handle_.buttonstate_ = OD::RightButton;
    else if ( ev->button() == Qt::LeftButton )
	handle_.buttonstate_ = OD::LeftButton;
    else
	handle_.buttonstate_ = OD::NoButton;

    QTableWidget::mouseReleaseEvent( ev );
    handle_.buttonstate_ = OD::NoButton;
}


void uiTableBody::keyPressEvent( QKeyEvent* ev )
{
    if ( ev->matches(QKeySequence::Copy) )
	copy();
    else if ( ev->matches(QKeySequence::Paste) )
	paste();
    else if ( ev->matches(QKeySequence::Cut) )
	cut();
    else
	QTableWidget::keyPressEvent( ev );
}


void uiTableBody::copy()
{
    QList<QTableWidgetSelectionRange> ranges = selectedRanges();
    if ( ranges.isEmpty() )
	return;

    const QTableWidgetSelectionRange& range = ranges.first();
    BufferString str;
    for ( int i=0; i<range.rowCount(); i++ )
    {
	if ( i > 0 )
	    str.addNewLine();

	for ( int j=0; j<range.columnCount(); j++ )
	{
	    if ( j > 0 )
		str.addTab();

	    QTableWidgetItem* itm =
		item( range.topRow()+i, range.leftColumn()+j );
	    if ( itm )
		str.add( itm->text() );
	}
    }

    str.addNewLine();
    uiClipboard::setText( toUiString(str) );
}


void uiTableBody::paste()
{
    const QString str = uiClipboard::getText();
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
    const QStringList rows = str.split( '\n', Qt::SkipEmptyParts );
#else
    const QStringList rows = str.split( '\n', QString::SkipEmptyParts );
#endif

    const int nrrows = rows.count();
    const int nrcols = rows.first().count('\t') + 1;
    const int startrow = currentRow();
    const int totalnrrows = nrrows + startrow;
    const int startcol = currentColumn();

    if ( rowCount() < totalnrrows )
	setRowCount( totalnrrows );

    for ( int i = 0; i<nrrows; i++ )
    {
	setCurrentCell( startrow+i, startcol );
	QStringList columns = rows[i].split( '\t' );
	for ( int j=0; j<nrcols; j++ )
	{
	    const RowCol rc( startrow+i, startcol+j );
	    QTableWidgetItem* itm = getItem( rc, true );
	    if ( itm )
		itm->setText( columns[j] );
	}
    }
}


void uiTableBody::cut()
{
    copy();

    QList<QTableWidgetSelectionRange> ranges = selectedRanges();
    if ( ranges.isEmpty() ) return;

    const QTableWidgetSelectionRange& range = ranges.first();
    for ( int i=0; i<range.rowCount(); i++ )
    {
	for ( int j=0; j<range.columnCount(); j++ )
	{
	    QTableWidgetItem* itm =
		item( range.topRow()+i, range.leftColumn()+j );
	    if ( itm ) itm->setText( "" );
	}
    }
}


void uiTableBody::setNrLines( int prefnrlines )
{
    const int maxheight = 200; // should be about 5 rows
    setRowCount( prefnrlines );
    setPrefHeightInRows( prefnrlines, maxheight );
}


void uiTableBody::setPrefHeightInRows( int nrrows, int maxheight )
{
    if ( finalised() || nrrows<=0 )
	return;

    QHeaderView* vhdr = verticalHeader();
    const QSize qsz = vhdr->sizeHint();
    const int rowh = rowHeight(0) + 1;
    const int prefh = rowh*nrrows + qsz.height();
    setPrefHeight( mMIN(prefh,maxheight) );
}


void uiTableBody::setPrefWidthInChars( int nrchars, int maxwidth )
{
    if ( finalised() || nrchars<=0 )
	return;

    QHeaderView* hhdr = horizontalHeader();
    const QSize qsz = hhdr->sizeHint();
    const float lookgoodfactor = 1.5;	// emperical
    const int charw = mCast( int, fontWidth() * lookgoodfactor );
    const int prefw = charw*nrchars + qsz.width();
    setPrefWidth( mMIN(prefw,maxwidth) );
}


int uiTableBody::nrTxtLines() const
{ return rowCount()>=0 ? rowCount()+1 : 7; }


QTableWidgetItem* uiTableBody::getItem( const RowCol& rc, bool createnew )
{
    QTableWidgetItem* itm = item( rc.row(), rc.col() );
    if ( !itm && createnew )
    {
	itm = new QTableWidgetItem;
	itm->setBackground( QBrush(getQCol(Color::White())) );
	setItem( rc.row(), rc.col(), itm );
    }

    return itm;
}


void uiTableBody::setCellObject( const RowCol& rc, uiObject* obj )
{
    if ( !obj )
    {
	setCellWidget( rc.row(), rc.col(), 0 );
	return;
    }

    getItem( rc );
    QWidget* qw = obj->body()->qwidget();
    setCellWidget( rc.row(), rc.col(), qw );
    cellobjects_ += new CellObject( qw, obj, rc );
}


uiObject* uiTableBody::getCellObject( const RowCol& rc ) const
{
    QWidget* qw = cellWidget( rc.row(), rc.col() );
    if ( !qw ) return 0;

    uiObject* obj = 0;
    for ( int idx=0; idx<cellobjects_.size(); idx++ )
    {
	if ( cellobjects_[idx]->qwidget_ == qw )
	{
	    obj = cellobjects_[idx]->uiobj_;
	    break;
	}
    }

    return obj;
}


RowCol uiTableBody::getCell( uiObject* obj )
{
    for ( int idx=0; idx<cellobjects_.size(); idx++ )
    {
	if ( cellobjects_[idx]->uiobj_ == obj )
	    return cellobjects_[idx]->rowcol_;
    }

    return RowCol( -1, -1 );
}


void uiTableBody::clearCellObject( const RowCol& rc )
{
    QWidget* qw = cellWidget( rc.row(), rc.col() );
    if ( !qw ) return;

    CellObject* co = 0;
    for ( int idx=0; idx<cellobjects_.size(); idx++ )
    {
	if ( cellobjects_[idx]->qwidget_ == qw )
	{
	    co = cellobjects_[idx];
	    break;
	}
    }

    setCellWidget( rc.row(), rc.col(), 0 );
    if ( co )
    {
	cellobjects_ -= co;
	delete co;
    }
}


void uiTableBody::finalise()
{
    uiObjectBody::finalise();
    for ( auto obj : cellobjects_ )
	obj->uiobj_->finalise();
}


uiTable::SelectionBehavior uiTableBody::getSelBehavior() const
{
    return (uiTable::SelectionBehavior) selectionBehavior();
}


int uiTableBody::maxNrOfSelections() const
{
    if ( selectionMode()==QAbstractItemView::NoSelection )
	return 0;
    if ( selectionMode()==QAbstractItemView::SingleSelection )
	return 1;
    if ( getSelBehavior()==uiTable::SelectRows )
	return rowCount();
    if ( getSelBehavior()==uiTable::SelectColumns )
	return columnCount();

    return rowCount()*columnCount();
}



uiTable::uiTable( uiParent* p, const Setup& s, const char* nm )
    : uiObject(p,nm,mkbody(p,nm,s.size_.row(),s.size_.col()))
    , setup_(s)
    , buttonstate_(OD::NoButton)
    , cornerlabel_(0)
    , valueChanged(this)
    , rightClicked(this)
    , leftClicked(this)
    , doubleClicked(this)
    , rowInserted(this)
    , colInserted(this)
    , rowDeleted(this)
    , colDeleted(this)
    , selectionChanged(this)
    , selectionDeleted(this)
    , rowClicked(this)
    , columnClicked(this)
    , colorSelectionChanged(this)
    , istablereadonly_(false)
    , seliscols_(false)
{
    setGeometry.notify( mCB(this,uiTable,geometrySet_) );

    setStretch( 2, 2 );

    setSelectionMode( s.selmode_ );
    if ( s.defrowlbl_ )
	setDefaultRowLabels();
    if ( s.defcollbl_ )
	setDefaultColLabels();

    QHeaderView* hhdr = body_->horizontalHeader();
    hhdr->setMinimumSectionSize( (int)(s.mincolwdt_*body_->fontWidth()) );
}


uiTableBody& uiTable::mkbody( uiParent* p, const char* nm, int nr, int nc )
{
    body_ = new uiTableBody( *this, p, nm, nr, nc );
    return *body_;
}


uiTable::~uiTable()
{
    deepErase( selranges_ );
}


void uiTable::showGrid( bool yn )
{ body_->setShowGrid( yn ); }

bool uiTable::gridShown() const
{ return body_->showGrid(); }


void uiTable::setDefaultRowLabels()
{
    const int nrrows = nrRows();
    for ( int idx=0; idx<nrrows; idx++ )
    {
	uiString lbl( setup_.rowdesc_ );
	lbl = toUiString("%1 %2").arg(lbl).arg(idx + setup_.defrowstartidx_);
	setRowLabel( idx, lbl );
    }
}


void uiTable::setDefaultColLabels()
{
    const int nrcols = nrCols();
    for ( int idx=0; idx<nrcols; idx++ )
    {
	uiString lbl = toUiString("%1 %2").arg(toUiString(setup_.coldesc_))
					  .arg(idx+1);
	setColumnLabel( idx, lbl );
    }
}

#define updateRow(r) update( true, r )
#define updateCol(c) update( false, c )

void uiTable::update( bool row, int rc )
{
    if ( row && setup_.defrowlbl_ ) setDefaultRowLabels();
    else if ( !row && setup_.defcollbl_ ) setDefaultColLabels();

    int max = row ? nrRows() : nrCols();

    if ( rc > max ) rc = max;
    if ( rc < 0 ) rc = 0;

    int r = row ? rc : 0;
    int c = row ? 0 : rc;

    setCurrentCell( RowCol(r,c) );
}


int uiTable::columnWidth( int col ) const
{
    return col == -1 ? 0 : body_->columnWidth(col);
}


int uiTable::rowHeight( int row ) const
{
    return row == -1 ? 0 : body_->rowHeight( row );
}


void uiTable::setLeftMargin( int wdth )
{
    QHeaderView* header = body_->verticalHeader();
    if ( !header ) return;

    header->setVisible( wdth > 0 );
}


void uiTable::setColumnWidth( int col, int w )
{
    if ( col >= 0 )
	body_->setColumnWidth( col, w );
    else if ( col == -1 )
    {
	for ( int idx=0; idx<nrCols(); idx++ )
	    body_->setColumnWidth( idx, w );
    }
}


void uiTable::setColumnWidthInChar( int col, float w )
{
    const float wdt = w * body_->fontWidth();
    setColumnWidth( col, mNINT32(wdt) );
}


void uiTable::setTopMargin( int h )
{
    QHeaderView* header = body_->horizontalHeader();
    if ( !header ) return;

    header->setVisible( h > 0 );
}


void uiTable::setRowHeight( int row, int h )
{
    if ( row >= 0 )
	body_->setRowHeight( row, h );
    else if ( row == -1 )
    {
	for ( int idx=0; idx<nrRows(); idx++ )
	    body_->setRowHeight( idx, h );
    }
}


void uiTable::setRowHeightInChar( int row, float h )
{
    float hgt = h * body_->fontHeight();
    setRowHeight( row, mNINT32(hgt) );
}


void uiTable::insertRows( int row, int cnt )
{
    mBlockCmdRec;
    for ( int idx=0; idx<cnt; idx++ )
	body_->insertRow( row );

    updateRow( row );
}


void uiTable::insertColumns( int col, int cnt )
{
    mBlockCmdRec;
    for ( int idx=0; idx<cnt; idx++ )
	body_->insertColumn( col );

    updateCol( col );
}


void uiTable::removeRCs( const TypeSet<int>& idxs, bool col )
{
    if ( idxs.size() < 1 ) return;
    for ( int idx=idxs.size()-1; idx>=0; idx-- )
	col ? removeColumn( idxs[idx] ) : removeRow( idxs[idx] );
}


void uiTable::removeRow( int row )
{
    mBlockCmdRec;
    for ( int col=0; col<nrCols(); col++ )
	clearCellObject( RowCol(row,col) );
    body_->removeRow( row );
    updateRow(row);
}


void uiTable::removeColumn( int col )
{
    mBlockCmdRec;
    for ( int row=0; row<nrRows(); row++ )
	clearCellObject( RowCol(row,col) );
    body_->removeColumn( col );
    updateCol(col);
}


void uiTable::removeRows( const TypeSet<int>& idxs )
{ removeRCs( idxs, false ); }

void uiTable::removeColumns( const TypeSet<int>& idxs )
{ removeRCs( idxs, true ); }

void uiTable::setNrRows( int nr )
{
    mBlockCmdRec;
    body_->setNrLines( nr );
    updateRow(0);
}

void uiTable::setNrCols( int nr )
{
    mBlockCmdRec;
    body_->setColumnCount( nr );
    updateCol(0);
}

int uiTable::nrRows() const		{ return body_->rowCount(); }
int uiTable::nrCols() const		{ return body_->columnCount(); }


void uiTable::setPrefHeightInRows( int nrrows )
{
    const QList<QScreen*> screens = QGuiApplication::screens();
    if ( screens.isEmpty() )
	return;
    const QRect geom = screens[0]->availableVirtualGeometry();
    body_->setPrefHeightInRows( nrrows, int(geom.height()*0.9) );
}


void uiTable::setPrefWidthInChars( int nrchars )
{
    const QList<QScreen*> screens = QGuiApplication::screens();
    if ( screens.isEmpty() )
	return;
    const QRect geom = screens[0]->availableVirtualGeometry();
    body_->setPrefWidthInChars( nrchars, int(geom.width()*0.9) );
}


void uiTable::clearCell( const RowCol& rc )
{
    mBlockCmdRec;
    QTableWidgetItem* itm = body_->takeItem( rc.row(), rc.col() );
    delete itm;
}


void uiTable::setCurrentCell( const RowCol& rc, bool noselection )
{
    mBlockCmdRec;
    if ( noselection )
	body_->setCurrentCell( rc.row(), rc.col(),
			       QItemSelectionModel::NoUpdate );
    else
	body_->setCurrentCell( rc.row(), rc.col() );
}


const char* uiTable::text( const RowCol& rc ) const
{
    const uiObject* cellobj = getCellObject( rc );
    if ( cellobj )
    {
	mDynamicCastGet(const uiComboBox*,cb,cellobj)
	if ( !cb )
	{
	    pErrMsg("TODO: unknown table cell obj: add it!");
	    return cellobj->name().buf();
	}
	return cb->text();
    }

    mDeclStaticString( ret );
    QTableWidgetItem* itm = body_->item( rc.row(), rc.col() );
    ret = itm ? itm->text() : "";
    return ret;
}


void uiTable::setText( const RowCol& rc, const char* txt )
{
    setText( rc, toUiString(txt) );
}


void uiTable::setText( const RowCol& rc, const OD::String& txt )
{
    setText( rc, toUiString(txt) );
}


void uiTable::setText( const RowCol& rc, const uiString& txt )
{
    mBlockCmdRec;
    uiObject* cellobj = getCellObject( rc );
    if ( !cellobj )
    {
	QTableWidgetItem* itm = body_->getItem( rc );
	itm->setText( toQString(txt) );
    }
    else
    {
	mDynamicCastGet(uiComboBox*,cb,cellobj)
	if ( !cb )
	    pErrMsg("TODO: unknown table cell obj: add it!");
	else
	    cb->setText( toString(txt) );
    }
}


static QAbstractItemView::EditTriggers triggers_ro =
				QAbstractItemView::NoEditTriggers;
static QAbstractItemView::EditTriggers triggers =
				QAbstractItemView::EditKeyPressed |
				QAbstractItemView::AnyKeyPressed |
				QAbstractItemView::DoubleClicked;

void uiTable::setTableReadOnly( bool yn )
{
    body_->setEditTriggers( yn ? triggers_ro : triggers );
    istablereadonly_ = yn;
}


bool uiTable::isTableReadOnly() const
{ return istablereadonly_; }


static Qt::ItemFlags std_flags = Qt::ItemIsSelectable |
			     Qt::ItemIsEditable | Qt::ItemIsEnabled;
static Qt::ItemFlags std_flags_ro = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

void uiTable::setColumnReadOnly( int col, bool yn )
{
    mBlockCmdRec;
    for ( int row=0; row<nrRows(); row++ )
    {
	QTableWidgetItem* itm = body_->getItem( RowCol(row,col),
							  true );
	if ( itm ) itm->setFlags( yn ? std_flags_ro : std_flags );
    }
}


void uiTable::setCellReadOnly( const RowCol& rc, bool yn )
{
    mBlockCmdRec;
    QTableWidgetItem* itm = body_->item( rc.row(), rc.col() );
    if ( itm ) itm->setFlags( yn ? std_flags_ro : std_flags );
}


bool uiTable::isCellReadOnly( const RowCol& rc ) const
{
    QTableWidgetItem* itm = body_->item( rc.row(), rc.col() );
    return itm && !itm->flags().testFlag( Qt::ItemIsEditable );
}


void uiTable::setRowReadOnly( int row, bool yn )
{
    mBlockCmdRec;
    for ( int col=0; col<nrCols(); col++ )
    {
	QTableWidgetItem* itm = body_->getItem( RowCol(row,col),
							  true );
	if ( itm ) itm->setFlags( yn ? std_flags_ro : std_flags );
    }
}


bool uiTable::isColumnReadOnly( int colidx ) const
{
    const int nrrows = nrRows();
    if ( nrrows < 1 )
	return false;

    for ( int irow=0; irow<nrrows; irow++ )
    {
	QTableWidgetItem* itm = body_->item( irow, colidx );
	if ( itm && itm->flags().testFlag(Qt::ItemIsEditable) )
	    return false;
    }

    return true;
}


bool uiTable::isRowReadOnly( int rowidx ) const
{
    const int nrcols = nrCols();
    if ( nrcols < 1 )
	return false;

    for ( int icol=0; icol<nrcols; icol++ )
    {
	QTableWidgetItem* itm = body_->item( rowidx, icol );
	if ( itm && !itm->flags().testFlag(Qt::ItemIsEditable) )
	    return false;
    }

    return true;
}


void uiTable::hideColumn( int colidx, bool yn )
{
    mBlockCmdRec;
    if ( yn )
	body_->hideColumn( colidx );
    else
	body_->showColumn( colidx );
}


void uiTable::hideRow( int colidx, bool yn )
{
    mBlockCmdRec;
    if ( yn )
	body_->hideRow( colidx );
    else
	body_->showRow( colidx );
}


bool uiTable::isColumnHidden( int col ) const
{ return body_->isColumnHidden(col); }

bool uiTable::isRowHidden( int row ) const
{ return body_->isRowHidden(row); }


bool uiTable::isTopHeaderHidden() const
{ return !body_->horizontalHeader()->isVisible(); }

bool uiTable::isLeftHeaderHidden() const
{ return !body_->verticalHeader()->isVisible(); }

void uiTable::setTopHeaderHidden( bool yn )
{ body_->horizontalHeader()->setVisible( !yn ); }

void uiTable::setLeftHeaderHidden( bool yn )
{ body_->verticalHeader()->setVisible( !yn ); }


void uiTable::resizeHeaderToContents( bool hor )
{
    QHeaderView* hdr = hor ? body_->horizontalHeader()
			   : body_->verticalHeader();
    if ( hdr ) hdr->resizeSections( QHeaderView::ResizeToContents );
}


void uiTable::resizeColumnToContents( int col )
{ body_->resizeColumnToContents( col ); }

void uiTable::resizeColumnsToContents()
{ body_->resizeColumnsToContents(); }

void uiTable::resizeRowToContents( int row )
{ body_->resizeRowToContents( row ); }

void uiTable::resizeRowsToContents()
{ body_->resizeRowsToContents(); }


void uiTable::setColumnResizeMode( ResizeMode mode )
{
    QHeaderView* header = body_->horizontalHeader();
    setResizeMode( header, (QHeaderView::ResizeMode)(int)mode );
}


void uiTable::setRowResizeMode( ResizeMode mode )
{
    QHeaderView* header = body_->verticalHeader();
    setResizeMode( header, (QHeaderView::ResizeMode)(int)mode );
}


void uiTable::setColumnStretchable( int col, bool yn )
{
    QHeaderView* header = body_->horizontalHeader();
    QHeaderView::ResizeMode mode = yn ? QHeaderView::Stretch
				      : QHeaderView::Interactive ;
    setResizeMode( header, mode, header->logicalIndex(col) );
}


void uiTable::setRowStretchable( int row, bool yn )
{
    QHeaderView* header = body_->verticalHeader();
    QHeaderView::ResizeMode mode = yn ? QHeaderView::Stretch
				      : QHeaderView::Interactive;
    setResizeMode( header, mode, header->logicalIndex(row) );
}


bool uiTable::isColumnStretchable( int col ) const
{  pErrMsg( "Not impl yet" ); return false; }

bool uiTable::isRowStretchable( int row ) const
{  pErrMsg( "Not impl yet" ); return false; }


void uiTable::setPixmap( const RowCol& rc, const uiPixmap& pm )
{
    mBlockCmdRec;
    QTableWidgetItem* itm = body_->getItem( rc );
    if ( itm ) itm->setIcon( *pm.qpixmap() );
}


class uiTableCellColSelector : public uiColorInput
{
public:

uiTableCellColSelector( const Setup& su, const RowCol& rc, uiTable* tbl )
    : uiColorInput(0,su,BufferString("CTSel ",rc.toString()))
    , rowcol_(rc)
    , tbl_(tbl)
{
    mAttachCB( colorChanged, uiTableCellColSelector::colChgCB );
}

~uiTableCellColSelector()
{
    detachAllNotifiers();
}

void colChgCB( CallBacker* )
{
    tbl_->colorSelectionChanged.trigger( rowcol_ );
}

    const RowCol	rowcol_;
    uiTable*		tbl_;

};


void uiTable::setColorSelectionCell( const RowCol& rc, bool withtr )
{
    uiGroup* cellgrp = getCellGroup( rc );
    mDynamicCastGet( uiTableCellColSelector*, colsel, cellgrp );
    if ( colsel )
	return;

    uiColorInput::Setup cisu( Color::White(),
		withtr ? uiColorInput::Setup::InSelector
		       : uiColorInput::Setup::None );
    cisu.withdesc( false );
    colsel = new uiTableCellColSelector( cisu, rc, this );
    setCellGroup( rc, colsel );
}


void uiTable::setCellColor( const RowCol& rc, const Color& col )
{
    uiGroup* cellgrp = getCellGroup( rc );
    mDynamicCastGet( uiTableCellColSelector*, colsel, cellgrp );
    if ( colsel )
	colsel->setColor( col );
    else
    {
	QTableWidgetItem* itm = body_->getItem( rc );
	if ( itm )
	    itm->setBackground( QBrush(getQCol(col)) );
    }
}


Color uiTable::getCellColor( const RowCol& rc ) const
{
    uiGroup* cellgrp = getCellGroup( rc );
    mDynamicCastGet( uiTableCellColSelector*, colsel, cellgrp );
    if ( colsel )
	return colsel->color();
    else
    {
	const QTableWidgetItem* itm = body_->getItem( rc, false );
	return itm ? getODCol( itm->background().color() ) : Color::White();
    }
}


void uiTable::setRowBackground( int rowidx, const Color& col )
{
    const int nrcols = nrCols();
    const QBrush qbrush( getQCol(col) );
    for ( int icol=0; icol<nrcols; icol++ )
    {
	QTableWidgetItem* itm = body_->item( rowidx, icol );
	if ( itm )
	    itm->setBackground( qbrush );
    }
}


void uiTable::setColumnBackground( int colidx, const Color& col )
{
    const int nrrows = nrRows();
    const QBrush qbrush( getQCol(col) );
    for ( int irow=0; irow<nrrows; irow++ )
    {
	QTableWidgetItem* itm = body_->item( irow, colidx );
	if ( itm )
	    itm->setBackground( qbrush );
    }
}


void uiTable::setHeaderBackground( int idx, const Color& col, bool isrow )
{
    QTableWidgetItem* itm = isrow ? body_->verticalHeaderItem( idx )
				  : body_->horizontalHeaderItem( idx);
    if ( itm )
	itm->setBackground( QBrush(getQCol(col)) );
}


Color uiTable::getHeaderBackground( int idx, bool isrow ) const
{
    const QTableWidgetItem* itm = isrow ? body_->verticalHeaderItem( idx )
					: body_->horizontalHeaderItem( idx );
    return itm ? getODCol( itm->background().color() ) : Color::White();
}


const char* uiTable::rowLabel( int row ) const
{
    QTableWidgetItem* itm = body_->verticalHeaderItem( row );
    if ( !itm )
	return 0;

    mDeclStaticString( ret );
    ret = itm->text();
    return ret;
}


void uiTable::setRowLabel( int row, const uiString& label )
{
    QTableWidgetItem& itm = body_->getRCItem( row, true );
    mGetQStr( qstr, label );
    itm.setText( qstr );
    itm.setToolTip( qstr );
}


void uiTable::setTopLeftCornerLabel( const uiString& txt )
{
    if ( !cornerlabel_ )
    {
	cornerlabel_ = new QLabel( body_ );
	cornerlabel_->move( 2, 2 );
    }
    cornerlabel_->setText( toQString(txt) );
    cornerlabel_->adjustSize();
    cornerlabel_->show();
}


void uiTable::setRowToolTip( int row, const uiString& tt )
{
    body_->getRCItem(row,true).setToolTip( toQString(tt) );
}


void uiTable::setLabelBGColor( int rc, Color c, bool isrow )
{
    QTableWidgetItem& qw = body_->getRCItem( rc, isrow );
    qw.setBackground( QBrush(getQCol(c)) );
}


void uiTable::setRowLabels( const char** labels )
{
    BufferStringSet lbls( labels );
    setRowLabels( lbls.getUiStringSet() );
}


void uiTable::setRowLabels( const BufferStringSet& labels )
{
    body_->setNrLines( labels.size() );
    for ( int i=0; i<labels.size(); i++ )
	setRowLabel( i, toUiString(*labels[i]) );
}


void uiTable::setRowLabels( const uiStringSet& lblset )
{
    body_->setNrLines( lblset.size() );
    for ( int i=0; i<lblset.size(); i++ )
	setRowLabel( i, lblset.get(i) );
}


const char* uiTable::columnLabel( int col ) const
{
    QTableWidgetItem* itm = body_->horizontalHeaderItem( col );
    if ( !itm )
	return 0;

    mDeclStaticString( ret );
    ret = itm->text();
    return ret;
}


void uiTable::setColumnLabel( int col, const uiString& label )
{
    QTableWidgetItem& itm = body_->getRCItem( col, false );
    mGetQStr( qstr, label );
    itm.setText( qstr );
    itm.setToolTip( qstr );
}


void uiTable::setColumnToolTip( int col, const uiString& tt )
{
    QTableWidgetItem& itm = body_->getRCItem( col, false );
    itm.setToolTip( toQString(tt) );
}


void uiTable::setColumnLabels( const BufferStringSet& labels )
{
    body_->setColumnCount( labels.size() );

    for ( int i=0; i<labels.size(); i++ )
	setColumnLabel( i, toUiString(labels[i]->buf()) );
}


void uiTable::setColumnLabels( const uiStringSet& lblset )
{
    body_->setColumnCount( lblset.size() );

    for ( int i=0; i<lblset.size(); i++ )
	setColumnLabel( i, lblset.get(i) );
}


void uiTable::setLabelAlignment( OD::Alignment::HPos hal, bool col )
{
    QHeaderView* hdr = col ? body_->horizontalHeader()
				     : body_->verticalHeader();
    if ( hdr )
    {
	OD::Alignment al( hal, OD::Alignment::VCenter );
	hdr->setDefaultAlignment( (Qt::Alignment)al.uiValue() );
    }
}


void uiTable::setCellToolTip( const RowCol& rc, const uiString& tt )
{
    uiObject* cellobj = getCellObject( rc );
    if ( !cellobj )
    {
	QTableWidgetItem* itm = body_->getItem( rc );
	itm->setToolTip( toQString(tt) );
    }
    else
	cellobj->setToolTip( tt );
}


int uiTable::getIntValue( const RowCol& rc ) const
{ const char* str = text(rc); return toInt( str, mUdf(int) ); }
od_int64 uiTable::getInt64Value( const RowCol& rc ) const
{ const char* str = text(rc); return toInt64( str, mUdf(int) ); }
float uiTable::getFValue( const RowCol& rc ) const
{ const char* str = text(rc); return toFloat( str, mUdf(float) ); }
double uiTable::getDValue( const RowCol& rc ) const
{ const char* str = text(rc); return toDouble( str, mUdf(double) ); }


void uiTable::setValue( const RowCol& rc, int val )
{ setText( rc, toString(val) ); }
void uiTable::setValue( const RowCol& rc, od_int64 val )
{ setText( rc, toString(val) ); }
void uiTable::setValue( const RowCol& rc, float val, int nrdec )
{ setText( rc, mIsUdf(val) ? ""
			: (nrdec < 0 ? toString(val) : toString(val,nrdec) )); }
void uiTable::setValue( const RowCol& rc, double val, int nrdec )
{ setText( rc, mIsUdf(val) ? ""
			: (nrdec < 0 ? toString(val) : toString(val,nrdec) )); }


void uiTable::setSelectionMode( SelectionMode m )
{
    switch ( m )
    {
	case Single:
	    body_->setSelectionMode( QAbstractItemView::SingleSelection );
	    break;
	case Multi:
	    body_->setSelectionMode( QAbstractItemView::ExtendedSelection );
	    break;
	case SingleRow:
	    setSelectionBehavior( uiTable::SelectRows );
	    break;
	default:
	    body_->setSelectionMode( QAbstractItemView::NoSelection );
	    break;
    }
}


void uiTable::setSelectionBehavior( SelectionBehavior sb )
{
    const int sbi = (int)sb;
    body_->setSelectionBehavior( (QAbstractItemView::SelectionBehavior)sbi );
}


void uiTable::editCell( const RowCol& rc, bool replace )
{
    mBlockCmdRec;
    QTableWidgetItem* itm = body_->item( rc.row(), rc.col() );
    body_->editItem( itm );
}


void uiTable::popupMenu( CallBacker* )
{
    const int xcursorpos = QCursor::pos().x();
    const int ycursorpos = QCursor::pos().y();

    if ( uiVirtualKeyboard::isVirtualKeyboardActive() )
	return;

    if ( (!setup_.rowgrow_ && !setup_.colgrow_ && !setup_.enablecopytext_) ||
	 setup_.rightclickdisabled_ )
    {
	popupVirtualKeyboard( xcursorpos, ycursorpos );
	return;
    }

    uiMenu* mnu = new uiMenu( parent(), uiStrings::sAction() );
    uiString itmtxt;
    const RowCol cur = notifiedCell();
    if ( setup_.removeselallowed_ )
	getSelected();

    const int nrrows = nrRows();
    const int nrcols = nrCols();
    if ( (nrrows*nrcols)!=0 && cur.row()==-1 && cur.col()==-1 )
	return;
    uiString beforestr;
    uiString afterstr;
    int inscolbef = 0;
    int delcol = 0;
    int delcols = 0;
    int inscolaft = 0;
    if ( setup_.colgrow_ )
    {
	if ( setup_.insertcolallowed_ )
	{
	    if ( nrcols==0 )
	    {
		itmtxt = uiStrings::phrInsert( toUiString(setup_.coldesc_) );
		inscolaft = mnu->insertAction( new uiAction(itmtxt), 2 );
	    }
	    else
	    {
		beforestr = tr("%1 before").arg( toUiString(setup_.coldesc_) );
		afterstr = tr("%1 after").arg( toUiString(setup_.coldesc_) );
		itmtxt = uiStrings::phrInsert( beforestr );
		inscolbef = mnu->insertAction( new uiAction(itmtxt), 0 );
		itmtxt = uiStrings::phrInsert( afterstr );
		inscolaft = mnu->insertAction( new uiAction(itmtxt), 2 );
	    }
	}

	if ( nrcols>0 && setup_.removecolallowed_ && notifcols_.size() < 2 )
	{
	    itmtxt = uiStrings::phrRemove(toUiString(setup_.coldesc_));
	    delcol = mnu->insertAction( new uiAction(itmtxt), 4 );
	}

	if ( notifcols_.size() > 1 )
	{
	    itmtxt = uiStrings::phrRemoveSelected(toUiString("%2s")
		     .arg(setup_.coldesc_));
	    delcols = mnu->insertAction( new uiAction(itmtxt), 6 );
	}
    }

    int insrowbef = 0;
    int delrow = 0;
    int delrows = 0;
    int insrowaft = 0;
    if ( setup_.rowgrow_ )
    {
	if ( setup_.insertrowallowed_ )
	{
	    if ( nrrows==0 )
	    {
		itmtxt = uiStrings::phrInsert( toUiString(setup_.rowdesc_) );
		insrowaft = mnu->insertAction( new uiAction(itmtxt), 3 );
	    }
	    else
	    {
		beforestr = tr("%1 before").arg( toUiString(setup_.rowdesc_) );
		afterstr = tr("%1 after").arg( toUiString(setup_.rowdesc_) );
		itmtxt =  uiStrings::phrInsert( beforestr );
		insrowbef = mnu->insertAction( new uiAction(itmtxt), 1 );
		itmtxt =  uiStrings::phrInsert( afterstr );
		insrowaft = mnu->insertAction( new uiAction(itmtxt), 3 );
	    }
	}

	if ( nrrows>0 && setup_.removerowallowed_ && notifrows_.size() < 2 )
	{
	    itmtxt = toUiString("%1 %2").arg(uiStrings::sRemove()).
					    arg(setup_.rowdesc_);
	    delrow = mnu->insertAction( new uiAction(itmtxt), 5 );
	}

	if ( notifrows_.size() > 1 )
	{
	    itmtxt = uiStrings::phrRemoveSelected(toUiString("%2s")
		     .arg(setup_.rowdesc_));
	    delrows = mnu->insertAction( new uiAction(itmtxt), 7 );
	}
    }

    int cptxt = 0;
    if ( isTableReadOnly() && setup_.enablecopytext_ )
    {
	itmtxt = uiStrings::phrCopy( uiStrings::sText().toLower() );
	cptxt = mnu->insertAction( new uiAction(itmtxt), 8 );
    }

    int virkeyboardid = mUdf(int);
    if ( needOfVirtualKeyboard() )
    {
	mnu->insertSeparator();
	itmtxt = tr("Virtual Keyboard");
	virkeyboardid = mnu->insertAction( new uiAction(itmtxt), 100 );
    }

    int ret = mnu->exec();
    if ( ret<0 ) return;

    if ( ret == inscolbef || ret == inscolaft )
    {
	const int offset = (ret == inscolbef) ? 0 : 1;
	newcell_ = RowCol( cur.row(), cur.col()+offset );
	insertColumns( newcell_, 1 );

	if ( !setup_.defcollbl_ )
	    setColumnLabel( newcell_, toUiString(newcell_.col()) );

	colInserted.trigger();
    }
    else if ( ret == delcol )
    {
	removeColumn( cur.col() );
	colDeleted.trigger();
    }
    else if ( ret == delcols )
    {
	seliscols_ = true;
	removeColumns( notifcols_ );
	selectionDeleted.trigger();
    }
    else if ( ret == insrowbef || ret == insrowaft  )
    {
	const int offset = (ret == insrowbef) ? 0 : 1;
	newcell_ = RowCol( cur.row()+offset, cur.col() );
	insertRows( newcell_, 1 );

	if ( !setup_.defrowlbl_ )
	    setRowLabel( newcell_, toUiString(newcell_.row()) );

	rowInserted.trigger();
    }
    else if ( ret == delrow )
    {
	removeRow( cur.row() );
	rowDeleted.trigger();
    }
    else if ( ret == delrows )
    {
	seliscols_ = false;
	removeRows( notifrows_ );
	selectionDeleted.trigger();
    }
    else if ( ret == virkeyboardid )
    {
	popupVirtualKeyboard( xcursorpos, ycursorpos );
	return;
    }
    else if ( ret == cptxt )
    {
	const char* str = text( cur );
	if ( !str || !*str )
	    return;

	uiClipboard::setText( toUiString(str) );
    }

    setCurrentCell( newcell_ );
//    updateCellSizes();
}


void uiTable::geometrySet_( CallBacker* cb )
{
//    if ( !mainwin() ||  mainwin()->poppedUp() ) return;

//    mCBCapsuleUnpack(uiRect&,sz,cb);
//    const uiSize size = sz.getPixelSize();

//    updateCellSizes( &size );
}


void uiTable::updateCellSizes( const uiSize* size )
{
    if ( size ) lastsz = *size;
    else	size = &lastsz;

    if ( !setup_.manualresize_ && body_->layoutItem()->inited() )
    {
	if ( !setup_.fillcol_ )
	    for ( int idx=0; idx < nrCols(); idx++ )
		setColumnStretchable( idx, true );

	if ( !setup_.fillrow_ )
	    for ( int idx=0; idx < nrRows(); idx++ )
		setRowStretchable( idx, true );
    }

    int nc = nrCols();
    if ( nc && setup_.fillrow_ )
    {
	int wdth = size->hNrPics();
	int availwdt = wdth - body_->verticalHeader()->frameSize().width()
			 - 2*body_->frameWidth();

	int colwdt = availwdt / nc;

	const int minwdt = (int)(setup_.mincolwdt_ * (float)font()->avgWidth());
	const int maxwdt = (int)(setup_.maxcolwdt_ * (float)font()->avgWidth());

	if ( colwdt < minwdt ) colwdt = minwdt;
	if ( colwdt > maxwdt ) colwdt = maxwdt;

	for ( int idx=0; idx < nc; idx ++ )
	{
	    if ( idx < nc-1 )
		setColumnWidth( idx, colwdt );
	    else
	    {
		int wdt = availwdt;
		if ( wdt < minwdt )	wdt = minwdt;
		if ( wdt > maxwdt )	wdt = maxwdt;

		setColumnWidth( idx, wdt );
	    }
	    availwdt -= colwdt;
	}
    }

    int nr = nrRows();
    if ( nr && setup_.fillcol_ )
    {
	int hght = size->vNrPics();
	int availhgt = hght - body_->horizontalHeader()->frameSize().height()
			 - 2*body_->frameWidth();

	int rowhgt =  availhgt / nr;
	const float fonthgt = (float)font()->height();

	const int minhgt = (int)(setup_.minrowhgt_ * fonthgt);
	const int maxhgt = (int)(setup_.maxrowhgt_ * fonthgt);

	if ( rowhgt < minhgt ) rowhgt = minhgt;
	if ( rowhgt > maxhgt ) rowhgt = maxhgt;

	for ( int idx=0; idx < nr; idx ++ )
	{
	    if ( idx < nr-1 )
		setRowHeight( idx, rowhgt );
	    else
	    {
		int hgt = availhgt;
		if ( hgt < minhgt ) hgt = minhgt;
		if ( hgt > maxhgt ) hgt = maxhgt;

		setRowHeight( idx, hgt );
	    }
	    availhgt -= rowhgt;
	}
    }
}


void uiTable::clearTable()
{
    mBlockCmdRec;
    body_->clearTable();
}


void uiTable::removeAllSelections()
{
    mBlockCmdRec;
    body_->clearSelection();
}


bool uiTable::isSelected ( const RowCol& rc ) const
{
    const QItemSelectionModel* selmodel = body_->selectionModel();
    const QAbstractItemModel* model = selmodel ? selmodel->model()
							 : 0;
    if ( !model )
	return false;

    QModelIndex idx = body_->rootIndex();
    idx = model->index( rc.row(), rc.col(), idx );
    return selmodel->isSelected( idx );
}


bool uiTable::isRowSelected( int row ) const
{
    QItemSelectionModel* model = body_->selectionModel();
    return model ? model->isRowSelected( row, body_->rootIndex() ) : false;
}


bool uiTable::isColumnSelected( int col ) const
{
    QItemSelectionModel* model = body_->selectionModel();
    return model ? model->isColumnSelected( col, body_->rootIndex() ) : false;
}


bool uiTable::getSelected()
{
    notifrows_.setEmpty();
    notifcols_.setEmpty();
    return getSelectedRows( notifrows_ ) && getSelectedCols( notifcols_ ) &&
	   getSelectedCells( notifcells_ );
}


bool uiTable::getSelectedCells( TypeSet<RowCol>& sel ) const
{
    sel.erase();
    for ( int idx=0; idx<selectedRanges().size(); idx++ )
    {
	const SelectionRange* selrg = selectedRanges()[idx];
	const int firstrow = selrg->firstrow_;
	const int lastrow = selrg->lastrow_;
	const int firstcol = selrg->firstcol_;
	const int lastcol = selrg->lastcol_;
	for ( int row = firstrow; row<=lastrow; row++ )
	{
	    for ( int col=firstcol; col<=lastcol; col++ )
		sel += RowCol( row, col );
	}
    }

    return !sel.isEmpty();
}


bool uiTable::getSelectedRows( TypeSet<int>& sel ) const
{
    sel.setEmpty();
    for ( int idx=0; idx<selectedRanges().size(); idx++ )
    {
	const int firstrow = selectedRanges()[idx]->firstrow_;
	const int lastrow = selectedRanges()[idx]->lastrow_;
	for ( int row = firstrow; row<=lastrow; row++ )
	    sel += row;
    }

    return !sel.isEmpty();
}


bool uiTable::getSelectedCols( TypeSet<int>& sel ) const
{
    sel.setEmpty();
    for ( int idx=0; idx<selectedRanges().size(); idx++ )
    {
	const int firstcol = selectedRanges()[idx]->firstcol_;
	const int lastcol = selectedRanges()[idx]->lastcol_;
	for ( int col = firstcol; col<=lastcol; col++ )
	    sel += col;
    }

    return !sel.isEmpty();
}


int uiTable::currentRow() const
{ return body_->currentRow(); }

int uiTable::currentCol() const
{ return body_->currentColumn(); }


void uiTable::setSelected( const RowCol& rc, bool yn )
{
    mBlockCmdRec;
    QTableWidgetItem* itm = body_->item( rc.row(), rc.col() );
    if ( itm )
	itm->setSelected( yn );
}


void  uiTable::selectRow( int row )
{
    mBlockCmdRec;
    return body_->selectRow( row );
}


void  uiTable::selectColumn( int col )
{
    mBlockCmdRec;
    return body_->selectColumn( col );
}


void uiTable::ensureCellVisible( const RowCol& rc )
{
    QTableWidgetItem* itm = body_->item( rc.row(), rc.col() );
    body_->scrollToItem( itm );
}


void uiTable::setCellGroup( const RowCol& rc, uiGroup* grp )
{
    mBlockCmdRec;
    clearCellObject( rc );
    body_->setCellObject( rc, grp ? grp->attachObj() : 0 );
}


uiGroup* uiTable::getCellGroup( const RowCol& rc ) const
{
    mDynamicCastGet(uiGroupObj*,grpobj,getCellObject(rc));
    return grpobj ? grpobj->group() : 0;
}


RowCol uiTable::getCell( uiGroup* grp )
{ return grp ? getCell( grp->attachObj() ) : RowCol(-1,-1); }


void uiTable::setCellObject( const RowCol& rc, uiObject* obj )
{
    mBlockCmdRec;
    clearCellObject( rc );
    body_->setCellObject( rc, obj );

    mDynamicCastGet(uiComboBox*,cb,obj)
    if ( cb )
	cb->selectionChanged.notify( mCB(this,uiTable,cellObjChangedCB) );
}


uiObject* uiTable::getCellObject( const RowCol& rc ) const
{ return body_->getCellObject( rc ); }


void uiTable::clearCellObject( const RowCol& rc )
{
    mBlockCmdRec;

    uiObject* obj = getCellObject( rc );
    mDynamicCastGet(uiComboBox*,cb,obj)
    if ( cb )
	cb->selectionChanged.remove( mCB(this,uiTable,cellObjChangedCB) );

    body_->clearCellObject( rc );
}


RowCol uiTable::getCell( uiObject* obj )
{ return body_->getCell( obj ); }


void uiTable::cellObjChangedCB( CallBacker* cb )
{
    mDynamicCastGet(uiObject*,obj,cb)
    if ( !obj ) return;

    notifcell_ = getCell( obj );
    valueChanged.trigger();
}


void uiTable::setCellChecked( const RowCol& rc, bool yn )
{
    QTableWidgetItem* itm = body_->getItem( rc );
    itm->setCheckState( yn ? Qt::Checked : Qt::Unchecked );
}


bool uiTable::isCellChecked( const RowCol& rc ) const
{
    QTableWidgetItem* itm = body_->getItem( rc, false );
    return itm && itm->checkState()==Qt::Checked;
}


const ObjectSet<uiTable::SelectionRange>& uiTable::selectedRanges() const
{
    deepErase( selranges_ );
    QItemSelectionModel* mdl = body_->selectionModel();
    if ( !mdl ) return selranges_;

    const QList<QItemSelectionRange> qranges = mdl->selection();
    for ( int idx=0; idx<qranges.count(); idx++ )
    {
	uiTable::SelectionRange* rg = new uiTable::SelectionRange;
	rg->firstrow_ = qranges[idx].top();
	rg->lastrow_ = qranges[idx].bottom();
	rg->firstcol_ = qranges[idx].left();
	rg->lastcol_ = qranges[idx].right();
	selranges_ += rg;
    }

    return selranges_;
}


uiTable::SelectionBehavior uiTable::getSelBehavior() const
{
    return body_->getSelBehavior();
}


int uiTable::maxNrOfSelections() const
{
    return body_->maxNrOfSelections();
}


void uiTable::selectItems( const TypeSet<RowCol>& rcs, bool yn )
{
    mBlockCmdRec;
    removeAllSelections();
    for ( int idx=0; idx<rcs.size(); idx++ )
    {
	if ( mIsUdf(rcs[idx].row()) || mIsUdf(rcs[idx].col()) )
	    continue;
	QTableWidgetItem* itm = body_->item( rcs[idx].row(), rcs[idx].col() );
	if ( !itm || (yn && itm->isSelected()) || (!yn && !itm->isSelected()) )
	    continue;
	itm->setSelected( yn );
    }
}


bool uiTable::handleLongTabletPress()
{
    BufferString msg = "rightClicked ";
    msg += notifcell_.row(); msg += " "; msg += notifcell_.col();
    const int refnr = beginCmdRecEvent( msg );
    rightClicked.trigger();
    endCmdRecEvent( refnr, msg );
    return true;
}


bool uiTable::needOfVirtualKeyboard() const
{
    if ( isCellReadOnly(notifiedCell()) || getCellObject(notifiedCell()) )
	return false;
    if ( isRowReadOnly(notifiedCell().row()) )
	return false;
    if ( isColumnReadOnly(notifiedCell().col()) )
	return false;

    return hasFocus() && !isTableReadOnly();
}


void uiTable::popupVirtualKeyboard( int globalx, int globaly )
{
    if ( needOfVirtualKeyboard() )
    {
	uiVirtualKeyboard virkeyboard( *this, globalx, globaly );
	virkeyboard.show();
    }
}
