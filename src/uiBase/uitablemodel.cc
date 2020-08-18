/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		October 2019
________________________________________________________________________

-*/


#include "uitablemodel.h"

#include "uiobjbodyimpl.h"
#include "perthreadrepos.h"
#include "staticstring.h"

#include <QAbstractTableModel>
#include <QCheckBox>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPainter>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QStringList>
#include <QTableView>


class ODStyledItemDelegate : public QStyledItemDelegate
{
public:

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

};


class DoubleItemDelegate : public ODStyledItemDelegate
{
public:
DoubleItemDelegate()
    : ODStyledItemDelegate()
    , nrdecimals_(2)
{}


QString displayText( const QVariant& val, const QLocale& locale ) const
{
    bool ok;
    const double dval = val.toDouble( &ok );
    if ( !ok )
	return QStyledItemDelegate::displayText( val, locale );

    return QString( toString(dval/*,nrdecimals_*/) );
}

   int		nrdecimals_;
};


class TextItemDelegate : public ODStyledItemDelegate
{
public:
TextItemDelegate()
    : ODStyledItemDelegate()
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

};


class ODAbstractTableModel : public QAbstractTableModel
{
public:
			ODAbstractTableModel( uiTableModel& mdl )
			    : model_(mdl)		{}

    Qt::ItemFlags	flags(const QModelIndex&) const;
    int			rowCount(const QModelIndex&) const;
    int			columnCount(const QModelIndex&) const;
    QVariant		data(const QModelIndex&,int role) const;
    QVariant		headerData(int rowcol,Qt::Orientation orientation,
				   int role=Qt::DisplayRole) const;
    bool		setData(const QModelIndex&,const QVariant&,int role);
    void		beginReset();
    void		endReset();

protected:
    uiTableModel&	model_;
};


Qt::ItemFlags ODAbstractTableModel::flags( const QModelIndex& qmodidx ) const
{
    if ( !qmodidx.isValid() )
	return Qt::NoItemFlags;

    const int modelflags = model_.flags( qmodidx.row(), qmodidx.column() );
    return sCast(Qt::ItemFlags,modelflags);
}


int ODAbstractTableModel::rowCount( const QModelIndex& ) const
{
    return model_.nrRows();
}


int ODAbstractTableModel::columnCount( const QModelIndex& ) const
{
    return model_.nrCols();
}


QVariant ODAbstractTableModel::data( const QModelIndex& qmodidx,
				     int role ) const
{
    if ( !qmodidx.isValid() )
	return QVariant();

    if ( role == Qt::DisplayRole || role == Qt::EditRole )
    {
	uiTableModel::CellData cd =
		model_.getCellData( qmodidx.row(), qmodidx.column() );
	return cd.qvar_;
    }

    if ( qmodidx.column()==0 && role == Qt::CheckStateRole )
    {
	uiTableModel::CellData cd =
		model_.getCellData( qmodidx.row(), qmodidx.column() );
	return cd.qvar_;
    }

    if ( role == Qt::BackgroundRole )
    {
	Color odcol = model_.color( qmodidx.row(), qmodidx.column() );
	if ( odcol==Color::NoColor() )
	    return QVariant();

	return QColor( odcol.rgb() );
    }

    if ( role == Qt::ForegroundRole )
    {
	Color odcol = model_.textColor( qmodidx.row(), qmodidx.column() );
	return QColor( odcol.rgb() );
    }

    if ( role == Qt::ToolTipRole )
    {
	const uiString tt =
	    model_.tooltip( qmodidx.row(), qmodidx.column() );
	mGetQStr(qstr, tt);
	return qstr;
    }

    return QVariant();
}


bool ODAbstractTableModel::setData( const QModelIndex& qmodidx,
				    const QVariant& qvar, int role )
{
    if ( !qmodidx.isValid() )
	return false;

    if ( role == Qt::EditRole )
    {
	uiTableModel::CellData cd( qvar );
	model_.setCellData( qmodidx.row(), qmodidx.column(), cd );
	return true;
    }

    if ( role == Qt::CheckStateRole )
    {
	uiTableModel::CellData cd( qvar );
	model_.setChosenRow( qmodidx.row(), qmodidx.column(), cd );
	return true;
    }

    return true;
}


QVariant ODAbstractTableModel::headerData( int rowcol, Qt::Orientation orient,
					   int role ) const
{
    if ( role == Qt::DisplayRole )
    {
	uiString str = model_.headerText( rowcol,
	    orient==Qt::Horizontal ? OD::Horizontal : OD::Vertical );
	mGetQStr(qstr, str);
	return qstr;
    }

    return QVariant();
}


