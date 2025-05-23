/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitableview.h"
#include "i_qtableview.h"

#include "uiclipboard.h"
#include "uiobjbodyimpl.h"
#include "uipixmap.h"
#include "uiicon.h"

#include "tablemodel.h"

#include "q_uiimpl.h"

#include <QApplication>
#include <QByteArray>
#include <QCheckBox>
#include <QComboBox>
#include <QDate>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMargins>
#include <QPainter>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QTableView>


class ODStyledItemDelegate : public QStyledItemDelegate
{
public:
ODStyledItemDelegate( QObject* parent )
    : QStyledItemDelegate(parent)
{}

ODStyledItemDelegate( TableModel::CellType typ )
    : celltype_(typ)
{}

void paint( QPainter* painter, const QStyleOptionViewItem& option,
	    const QModelIndex& index ) const override
{
    QStyleOptionViewItem myoption = option;
    if ( option.state & QStyle::State_Selected )
	myoption.font.setBold( true );
    else
	myoption.font.setBold( false );
    initStyleOption( &myoption, index );

    // Give cells their original background color
    QVariant background = index.data( Qt::BackgroundRole );
    if ( background.canConvert<QBrush>() )
	painter->fillRect( option.rect, background.value<QBrush>() );

    QStyledItemDelegate::paint( painter, myoption, index );
}

TableModel::CellType cellType() const
{
    return celltype_;
}

private:

    TableModel::CellType	celltype_	= TableModel::Text;

}; // class ODStyledItemDelegate


class DecorationItemDelegate : public ODStyledItemDelegate
{
public:
DecorationItemDelegate()
    : ODStyledItemDelegate(TableModel::Color)
{}

static const int sXPosPadding = 2;
static const int sPmScalePadding = 10;

const uiPixmap* createPixmap( const QVariant& qvar, QRect rect ) const
{
    const QString qstr = qvar.toString();
    const BufferString desc( qstr );
    if ( desc.isEmpty() )
	return nullptr;

    PixmapDesc pd;
    pd.fromString( desc );
    if ( !pd.isValid() )
	return nullptr;

    auto* pm = new uiPixmap( pd );
    BufferString pmsrc = pm->source();
    if ( pm && pmsrc != PixmapDesc::sKeySingleColorSrc() )
	pm->scaleToHeight( rect.height() - sPmScalePadding );

    return pm;
}


void paint( QPainter* painter, const QStyleOptionViewItem& option,
	    const QModelIndex& index ) const override
{
    QVariant color = index.data( Qt::BackgroundRole );
    if ( color.canConvert<QColor>() )
	painter->fillRect( option.rect, color.value<QColor>() );

    const QVariant qvar = index.data( Qt::DecorationRole );
    ConstPtrMan<uiPixmap> pm = createPixmap( qvar, option.rect );
    if ( pm )
    {
	const QPixmap* qpm = pm->qpixmap();
	const int qpmwidth = qpm->rect().width();
	const int qpmheight = qpm->rect().height();
	const int xpos = option.rect.left() + sXPosPadding;
	const int ypos = option.rect.center().y() - qpmheight/2;
	painter->drawPixmap( xpos, ypos, qpmwidth, qpmheight, *qpm );
    }

    ODStyledItemDelegate::paint( painter, option, index );
}

}; // class DecorationItemDelegate


class DoubleItemDelegate : public ODStyledItemDelegate
{
public:
DoubleItemDelegate( TableModel::CellType tp,
		    char specifier, int precision )
    : ODStyledItemDelegate(tp)
    , specifier_(specifier)
    , precision_(precision)
{}


QString displayText( const QVariant& val, const QLocale& locale ) const override
{
    bool ok;
    const double dval = val.toDouble( &ok );
    if ( !ok )
	return QStyledItemDelegate::displayText( val, locale );

    QString ret;
    toUiString( dval, 0, specifier_, precision_ ).fillQString( ret );
    return ret;
}

   int		precision_;
   char		specifier_;

}; // class DoubleItemDelegate


