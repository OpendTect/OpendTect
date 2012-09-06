/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          12/02/2003
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uitable.cc,v 1.111 2012-09-06 11:52:57 cvsraman Exp $";


#include "uitable.h"

#include "uifont.h"
#include "uimenu.h"
#include "pixmap.h"
#include "uilabel.h"
#include "uiobjbody.h"
#include "uicombobox.h"
#include "uivirtualkeyboard.h"
#include "convert.h"
#include "bufstringset.h"
#include "i_layoutitem.h"
#include "i_qtable.h"

#include <QHeaderView>
#include <QMouseEvent>
#include <QCursor>
#include <QClipboard>
#include <QApplication>


class CellObject
{
    public:
			CellObject( mQtclass(QWidget*) qw, uiObject* obj,
				    const RowCol& rc )
			    : qwidget_(qw)
			    , object_(obj)
			    , rowcol_(rc)    {}
			~CellObject();

    uiObject*			object_;
    mQtclass(QWidget*)		qwidget_;
    RowCol			rowcol_;
};


CellObject::~CellObject()
{
    mDynamicCastGet(uiGroupObj*,grpobj,object_);

    if ( grpobj && grpobj->group() )
	delete grpobj->group();
    else
	delete object_;
}


class uiTableBody : public uiObjBodyImpl<uiTable,mQtclass(QTableWidget)>
{
public:
			uiTableBody(uiTable&,uiParent*,const char*,int,int);
			~uiTableBody();

    void		setNrLines(int);
    virtual int 	nrTxtLines() const;

    mQtclass(QTableWidgetItem*)	getItem(const RowCol&,bool createnew=true);

    void		clearCellObject(const RowCol&);
    uiObject*		getCellObject(const RowCol&) const;
    void		setCellObject(const RowCol&,uiObject*);
    RowCol		getCell(uiObject*);

    int			maxNrOfSelections() const;
    uiTable::SelectionBehavior getSelBehavior() const;


    mQtclass(QTableWidgetItem&)	getRCItem(int,bool isrow);

protected:
    virtual void	mouseReleaseEvent(mQtclass(QMouseEvent*));

    ObjectSet<CellObject> cellobjects_;

    BoolTypeSet		columnsreadonly;
    BoolTypeSet		rowsreadonly;

private:

    i_tableMessenger&	messenger_;

};


uiTableBody::uiTableBody( uiTable& hndl, uiParent* parnt, const char* nm,
			  int nrows, int ncols )
    : uiObjBodyImpl<uiTable,mQtclass(QTableWidget)>(hndl,parnt,nm)
    , messenger_ (*new i_tableMessenger(this,&hndl))
{
    if ( nrows >= 0 ) setNrLines( nrows );
    if ( ncols >= 0 ) setColumnCount( ncols );

// TODO: Causes tremendous performance delay in Qt 4.4.1;
//       For now use uiTable::resizeRowsToContents() in stead.
//    mQtclass(QHeaderView*) vhdr = verticalHeader();
//    vhdr->setResizeMode( mQtclass(QHeaderView)::ResizeToContents );

    mQtclass(QHeaderView*) hhdr = horizontalHeader();
    hhdr->setResizeMode( mQtclass(QHeaderView)::Stretch );

    setHorizontalScrollMode( mQtclass(QAbstractItemView)::ScrollPerPixel );

    setMouseTracking( true );
}


uiTableBody::~uiTableBody()
{
    deepErase( cellobjects_ );
    delete &messenger_;
}


mQtclass(QTableWidgetItem&) uiTableBody::getRCItem( int idx, bool isrow )
{
    mQtclass(QTableWidgetItem*) itm = isrow ? verticalHeaderItem( idx )
				  : horizontalHeaderItem( idx );
    if ( !itm )
    {
	itm = new mQtclass(QTableWidgetItem);
	if ( isrow )
	    setVerticalHeaderItem( idx, itm );
	else
	    setHorizontalHeaderItem( idx, itm );
    }

    return *itm;
}


void uiTableBody::mouseReleaseEvent( mQtclass(QMouseEvent*) ev )
{
    if ( !ev ) return;

    if ( ev->button() == mQtclass(Qt)::RightButton )
	handle_.buttonstate_ = OD::RightButton;
    else if ( ev->button() == mQtclass(Qt)::LeftButton )
	handle_.buttonstate_ = OD::LeftButton;
    else
	handle_.buttonstate_ = OD::NoButton;

    mQtclass(QAbstractItemView)::mouseReleaseEvent( ev );
    handle_.buttonstate_ = OD::NoButton;
}