void ODAbstractTableModel::beginReset()
{ beginResetModel(); }

void ODAbstractTableModel::endReset()
{ endResetModel(); }


// uiTableModel
uiTableModel::uiTableModel()
{
    odtablemodel_ = new ODAbstractTableModel(*this);
}


uiTableModel::~uiTableModel()
{
    delete odtablemodel_;
}


uiTableModel::CellData::CellData() : qvar_(*new QVariant())
{}

uiTableModel::CellData::CellData( const QVariant& qvar)
    : qvar_(*new QVariant(qvar))
{}

uiTableModel::CellData::CellData( const char* txt ) : qvar_(*new QVariant(txt))
{}

uiTableModel::CellData::CellData( int val ) : qvar_(*new QVariant(val))
{}

uiTableModel::CellData::CellData( float val, int ) : qvar_(*new QVariant(val))
{}

uiTableModel::CellData::CellData( double val, int ) : qvar_(*new QVariant(val))
{}

uiTableModel::CellData::CellData( bool val ) : qvar_(*new QVariant(val))
{}

uiTableModel::CellData::CellData( const CellData& cd )
    : qvar_(*new QVariant(cd.qvar_))
{}

uiTableModel::CellData::~CellData()
{ delete &qvar_; }

bool uiTableModel::CellData::getBoolValue() const
{ return qvar_.toBool(); }

const char* uiTableModel::CellData::text() const
{
    mDeclStaticString( ret );
    ret.setEmpty();
    ret = qvar_.toString();
    return ret.buf();
}

float uiTableModel::CellData::getFValue() const
{ return qvar_.toFloat(); }

double uiTableModel::CellData::getDValue() const
{ return qvar_.toDouble(); }

int uiTableModel::CellData::getIntValue() const
{ return qvar_.toInt(); }

void uiTableModel::beginReset()
{ odtablemodel_->beginReset(); }

void uiTableModel::endReset()
{ odtablemodel_->endReset(); }

class ODTableView : public uiObjBodyImpl<uiTableView,QTableView>
{
public:
ODTableView( uiTableView& hndl, uiParent* p, const char* nm )
    : uiObjBodyImpl<uiTableView,QTableView>(hndl,p,nm)
{
    setStyleSheet( "selection-background-color: rgba(50, 50, 50, 50);"
		   "selection-color: black;" );
}

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

    QTableView::keyPressEvent( ev );
}
};


uiTableView::uiTableView( uiParent* p, const char* nm )
    : uiObject(p,nm,mkView(p,nm))
    , tablemodel_(nullptr)
    , qproxymodel_(nullptr)
{
}


uiTableView::~uiTableView()
{
}


ODTableView& uiTableView::mkView( uiParent* p, const char* nm )
{
    odtableview_ = new ODTableView( *this, p, nm );
    return *odtableview_;
}


void uiTableView::setModel( uiTableModel* mdl )
{
    tablemodel_ = mdl;
    if ( !tablemodel_ )
	return;

    delete qproxymodel_;
    qproxymodel_ = new QSortFilterProxyModel();
    qproxymodel_->setSourceModel(  tablemodel_->getAbstractModel() );
    odtableview_->setModel( qproxymodel_ );
}


void uiTableView::setSortingEnabled( bool yn )
{ odtableview_->setSortingEnabled( yn ); }

bool uiTableView::isSortingEnabled() const
{ return odtableview_->isSortingEnabled(); }


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


void uiTableView::setSelectionMode( SelectionMode sm )
{
    odtableview_->setSelectionMode(
		sCast(QAbstractItemView::SelectionMode,sCast(int,sm)) );
}


void uiTableView::clearSelection()
{
    odtableview_->clearSelection();
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


bool uiTableView::getSelectedCells( TypeSet<RowCol>& rcs ) const
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

	rcs += RowCol( selrow, selcol );
    }

    return rcs.size();
}


static TextItemDelegate* getTextDelegate()
{
    mDefineStaticLocalObject( PtrMan<TextItemDelegate>, del,
			      = new TextItemDelegate )
    return del;
}


static DoubleItemDelegate* getDoubleDelegate()
{
    mDefineStaticLocalObject( PtrMan<DoubleItemDelegate>, del,
			      = new DoubleItemDelegate )
    return del;
}


void uiTableView::setColumnValueType( int col, CellType tp )
{
    if ( tp==NumD || tp==NumF )
	odtableview_->setItemDelegateForColumn( col, getDoubleDelegate() );
    else if ( tp==Text )
	odtableview_->setItemDelegateForColumn( col, getTextDelegate() );
}


void uiTableView::setColumnWidth( int col, int wdt )
{
    odtableview_->setColumnWidth( col, wdt );
}