class TextItemDelegate : public ODStyledItemDelegate
{
public:
TextItemDelegate()
    : ODStyledItemDelegate(TableModel::Text)
{}


QWidget* createEditor( QWidget* prnt,
		       const QStyleOptionViewItem&,
		       const QModelIndex& ) const override
{
    return new QLineEdit( prnt );
}


void setEditorData( QWidget* editor, const QModelIndex& index ) const override
{
    QString value = index.model()->data(index,Qt::DisplayRole).toString();
    QLineEdit* lineedit = static_cast<QLineEdit*>(editor);
    lineedit->setText( value );
}


void setModelData( QWidget* editor, QAbstractItemModel* model,
		   const QModelIndex& index ) const override
{
    QLineEdit* lineedit = static_cast<QLineEdit*>(editor);
    QString txt = lineedit->text();
    model->setData( index, txt, Qt::EditRole );
}

}; // class TextItemDelegate


class EnumItemDelegate : public ODStyledItemDelegate
{
public:
EnumItemDelegate(const EnumDef* enumdef)
    : ODStyledItemDelegate(TableModel::Enum)
    , enumdef_(enumdef)
{}


static const int sXPosPadding = 2;

void paint( QPainter* painter, const QStyleOptionViewItem& option,
	    const QModelIndex& index ) const override
{
    if ( !enumdef_ )
    {
	pErrMsg("Model must supply an EnumDef for columns with CellType Enum");
	QStyledItemDelegate::paint( painter, option, index );
	return;
    }

    const int fontheight = option.fontMetrics.height();
    QRect rect = option.rect;
    const int ymargin = (rect.height() - fontheight) / 2;
    rect -= QMargins( sXPosPadding, ymargin, sXPosPadding, ymargin );

    const QVariant qvar = index.data( Qt::DisplayRole );
    const int enumidx = enumdef_->indexOf( qvar.toInt() );
    const char* iconfnm = enumdef_->getIconFileForIndex( enumidx );
    Qt::Alignment textalignment = Qt::AlignCenter;
    if ( iconfnm && *iconfnm )
    {
	uiIcon icon( iconfnm );
	QRect iconrect = rect;
	iconrect.setWidth( rect.height() );
	rect.adjust( rect.height()+sXPosPadding, 0, 0, 0 );
	textalignment = Qt::AlignLeft;
	icon.qicon().paint( painter, iconrect, Qt::AlignCenter );
    }

    uiString label = enumdef_->getUiStringForIndex( enumidx );
    QString labeltxt;
    painter->drawText( rect, textalignment, label.fillQString(labeltxt) );
}


QWidget* createEditor( QWidget* prnt,
		       const QStyleOptionViewItem&,
		       const QModelIndex& ) const override
{
    auto* qcb = new QComboBox( prnt );
    if ( !enumdef_ )
	return qcb;

    for ( int idx=0; idx<enumdef_->size(); idx++ )
    {
	qcb->addItem( toQString(enumdef_->getUiStringForIndex(idx)) );
	const char* iconnm = enumdef_->getIconFileForIndex( idx );
	if ( !iconnm )
	    continue;

	uiIcon icon( iconnm );
	qcb->setItemIcon( idx, icon.qicon() );
    }

    return qcb;
}


void setEditorData( QWidget* editor, const QModelIndex& index ) const override
{
    if ( !enumdef_ )
	return;

    const QVariant qvar = index.data( Qt::DisplayRole );
    const int enumidx = enumdef_->indexOf( qvar.toInt() );
    QComboBox* qcb = static_cast<QComboBox*>(editor);
    qcb->setCurrentIndex( enumidx );
}


void setModelData( QWidget* editor, QAbstractItemModel* model,
		   const QModelIndex& index ) const override
{
    if ( !enumdef_ )
	return;

    QComboBox* qcb = static_cast<QComboBox*>(editor);
    const int enumval = enumdef_->getEnumValForIndex( qcb->currentIndex() );
    model->setData( index, enumval, Qt::EditRole );
}

protected:

    const EnumDef*	enumdef_;

}; // class EnumItemDelegate


class DateItemDelegate : public ODStyledItemDelegate
{
public:
DateItemDelegate()
    : ODStyledItemDelegate(TableModel::Date)
{}


QString displayText( const QVariant& value,
		     const QLocale& locale ) const override
{
    return locale.toString( value.toDate(),
			    locale.dateFormat(QLocale::ShortFormat) );
}

}; // class DateItemDelegate