void uiTableBody::setNrLines( int prefnrlines )
{ 
    setRowCount( prefnrlines );
    if ( !finalised() && prefnrlines > 0 )
    {
	mQtclass(QHeaderView*) vhdr = verticalHeader();
	const mQtclass(QSize) qsz = vhdr->sizeHint();
	const int rowh = rowHeight(0) + 1;
	const int prefh = rowh*prefnrlines + qsz.height();
	setPrefHeight( mMIN(prefh,200) );
    }
}


int uiTableBody::nrTxtLines() const
{ return rowCount()>=0 ? rowCount()+1 : 7; }


mQtclass(QTableWidgetItem*) uiTableBody::getItem( const RowCol& rc,
						  bool createnew )
{
    mQtclass(QTableWidgetItem*) itm = item( rc.row, rc.col );
    if ( !itm && createnew )
    {
	itm = new mQtclass(QTableWidgetItem);
	setItem( rc.row, rc.col, itm );
    }

    return itm;
}


void uiTableBody::setCellObject( const RowCol& rc, uiObject* obj )
{
    mQtclass(QWidget*) qw = obj->body()->qwidget();
    setCellWidget( rc.row, rc.col, qw );
    cellobjects_ += new CellObject( qw, obj, rc );
}


uiObject* uiTableBody::getCellObject( const RowCol& rc ) const
{
    mQtclass(QWidget*) qw = cellWidget( rc.row, rc.col );
    if ( !qw ) return 0;

    uiObject* obj = 0;
    for ( int idx=0; idx<cellobjects_.size(); idx++ )
    {
	if ( cellobjects_[idx]->qwidget_ == qw )
	{
	    obj = cellobjects_[idx]->object_;
	    break;
	}
    }

    return obj;
}


RowCol uiTableBody::getCell( uiObject* obj )
{
    for ( int idx=0; idx<cellobjects_.size(); idx++ )
    {
	if ( cellobjects_[idx]->object_ == obj )
	    return cellobjects_[idx]->rowcol_;
    }

    return RowCol( -1, -1 );
}


void uiTableBody::clearCellObject( const RowCol& rc )
{
    mQtclass(QWidget*) qw = cellWidget( rc.row, rc.col );
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

    setCellWidget( rc.row, rc.col, 0 );
    if ( co )
    {
	cellobjects_ -= co;
	delete co;
    }
}


uiTable::SelectionBehavior uiTableBody::getSelBehavior() const
{
    return (uiTable::SelectionBehavior) selectionBehavior();
}


int uiTableBody::maxNrOfSelections() const
{
    if ( selectionMode()==mQtclass(QAbstractItemView)::NoSelection )
	return 0;
    if ( selectionMode()==mQtclass(QAbstractItemView)::SingleSelection )
       	return 1;
    if ( getSelBehavior()==uiTable::SelectRows )
	return rowCount();
    if ( getSelBehavior()==uiTable::SelectColumns )
	return columnCount();

    return rowCount()*columnCount();
}



uiTable::uiTable( uiParent* p, const Setup& s, const char* nm )
    : uiObject(p,nm,mkbody(p,nm,s.size_.row,s.size_.col))
    , setup_(s)
    , buttonstate_(OD::NoButton)
    , valueChanged(this)
    , rightClicked(this)
    , leftClicked(this)
    , doubleClicked(this)
    , rowInserted(this)
    , colInserted(this)
    , rowDeleted(this)
    , colDeleted(this)
    , selectionChanged(this)
    , rowClicked(this)
    , columnClicked(this)
    , istablereadonly_(false)
{
    setFont( FontList().get(FontData::key(FontData::Fixed)) );
    rightClicked.notify( mCB(this,uiTable,popupMenu) );
    setGeometry.notify( mCB(this,uiTable,geometrySet_) );

    setStretch( 2, 2 );

    setSelectionMode( s.selmode_ );
    if ( s.defrowlbl_ )
	setDefaultRowLabels();
    if ( s.defcollbl_ )
	setDefaultColLabels();

    mQtclass(QHeaderView*) hhdr = body_->horizontalHeader();
    hhdr->setMinimumSectionSize( (int)(s.mincolwdt_*body_->fontWdt()) );
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
	BufferString lbl( setup_.rowdesc_ ); lbl += " ";
	lbl += ( idx + setup_.defrowstartidx_ );
	setRowLabel( idx, lbl );
    }
}