class DateTimeItemDelegate : public ODStyledItemDelegate
{
public:
DateTimeItemDelegate()
    : ODStyledItemDelegate(TableModel::DateTime)
{}


QString displayText( const QVariant& value,
		     const QLocale& locale ) const override
{
    QDateTime qdt = value.toDateTime().toLocalTime();
    return locale.toString( qdt,
			    locale.dateTimeFormat(QLocale::ShortFormat) );
}

}; // class DateTimeItemDelegate


class ODTableView : public uiObjBodyImpl<uiTableView,QTableView>
{
public:
ODTableView( uiTableView& hndl, uiParent* p, const char* nm )
    : uiObjBodyImpl<uiTableView,QTableView>(hndl,p,nm)
    , messenger_(*new i_tableViewMessenger(this,&hndl))
{
    frozenview_ = new QTableView( this );
    helper_ = new FrozenColumnsHelper( this, frozenview_ );
}


~ODTableView()
{
    delete helper_;
    delete frozenview_;
    delete &messenger_;
}


void currentChanged( const QModelIndex& current,
		     const QModelIndex& previous ) override
{
    QTableView::currentChanged( current, previous );
}


void selectionChanged( const QItemSelection& selected,
		       const QItemSelection& deselected ) override
{
    QTableView::selectionChanged( selected, deselected );
    handle_.selectionChanged.trigger();
}


void setModel( QAbstractItemModel* tblmodel ) override
{
    QTableView::setModel( tblmodel );
    frozenview_->setModel( model() );
    frozenview_->setSelectionModel( selectionModel() );
}

void init()
{
    setStyleSheet( "selection-background-color: rgba(50, 50, 50, 50);"
		   "selection-color: black;" );
    if ( horizontalHeader() )
	horizontalHeader()->setDefaultAlignment(
		Qt::AlignCenter | Qt::Alignment(Qt::TextWordWrap) );

    setHorizontalScrollMode( ScrollPerPixel );

    initFrozenView();
}


void initFrozenView()
{
    viewport()->stackUnder( frozenview_ );

    frozenview_->setStyleSheet( styleSheet() );
    frozenview_->setFrameStyle( QFrame::NoFrame );
    frozenview_->setFocusPolicy( Qt::NoFocus );

    frozenview_->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    frozenview_->setHorizontalScrollMode( horizontalScrollMode() );
    frozenview_->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    frozenview_->setVerticalScrollMode( verticalScrollMode() );

    frozenview_->verticalHeader()->hide();
    frozenview_->horizontalHeader()->setSectionResizeMode( QHeaderView::Fixed );

    updateColumns();
}


void updateColumns()
{
    for ( int col=0; col<nrfrozencols_; col++ )
	frozenview_->setColumnWidth( col, columnWidth(col) );

    for ( int col=0; col<model()->columnCount(); col++ )
	frozenview_->setColumnHidden( col, col>=nrfrozencols_ );

    helper_->updateGeom();
}


void setNrFrozenColumns( int nrcols )
{
    nrfrozencols_ = nrcols;
    helper_->setNrColumns( nrcols );
    updateColumns();
    if ( nrcols>0 )
	frozenview_->show();
    else
	frozenview_->hide();
}


void setSortEnabled( bool yn )
{
    setSortingEnabled( yn );
    frozenview_->setSortingEnabled( yn );
}


void setContextMenuEnabled( bool yn )
{
    yn ? enableCustomContextMenu() : setContextMenuPolicyToDefault();
}


void setCurrentCell( const RowCol& rc, bool noselection )
{
    notifcell_ = rc;
    const QModelIndex index( model()->index(rc.row(),rc.col()) );
    if ( noselection )
	selectionModel()->setCurrentIndex( index,
					   QItemSelectionModel::NoUpdate );
    else
	setCurrentIndex( index );
}

    RowCol		notifcell_;

protected:

void keyPressEvent( QKeyEvent* ev ) override
{
    if ( !ev )
	return;

    if ( ev->key() == Qt::Key_Escape )
    {
	clearSelection();
	clearFocus();
    }
    else if ( ev->matches(QKeySequence::Copy) )
    {
	BufferString text;
	QItemSelectionRange range = selectionModel()->selection().first();
	for ( int row = range.top(); row <= range.bottom(); row++ )
	{
	    for ( int col = range.left(); col <= range.right(); col++ )
	    {
		text.add( model()->index(row,col).data().toString() );
		text.addTab();
	    }
	    text.addNewLine();
	}

	uiClipboard::setText( text );
	ev->setAccepted( true );
    }
    else
	QTableView::keyPressEvent( ev );
}


void resizeEvent( QResizeEvent* event ) override
{
    QTableView::resizeEvent( event );
    helper_->updateGeom();
}


void scrollTo( const QModelIndex& index, ScrollHint hint ) override
{
    if ( index.column() > nrfrozencols_-1 )
	QTableView::scrollTo( index, hint );
}


QModelIndex moveCursor( CursorAction act, Qt::KeyboardModifiers modif ) override
{
    QModelIndex current = QTableView::moveCursor( act, modif );
    const int mainviewx0 = visualRect(current).topLeft().x();
    int frozenwidth = 0;
    for ( int col=0; col<nrfrozencols_; col++ )
	frozenwidth += frozenview_->columnWidth( col );

    if ( act==MoveLeft && current.column()>0 && mainviewx0<frozenwidth )
    {
	const int newvalue =
		horizontalScrollBar()->value() + mainviewx0 - frozenwidth;
	horizontalScrollBar()->setValue( newvalue );
    }

    return current;
}


void enableCustomContextMenu()
{
    if ( contextMenuPolicy() == Qt::CustomContextMenu )
	return;

    setContextMenuPolicy( Qt::CustomContextMenu );
    horizontalHeader()->setContextMenuPolicy( Qt::CustomContextMenu );
    verticalHeader()->setContextMenuPolicy( Qt::CustomContextMenu );
    frozenview_->setContextMenuPolicy( Qt::CustomContextMenu );
    frozenview_->horizontalHeader()
	       ->setContextMenuPolicy( Qt::CustomContextMenu );
}


void setContextMenuPolicyToDefault()
{
    if ( contextMenuPolicy() == Qt::DefaultContextMenu )
	return;

    setContextMenuPolicy( Qt::DefaultContextMenu );
    horizontalHeader()->setContextMenuPolicy( Qt::DefaultContextMenu );
    verticalHeader()->setContextMenuPolicy( Qt::DefaultContextMenu );
    frozenview_->setContextMenuPolicy( Qt::DefaultContextMenu );
    frozenview_->horizontalHeader()
	       ->setContextMenuPolicy( Qt::DefaultContextMenu );
}

    QTableView*			frozenview_;
    int				nrfrozencols_	= 1;
    FrozenColumnsHelper*	helper_;
    i_tableViewMessenger&	messenger_;

};


uiTableView::uiTableView( uiParent* p, const char* nm )
    : uiObject(p,nm,mkView(p,nm))
    , doubleClicked(this)
    , rightClicked(this)
    , selectionChanged(this)
    , columnClicked(this)
    , rowClicked(this)
{
    columndelegates_.setNullAllowed( true );
}


uiTableView::~uiTableView()
{
    detachAllNotifiers();

    delete horizontalheaderstate_;
    deepErase( columndelegates_ );
}


ODTableView& uiTableView::mkView( uiParent* p, const char* nm )
{
    odtableview_ = new ODTableView( *this, p, nm );
    return *odtableview_;
}


void uiTableView::setModel( TableModel* mdl )
{
    tablemodel_ = mdl;
    if ( !tablemodel_ )
	return;

    deleteAndNullPtr( horizontalheaderstate_ );
    delete qproxymodel_;
    qproxymodel_ = new QSortFilterProxyModel();
    qproxymodel_->setSourceModel( tablemodel_->getAbstractModel() );
    odtableview_->setModel( qproxymodel_ );
    odtableview_->init();

    for ( int idx=0; idx<mdl->nrCols(); idx++ )
    {
	const char format = mdl->getColumnFormatSpecifier( idx );
	const int precision = mdl->getColumnPrecision( idx );
	setColumnValueType( idx, mdl->getColumnCellType(idx),
			    format, precision );
    }
}


void uiTableView::setNrFrozenColumns( int nrcols )
{
    odtableview_->setNrFrozenColumns( nrcols );
}