void uiTable::setDefaultColLabels()
{
    const int nrcols = nrCols();
    for ( int idx=0; idx<nrcols; idx++ )
    {
	BufferString lbl( setup_.coldesc_ ); lbl += " ";
	lbl += idx+1;
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
    mQtclass(QHeaderView*) header = body_->verticalHeader();
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
    const float wdt = w * body_->fontWdt();
    setColumnWidth( col, mNINT32(wdt) );
}


void uiTable::setTopMargin( int h )
{
    mQtclass(QHeaderView*) header = body_->horizontalHeader();
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
    float hgt = h * body_->fontHgt();
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

void uiTable::clearCell( const RowCol& rc )
{
    mBlockCmdRec;
    mQtclass(QTableWidgetItem*) itm = body_->takeItem( rc.row, rc.col );
    delete itm;
}


void uiTable::setCurrentCell( const RowCol& rc, bool noselection )
{
    mBlockCmdRec;
    if ( noselection )
	body_->setCurrentCell( rc.row, rc.col,
			       mQtclass(QItemSelectionModel)::NoUpdate );
    else
	body_->setCurrentCell( rc.row, rc.col );
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
	    return cellobj->name();
	}
	return cb->text();
    }

    static BufferString rettxt;
    mQtclass(QTableWidgetItem*) itm = body_->item( rc.row, rc.col );
    rettxt = itm ? itm->text().toAscii().data() : "";
    return rettxt;
}


void uiTable::setText( const RowCol& rc, const char* txt )
{
    mBlockCmdRec;
    uiObject* cellobj = getCellObject( rc );
    if ( !cellobj )
    {
	mQtclass(QTableWidgetItem*) itm = body_->getItem( rc );
	itm->setText( txt );
    }
    else
    {
	mDynamicCastGet(uiComboBox*,cb,cellobj)
	if ( !cb )
	    pErrMsg("TODO: unknown table cell obj: add it!");
	else
	    cb->setText( txt );
    }
}


static mQtclass(QAbstractItemView)::EditTriggers triggers_ro =
				mQtclass(QAbstractItemView)::NoEditTriggers;
static mQtclass(QAbstractItemView)::EditTriggers triggers =
				mQtclass(QAbstractItemView)::EditKeyPressed |
				mQtclass(QAbstractItemView)::AnyKeyPressed |
				mQtclass(QAbstractItemView)::DoubleClicked;

void uiTable::setTableReadOnly( bool yn )
{
    body_->setEditTriggers( yn ? triggers_ro : triggers );
    istablereadonly_ = yn;
}


bool uiTable::isTableReadOnly() const
{ return istablereadonly_; }


static mQtclass(Qt)::ItemFlags flags = mQtclass(Qt)::ItemIsSelectable |
		    mQtclass(Qt)::ItemIsEditable | mQtclass(Qt)::ItemIsEnabled;
static mQtclass(Qt)::ItemFlags flags_ro =
		   mQtclass(Qt)::ItemIsSelectable | mQtclass(Qt)::ItemIsEnabled;

void uiTable::setColumnReadOnly( int col, bool yn )
{
    for ( int row=0; row<nrRows(); row++ )
    {
	mQtclass(QTableWidgetItem*) itm = body_->getItem( RowCol(row,col),
							  true );
	if ( itm ) itm->setFlags( yn ? flags_ro : flags );
    }
}


void uiTable::setCellReadOnly( const RowCol& rc, bool yn )
{
    mQtclass(QTableWidgetItem*) itm = body_->item( rc.row, rc.col );
    if ( itm ) itm->setFlags( yn ? flags_ro : flags );
}


bool uiTable::isCellReadOnly( const RowCol& rc ) const
{
    mQtclass(QTableWidgetItem*) itm = body_->item( rc.row, rc.col );
    return itm && !itm->flags().testFlag( mQtclass(Qt)::ItemIsEditable );
}


void uiTable::setRowReadOnly( int row, bool yn )
{
    for ( int col=0; col<nrCols(); col++ )
    {
	mQtclass(QTableWidgetItem*) itm = body_->getItem( RowCol(row,col),
							  true );
	if ( itm ) itm->setFlags( yn ? flags_ro : flags );
    }
}