void uiTableView::setContextMenuEnabled( bool yn )
{
    odtableview_->setContextMenuEnabled( yn );
}


void uiTableView::saveHorizontalHeaderState()
{
    deleteAndNullPtr( horizontalheaderstate_ );
    QHeaderView* horhdr = odtableview_->horizontalHeader();
    if ( horhdr )
	horizontalheaderstate_ = new QByteArray( horhdr->saveState() );
}


void uiTableView::resetHorizontalHeader()
{
    if ( !horizontalheaderstate_ )
	return;

    QHeaderView* horhdr = odtableview_->horizontalHeader();
    if ( horhdr )
	horhdr->restoreState( *horizontalheaderstate_ );

    odtableview_->clearSelection();
    odtableview_->clearFocus();
}


void uiTableView::setSectionsMovable( bool yn )
{
    odtableview_->horizontalHeader()->setSectionsMovable( yn );
}


void uiTableView::setSortingEnabled( bool yn )
{
    odtableview_->setSortEnabled( yn );
}


bool uiTableView::isSortingEnabled() const
{
    return odtableview_->isSortingEnabled();
}


void uiTableView::sortByColumn( int col, bool asc )
{
    odtableview_->sortByColumn( col,
			asc ? Qt::AscendingOrder : Qt::DescendingOrder );
}


void uiTableView::setRowHidden( int row, bool yn )
{ odtableview_->setRowHidden( row, yn ); }

bool uiTableView::isRowHidden( int row ) const
{ return odtableview_->isRowHidden( row ); }

void uiTableView::setColumnHidden( int col, bool yn )
{ odtableview_->setColumnHidden( col, yn ); }

bool uiTableView::isColumnHidden( int col ) const
{ return odtableview_->isColumnHidden( col ); }


void uiTableView::getVisibleRows( TypeSet<int>& rows,
				  bool mappedtosource ) const
{
    for ( int idx=0; idx<tablemodel_->nrRows(); idx++ )
    {
	if ( isRowHidden(idx) )
	    continue;

	if ( mappedtosource )
	{
	    const RowCol rc = mapToSource( RowCol(idx,0) );
	    rows += rc.row();
	}
	else
	    rows += idx;
    }
}


void uiTableView::getVisibleColumns( TypeSet<int>& cols,
				     bool mappedtosource ) const
{
    for ( int idx=0; idx<tablemodel_->nrCols(); idx++ )
    {
	if ( isColumnHidden(idx) )
	    continue;

	if ( mappedtosource )
	{
	    const RowCol rc = mapToSource( RowCol(0,idx) );
	    cols += rc.col();
	}
	else
	    cols += idx;
    }
}


void uiTableView::setHeaderVisible( OD::Orientation odor, bool yn )
{
    if ( odor==OD::Horizontal && odtableview_->horizontalHeader() )
	odtableview_->horizontalHeader()->setVisible( yn );
    else if ( odor==OD::Vertical && odtableview_->verticalHeader() )
	odtableview_->verticalHeader()->setVisible( yn );
}


bool uiTableView::isHeaderVisible( OD::Orientation odor ) const
{
    if ( odor == OD::Horizontal )
	return odtableview_->horizontalHeader() &&
	       odtableview_->horizontalHeader()->isVisible();

    return odtableview_->verticalHeader() &&
	       odtableview_->verticalHeader()->isVisible();
}


RowCol uiTableView::mapFromSource( const RowCol& rc ) const
{
    QModelIndex sourceidx =
	tablemodel_->getAbstractModel()->index( rc.row(), rc.col() );
    QModelIndex qmi = qproxymodel_->mapFromSource( sourceidx );
    return RowCol( qmi.row(), qmi.column() );
}


RowCol uiTableView::mapToSource( const RowCol& rc ) const
{
    QModelIndex proxyidx = qproxymodel_->index( rc.row(), rc.col() );
    QModelIndex qmi = qproxymodel_->mapToSource( proxyidx );
    return RowCol( qmi.row(), qmi.column() );
}


void uiTableView::setSelectionBehavior( SelectionBehavior sb )
{
    odtableview_->setSelectionBehavior(
		sCast(QAbstractItemView::SelectionBehavior,sCast(int,sb)) );
}