bool uiTable::isColumnReadOnly( int col ) const
{
    int nritems = 0;
    int nrro = 0;
    for ( int row=0; row<nrRows(); row++ )
    {
	mQtclass(QTableWidgetItem*) itm = body_->item( row, col );
	if ( itm )
	{
	    nritems ++;
	    if ( !itm->flags().testFlag(mQtclass(Qt)::ItemIsEditable) )
		nrro++;
	}
    }

    return nritems && nritems==nrro;
}


bool uiTable::isRowReadOnly( int row ) const
{
    int nritems = 0;
    int nrro = 0;
    for ( int col=0; col<nrCols(); col++ )
    {
	mQtclass(QTableWidgetItem*) itm = body_->item( row, col );
	if ( itm )
	{
	    nritems++;
	    if ( !itm->flags().testFlag(mQtclass(Qt)::ItemIsEditable) )
		nrro++;
	}
    }

    return nritems && nritems==nrro;
}


void uiTable::hideColumn( int col, bool yn )
{
    mBlockCmdRec;
    if ( yn ) body_->hideColumn( col );
    else body_->showColumn( col );
}

void uiTable::hideRow( int col, bool yn )
{
    mBlockCmdRec;
    if ( yn ) body_->hideRow( col );
    else body_->showRow( col );
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
    mQtclass(QHeaderView*) hdr = hor ? body_->horizontalHeader()
				     : body_->verticalHeader();
    if ( hdr ) hdr->resizeSections( mQtclass(QHeaderView)::ResizeToContents );
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
    mQtclass(QHeaderView*) header = body_->horizontalHeader();
    header->setResizeMode( (mQtclass(QHeaderView)::ResizeMode)(int)mode );
}


void uiTable::setRowResizeMode( ResizeMode mode )
{
    mQtclass(QHeaderView*) header = body_->verticalHeader();
    header->setResizeMode( (mQtclass(QHeaderView)::ResizeMode)(int)mode );
}


void uiTable::setColumnStretchable( int col, bool yn )
{
    mQtclass(QHeaderView*) header = body_->horizontalHeader();
    mQtclass(QHeaderView)::ResizeMode mode = yn ? mQtclass(QHeaderView)::Stretch
				          : mQtclass(QHeaderView)::Interactive ;
    header->setResizeMode( header->logicalIndex(col), mode );
}


void uiTable::setRowStretchable( int row, bool yn )
{
    mQtclass(QHeaderView*) header = body_->verticalHeader();
    mQtclass(QHeaderView)::ResizeMode mode = yn ? mQtclass(QHeaderView)::Stretch
				           : mQtclass(QHeaderView)::Interactive;
    header->setResizeMode( header->logicalIndex(row), mode );
}


bool uiTable::isColumnStretchable( int col ) const
{  pErrMsg( "Not impl yet" ); return false; }

bool uiTable::isRowStretchable( int row ) const
{  pErrMsg( "Not impl yet" ); return false; }


void uiTable::setPixmap( const RowCol& rc, const ioPixmap& pm )
{
    mBlockCmdRec;
    mQtclass(QTableWidgetItem*) itm = body_->getItem( rc );
    if ( itm ) itm->setIcon( *pm.qpixmap() );
}


void uiTable::setColor( const RowCol& rc, const Color& col )
{
    mBlockCmdRec;
    mQtclass(QColor) qcol( col.r(), col.g(), col.b() );
    mQtclass(QTableWidgetItem*) itm = body_->getItem( rc );
    if ( itm ) itm->setBackground( qcol );
    body_->setFocus();
}


Color uiTable::getColor( const RowCol& rc ) const
{
    mQtclass(QTableWidgetItem*) itm = body_->getItem( rc, false );
    if ( !itm ) return Color(255,255,255);

    const mQtclass(QColor) qcol = itm->background().color();
    return Color( qcol.red(), qcol.green(), qcol.blue() );
}


void uiTable::setHeaderBackground( int idx, const Color& col, bool isrow )
{
    mQtclass(QTableWidgetItem*) itm = isrow ? body_->verticalHeaderItem( idx )
				 	    : body_->horizontalHeaderItem( idx);
    if ( !itm )
	return;

    mQtclass(QColor) qcol( col.r(), col.g(), col.b() );
    itm->setBackground( qcol );
}