uiTableView::SelectionBehavior uiTableView::getSelectionBehavior() const
{
    QAbstractItemView::SelectionBehavior sb = odtableview_->selectionBehavior();
    return sCast(uiTableView::SelectionBehavior,sb);
}


void uiTableView::setSelectionMode( SelectionMode sm )
{
    odtableview_->setSelectionMode(
		    sCast(QAbstractItemView::SelectionMode,sCast(int,sm)) );
}


uiTableView::SelectionMode uiTableView::getSelectionMode() const
{
    QAbstractItemView::SelectionMode sm = odtableview_->selectionMode();
    return sCast(uiTableView::SelectionMode,sm);
}


void uiTableView::clearSelection()
{
    odtableview_->clearSelection();
}


int uiTableView::maxNrOfSelections() const
{
    if ( getSelectionMode()==NoSelection )
	return 0;
    if ( getSelectionMode()==SingleSelection )
	return 1;
    if ( getSelectionBehavior()==SelectRows )
	return tablemodel_->nrRows();
    if ( getSelectionBehavior()==SelectColumns )
	return tablemodel_->nrCols();

    return tablemodel_->nrRows() * tablemodel_->nrCols();
}


bool uiTableView::getSelectedRows( TypeSet<int>& rows ) const
{
    QItemSelectionModel* selmdl = odtableview_->selectionModel();
    if ( !selmdl->hasSelection() )
	return false;

    QModelIndexList selection = selmdl->selectedRows();
    for ( int idx=0; idx<selection.size(); idx++ )
    {
	const int selrow = selection[idx].row();
	if ( !isRowHidden(selrow) )
	    rows += selrow;
    }

    return rows.size();
}


bool uiTableView::getSelectedColumns( TypeSet<int>& cols ) const
{
    QItemSelectionModel* selmdl = odtableview_->selectionModel();
    if ( !selmdl->hasSelection() )
	return false;

    QModelIndexList selection = selmdl->selectedColumns();
    for ( int idx=0; idx<selection.size(); idx++ )
    {
	const int selcol = selection[idx].column();
	if ( !isColumnHidden(selcol) )
	    cols += selcol;
    }

    return cols.size();
}


bool uiTableView::getSelectedCells( TypeSet<RowCol>& rcs,
				    bool mappedtosource ) const
{
    QItemSelectionModel* selmdl = odtableview_->selectionModel();
    if ( !selmdl->hasSelection() )
	return false;

    QModelIndexList selection = selmdl->selectedIndexes();
    for ( int idx=0; idx<selection.size(); idx++ )
    {
	const int selrow = selection[idx].row();
	const int selcol = selection[idx].column();
	if ( isRowHidden(selrow) || isColumnHidden(selcol) )
	    continue;

	const RowCol rc( selrow, selcol );
	if ( mappedtosource )
	    rcs += mapToSource( rc );
	else
	    rcs += rc;
    }

    return rcs.size();
}


void uiTableView::setSelectedCells( const TypeSet<RowCol>& rcs )
{
    QItemSelectionModel* selmdl = odtableview_->selectionModel();
    for ( const auto& rc : rcs )
    {
	const QModelIndex idx =
		tablemodel_->getAbstractModel()->index( rc.row(), rc.col() );
	selmdl->select( idx, QItemSelectionModel::Select );
    }
}


void uiTableView::setSelectedCells( const TypeSet<RowCol>& rcs,
				    bool mapfromsource )
{
    QItemSelectionModel* selmdl = odtableview_->selectionModel();
    for ( const auto& rc : rcs )
    {
	const QModelIndex sourceidx =
		tablemodel_->getAbstractModel()->index( rc.row(), rc.col() );
	if ( !mapfromsource )
	{
	    selmdl->select( sourceidx, QItemSelectionModel::Select );
	    continue;
	}

	const QModelIndex qmi = qproxymodel_->mapFromSource( sourceidx );
	selmdl->select( qmi, QItemSelectionModel::Select );
    }
}


void uiTableView::setCellSelected( const RowCol& rc, bool yn,
				   bool mapfromsource )
{
    QItemSelectionModel* selmdl = odtableview_->selectionModel();
    const QModelIndex sourceidx =
		tablemodel_->getAbstractModel()->index( rc.row(), rc.col() );
    if ( !mapfromsource )
	selmdl->select( sourceidx, QItemSelectionModel::Select );
    else
    {
	const QModelIndex qmi = qproxymodel_->mapFromSource( sourceidx );
	selmdl->select( qmi, QItemSelectionModel::Select );
    }
}


bool uiTableView::isCellSelected( const RowCol& rc, bool mapfromsource ) const
{
    const QItemSelectionModel* selmdl = odtableview_->selectionModel();
    const QAbstractItemModel* model = selmdl ? selmdl->model() : nullptr;
    if ( !model )
	return false;

    QModelIndex idx = odtableview_->rootIndex();
    idx = model->index( rc.row(), rc.col(), idx );
    return selmdl->isSelected( idx );
}


void uiTableView::selectColumn( int col )
{
    odtableview_->selectColumn( col );
}


void uiTableView::selectRow( int row )
{
    odtableview_->selectRow( row );
}


void uiTableView::removeSelection( const TypeSet<RowCol>& rcs )
{
    QItemSelectionModel* selmdl = odtableview_->selectionModel();
    if ( !selmdl->hasSelection() )
	return;

    for ( const auto& rc : rcs )
    {
	const QModelIndex idx = tablemodel_->getAbstractModel()
					   ->index( rc.row(), rc.col() );
	selmdl->select( idx, QItemSelectionModel::Deselect );
    }
}


void uiTableView::selectAll()
{
    odtableview_->selectAll();
}


void uiTableView::setColumnValueType( int col, TableModel::CellType tp,
				      char format, int precision )
{
    ODStyledItemDelegate* coldelegate = getColumnDelegate( col, tp,
							   format, precision );
    if ( coldelegate )
	odtableview_->setItemDelegateForColumn( col, coldelegate );
}


ODStyledItemDelegate*
	uiTableView::getColumnDelegate( int col, TableModel::CellType tp,
					char format, int precision )
{
    if ( columndelegates_.validIdx(col) && columndelegates_[col] &&
	 columndelegates_[col]->cellType() == tp )
	return columndelegates_[col];

    while ( columndelegates_.size() <= col )
	columndelegates_ += nullptr;

    ODStyledItemDelegate* res = createColumnDelegate( col, tp,
						      format, precision );
    delete columndelegates_.replace( col, res );
    return res;
}


ODStyledItemDelegate*
	uiTableView::createColumnDelegate( int col, TableModel::CellType tp,
					   char format, int precision )
{
    if ( tp==TableModel::NumD || tp==TableModel::NumF )
	return new DoubleItemDelegate( tp, format, precision );
    if ( tp==TableModel::Text )
	return new TextItemDelegate;
    if ( tp==TableModel::Enum )
	return new EnumItemDelegate( tablemodel_ ? tablemodel_->getEnumDef(col)
						 : nullptr );
    if ( tp==TableModel::Color )
	return new DecorationItemDelegate;
    if ( tp==TableModel::Date )
	return new DateItemDelegate;
    if ( tp==TableModel::DateTime )
	return new DateTimeItemDelegate;

    return nullptr;
}


void uiTableView::setColumnWidth( int col, int wdth )
{
    odtableview_->setColumnWidth( col, wdth );
}


const RowCol& uiTableView::currentCell() const
{
    return odtableview_->notifcell_;
}


void uiTableView::setCurrentCell( const RowCol& rc, bool noselection )
{
    return odtableview_->setCurrentCell( rc, noselection );
}


TableModel::CellType uiTableView::getCellType( int col ) const
{
    if ( columndelegates_.validIdx(col) && columndelegates_[col] )
	return columndelegates_[col]->cellType();

    return TableModel::Other;
}


void uiTableView::moveColumn( int from, int to )
{
    const QModelIndex fromidx
		= tablemodel_->getAbstractModel()->index( 0, from );
    if ( !fromidx.isValid() )
    {
	pErrMsg("Invalid \"from\" column");
	return;
    }

    const QModelIndex toidx
		= tablemodel_->getAbstractModel()->index( 0, to );
    if ( !toidx.isValid() )
    {
	pErrMsg("Invalid \"to\" column");
	return;
    }

    QHeaderView *headerView = odtableview_->horizontalHeader();
    headerView->moveSection(from,to);
}