Color uiTable::getHeaderBackground( int idx, bool isrow ) const
{
    mQtclass(QTableWidgetItem*) itm = isrow ? body_->verticalHeaderItem( idx )
				  	    : body_->horizontalHeaderItem( idx);
    if ( !itm ) return Color(255,255,255);

    const mQtclass(QColor) qcol = itm->background().color();
    return Color( qcol.red(), qcol.green(), qcol.blue() );
}


const char* uiTable::rowLabel( int row ) const
{
    static BufferString ret;
    mQtclass(QTableWidgetItem*) itm = body_->verticalHeaderItem( row );
    if ( !itm )
	return 0;

    ret = itm->text().toAscii().data();
    return ret;
}


void uiTable::setRowLabel( int row, const char* label )
{
    mQtclass(QTableWidgetItem&) itm = body_->getRCItem( row, true );
    itm.setText( label );
    itm.setToolTip( label );
}


void uiTable::setRowToolTip( int row, const char* tt )
{
    body_->getRCItem(row,true).setToolTip( tt );
}


void uiTable::setLabelBGColor( int rc, Color c, bool isrow )
{
    mQtclass(QTableWidgetItem&) qw = body_->getRCItem( rc, isrow );
    qw.setBackground( mQtclass(QBrush)(mQtclass(QColor)(c.r(),c.g(),
		    					c.b(),255)) );
}


void uiTable::setRowLabels( const char** labels )
{
    BufferStringSet lbls( labels );
    setRowLabels( lbls );
}


void uiTable::setRowLabels( const BufferStringSet& labels )
{
    body_->setNrLines( labels.size() );
    for ( int i=0; i<labels.size(); i++ )
        setRowLabel( i, *labels[i] );
}


const char* uiTable::columnLabel( int col ) const
{
    static BufferString ret;
    mQtclass(QTableWidgetItem*) itm = body_->horizontalHeaderItem( col );
    if ( !itm )
	return 0;

    ret = itm->text().toAscii().data();
    return ret;
}


void uiTable::setColumnLabel( int col, const char* label )
{
    mQtclass(QTableWidgetItem&) itm = body_->getRCItem( col, false );
    itm.setText( label );
    itm.setToolTip( label );
}


void uiTable::setColumnToolTip( int col, const char* tt )
{
    body_->getRCItem(col,false).setToolTip( tt );
}


void uiTable::setColumnLabels( const char** labels )
{
    BufferStringSet lbls( labels );
    setColumnLabels( lbls );
}


void uiTable::setColumnLabels( const BufferStringSet& labels )
{
    body_->setColumnCount( labels.size() );

    for ( int i=0; i<labels.size(); i++ )
        setColumnLabel( i, labels[i]->buf() );
}


void uiTable::setLabelAlignment( Alignment::HPos hal, bool col )
{
    mQtclass(QHeaderView*) hdr = col ? body_->horizontalHeader()
				     : body_->verticalHeader();
    if ( hdr )
    {
	Alignment al( hal, Alignment::VCenter );
	hdr->setDefaultAlignment( (mQtclass(Qt)::Alignment)al.uiValue() );
    }
}


int uiTable::getIntValue( const RowCol& rc ) const
{
    const char* str = text( rc );
    if ( !str || !*str ) return mUdf(int);

    return Conv::to<int>( str );
}


double uiTable::getdValue( const RowCol& rc ) const
{
    const char* str = text( rc );
    if ( !str || !*str ) return mUdf(double);

    return Conv::to<double>(str);
}


float uiTable::getfValue( const RowCol& rc ) const
{
    const char* str = text( rc );
    if ( !str || !*str ) return mUdf(float);

    return Conv::to<float>(str);
}


void uiTable::setValue( const RowCol& rc, int i )
{
    BufferString txt( Conv::to<const char*>(i) );
    setText( rc, txt.buf() );
}


void uiTable::setValue( const RowCol& rc, float f )
{
    if ( mIsUdf(f) )
	setText( rc, "" );
    else
    {
	BufferString txt( Conv::to<const char*>(f) );
	setText( rc, txt.buf() );
    }
}


void uiTable::setValue( const RowCol& rc, double d )
{
    if ( mIsUdf(d) )
	setText( rc, "" );
    else
    {
	BufferString txt( Conv::to<const char*>(d) );
	setText( rc, txt.buf() );
    }
}


void uiTable::setSelectionMode( SelectionMode m )
{
    switch ( m ) 
    {
	case Single:
	    body_->setSelectionMode(
		    	       mQtclass(QAbstractItemView)::SingleSelection );
	    break;
	case Multi:
	    body_->setSelectionMode(
		    	       mQtclass(QAbstractItemView)::ExtendedSelection );
	    break;
	case SingleRow:
	    setSelectionBehavior( uiTable::SelectRows );
	    break;
	default:
	    body_->setSelectionMode( mQtclass(QAbstractItemView)::NoSelection );
	    break;
    }
}


void uiTable::setSelectionBehavior( SelectionBehavior sb )
{
    const int sbi = (int)sb;
    body_->setSelectionBehavior(
	    		(mQtclass(QAbstractItemView)::SelectionBehavior)sbi );
}


void uiTable::editCell( const RowCol& rc, bool replace )
{
    mBlockCmdRec;
    mQtclass(QTableWidgetItem*) itm = body_->item( rc.row, rc.col );
    body_->editItem( itm );
}


void uiTable::popupMenu( CallBacker* )
{
    const int xcursorpos = mQtclass(QCursor)::pos().x();
    const int ycursorpos = mQtclass(QCursor)::pos().y();

    if ( uiVirtualKeyboard::isVirtualKeyboardActive() )
	return;

    if ( (!setup_.rowgrow_ && !setup_.colgrow_ && !setup_.enablecopytext_) ||
	 setup_.rightclickdisabled_ )
    {
	popupVirtualKeyboard( xcursorpos, ycursorpos );
	return;
    }

    uiPopupMenu* mnu = new uiPopupMenu( parent(), "Action" );
    BufferString itmtxt;

    const RowCol cur = notifiedCell();
    int inscolbef = 0;
    int delcol = 0;
    int inscolaft = 0;
    if ( setup_.colgrow_ )
    {
	if ( setup_.insertcolallowed_ )
	{
	    itmtxt =  BufferString( "Insert ", setup_.coldesc_, " before" );
	    inscolbef = mnu->insertItem( new uiMenuItem(itmtxt), 0 );
	    itmtxt =  BufferString( "Insert ", setup_.coldesc_, " after" );
	    inscolaft = mnu->insertItem( new uiMenuItem(itmtxt), 2 );
	}

	if ( setup_.removecolallowed_ )
	{
	    itmtxt = "Remove "; itmtxt += setup_.coldesc_;
	    delcol = mnu->insertItem( new uiMenuItem(itmtxt), 1 );
	}
    }

    int insrowbef = 0;
    int delrow = 0;
    int insrowaft = 0;
    if ( setup_.rowgrow_ )
    {
	if ( setup_.insertrowallowed_ )
	{
	    itmtxt = BufferString( "Insert ", setup_.rowdesc_, " before" );
	    insrowbef = mnu->insertItem( new uiMenuItem(itmtxt), 3 );
	    itmtxt = BufferString( "Insert ", setup_.rowdesc_, " after" );
	    insrowaft = mnu->insertItem( new uiMenuItem(itmtxt), 5 );
	}

	if ( setup_.removerowallowed_ )
	{
	    itmtxt = "Remove "; itmtxt += setup_.rowdesc_;
	    delrow = mnu->insertItem( new uiMenuItem(itmtxt), 4 );
	}
    }

    int cptxt = 0;
    if ( isTableReadOnly() && setup_.enablecopytext_ )
    {
	itmtxt = "Copy text";
	cptxt = mnu->insertItem( new uiMenuItem(itmtxt), 6 );
    }

    int virkeyboardid = mUdf(int);
    if ( needOfVirtualKeyboard() )
    {
	mnu->insertSeparator();
	itmtxt = "Virtual Keyboard";
	virkeyboardid = mnu->insertItem( new uiMenuItem(itmtxt), 100 );
    }

    int ret = mnu->exec();
    if ( ret<0 ) return;

    if ( ret == inscolbef || ret == inscolaft )
    {
	const int offset = (ret == inscolbef) ? 0 : 1;
	newcell_ = RowCol( cur.row, cur.col+offset );
	insertColumns( newcell_, 1 );

	if ( !setup_.defcollbl_ )
	    setColumnLabel( newcell_, toString(newcell_.col) );

	colInserted.trigger();
    }
    else if ( ret == delcol )
    {
	removeColumn( cur.col );
	colDeleted.trigger();
    }
    else if ( ret == insrowbef || ret == insrowaft  )
    {
	const int offset = (ret == insrowbef) ? 0 : 1;
	newcell_ = RowCol( cur.row+offset, cur.col );
	insertRows( newcell_, 1 );

	if ( !setup_.defrowlbl_ )
	    setRowLabel( newcell_, toString(newcell_.row) );

	rowInserted.trigger();
    }
    else if ( ret == delrow )
    {
	removeRow( cur.row );
	rowDeleted.trigger();
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

	mQtclass(QApplication)::clipboard()->setText( mQtclass(QString)(str) );
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
    body_->clearContents();
}


void uiTable::removeAllSelections()
{
    mBlockCmdRec;
    body_->clearSelection();
}


bool uiTable::isSelected ( const RowCol& rc ) const
{
    const mQtclass(QItemSelectionModel*) selmodel = body_->selectionModel();
    const mQtclass(QAbstractItemModel*) model = selmodel ? selmodel->model()
							 : 0;
    if ( !model )
	return false;

    mQtclass(QModelIndex) idx = body_->rootIndex();
    idx = model->index( rc.row, rc.col, idx );
    return selmodel->isSelected( idx );
}


bool uiTable::isRowSelected( int row ) const
{
    mQtclass(QItemSelectionModel*) model = body_->selectionModel();
    return model ? model->isRowSelected( row, body_->rootIndex() ) : false;
}


bool uiTable::isColumnSelected( int col ) const
{
    mQtclass(QItemSelectionModel*) model = body_->selectionModel();
    return model ? model->isColumnSelected( col, body_->rootIndex() ) : false;
}


int uiTable::currentRow() const
{ return body_->currentRow(); }

int uiTable::currentCol() const
{ return body_->currentColumn(); }


void uiTable::setSelected( const RowCol& rc, bool yn )
{
    mBlockCmdRec;
    mQtclass(QTableWidgetItem*) itm = body_->item( rc.row, rc.col );
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
    mQtclass(QTableWidgetItem*) itm = body_->item( rc.row, rc.col );
    body_->scrollToItem( itm );
}


void uiTable::setCellGroup( const RowCol& rc, uiGroup* grp )
{
    mBlockCmdRec;
    if ( !grp ) return;
    body_->setCellObject( rc, grp->attachObj() );
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
    body_->setCellObject( rc, obj );
}


uiObject* uiTable::getCellObject( const RowCol& rc ) const
{ return body_->getCellObject( rc ); }


void uiTable::clearCellObject( const RowCol& rc )
{
    mBlockCmdRec;
    body_->clearCellObject( rc );
}


RowCol uiTable::getCell( uiObject* obj )
{ return body_->getCell( obj ); }


const ObjectSet<uiTable::SelectionRange>& uiTable::selectedRanges() const
{
    deepErase( selranges_ );
    mQtclass(QList)<mQtclass(QTableWidgetSelectionRange)> qranges =
							body_->selectedRanges();
    for ( int idx=0; idx<qranges.size(); idx++ )
    {
	uiTable::SelectionRange* rg = new uiTable::SelectionRange;
	rg->firstrow_ = qranges[idx].topRow();
	rg->lastrow_ = qranges[idx].bottomRow();
	rg->firstcol_ = qranges[idx].leftColumn();
	rg->lastcol_ = qranges[idx].rightColumn();
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
	if ( mIsUdf(rcs[idx].row) || mIsUdf(rcs[idx].col) )
	    continue;
	mQtclass(QTableWidgetItem*) itm =
	    			    body_->item( rcs[idx].row, rcs[idx].col );
	if ( !itm || (yn && itm->isSelected()) || (!yn && !itm->isSelected()) )
	    continue;
	itm->setSelected( yn );
    }
}


bool uiTable::handleLongTabletPress()
{
    BufferString msg = "rightClicked ";
    msg += notifcell_.row; msg += " "; msg += notifcell_.col;
    const int refnr = beginCmdRecEvent( msg ); 
    rightClicked.trigger();
    endCmdRecEvent( refnr, msg );
    return true;
}


bool uiTable::needOfVirtualKeyboard() const
{
    if ( isCellReadOnly(notifiedCell()) || getCellObject(notifiedCell()) )
	return false;
    if ( isRowReadOnly(notifiedCell().row) )
	return false;
    if ( isColumnReadOnly(notifiedCell().col) )
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

