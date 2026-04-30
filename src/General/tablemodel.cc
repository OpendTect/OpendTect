/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "tablemodel.h"

#include "perthreadrepos.h"

#include <QAbstractTableModel>
#include <QByteArray>
#include <QDate>
#include <QDateTime>
#include <QSortFilterProxyModel>

#include "hiddenparam.h"

class ODAbstractTableModel : public QAbstractTableModel
{
public:
			ODAbstractTableModel( TableModel& mdl )
			    : model_(mdl)		{}

    Qt::ItemFlags	flags(const QModelIndex&) const override;
    int			rowCount(const QModelIndex&) const override;
    int			columnCount(const QModelIndex&) const override;
    QVariant		data(const QModelIndex&,int role) const override;
    QVariant		headerData(int rowcol,Qt::Orientation orientation,
				   int role=Qt::DisplayRole) const override;
    bool		setData(const QModelIndex&,const QVariant&,
				int role) override;
    void		beginReset();
    void		endReset();
    bool		collectEditRequests(const TableModelEditRequest&,
					    TypeSet<TableModelEditRequest>&);
    bool		applyEditRequest(const TableModelEditRequest&,
					 bool useoldval);
    void		rowBulkDataChanged(int row,
					   const TypeSet<int>&);

protected:
    TableModel&		model_;
};

static HiddenParam<TableModel,
		   CNotifier<CallBacker,const TableModelEditRequest&>*>
					       hp_editrequested_( nullptr );


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
	TableModel::CellData cd =
		model_.getCellData( qmodidx.row(), qmodidx.column() );
	return cd.qvar_;
    }

    if ( role == Qt::BackgroundRole )
    {
	OD::Color odcol = model_.cellColor( qmodidx.row(), qmodidx.column() );
	if ( odcol == OD::Color::NoColor() )
	    return QVariant();

	return odcol.getStdStr();
    }

    if ( role == Qt::DecorationRole )
    {
	const PixmapDesc pd = model_.pixmap( qmodidx.row(), qmodidx.column() );
	if ( !pd.isValid() )
	    return QVariant();

	return pd.toString().buf();
    }

    if ( role == Qt::ForegroundRole )
    {
	OD::Color odcol = model_.textColor( qmodidx.row(), qmodidx.column() );
	return odcol.rgb();
    }

    if ( role == Qt::ToolTipRole )
    {
	const uiString tt =
	    model_.tooltip( qmodidx.row(), qmodidx.column() );
	return tt.getString().str();
    }

    if ( role == Qt::CheckStateRole )
    {
	const int val = model_.isChecked( qmodidx.row(), qmodidx.column() );
	if ( val==-1 ) // no checkbox
	    return QVariant();

	return val==1 ? Qt::Checked : Qt::Unchecked;
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
	const TableModel::CellData oldval( data(qmodidx,role) );
	const TableModel::CellData newval( qvar );
	const TableModelEditRequest req( qmodidx.row(), qmodidx.column(),
					 oldval, newval, role );
	model_.editRequested().trigger( req );
	if ( req.handled_ )
	{
	    emit dataChanged( qmodidx, qmodidx, {Qt::DisplayRole,Qt::EditRole,
						 Qt::BackgroundRole} );
	    return true;
	}

	const TableModel::CellData cd( qvar );
	model_.setCellData( qmodidx.row(), qmodidx.column(), cd );
	emit dataChanged( qmodidx, qmodidx, {Qt::DisplayRole,Qt::EditRole,
					     Qt::BackgroundRole} );
	return true;
    }

    if ( role == Qt::CheckStateRole )
    {
	int val = -1;
	if ( qvar==Qt::Checked || qvar==Qt::Unchecked )
	    val = qvar==Qt::Checked ? 1 : 0;

	model_.setChecked( qmodidx.row(), qmodidx.column(), val );
	emit dataChanged( qmodidx, qmodidx, {Qt::CheckStateRole} );
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
	return str.getString().str();
    }

    return QVariant();
}


void ODAbstractTableModel::beginReset()
{ beginResetModel(); }

void ODAbstractTableModel::endReset()
{ endResetModel(); }


void ODAbstractTableModel::rowBulkDataChanged( int row,
					       const TypeSet<int>& changedcols )
{
    if ( changedcols.isEmpty() )
	return;

    bool inchangedcolrg = false;
    int startcol = 0;
    const int nrcols = columnCount( QModelIndex() );
    for ( int col=0; col<nrcols; col++ )
    {
	const bool ischanged = changedcols.isPresent( col );
	if ( ischanged && !inchangedcolrg )
	{
	    inchangedcolrg = true;
	    startcol = col;
	}
	else if ( inchangedcolrg )
	{
	    const QModelIndex tl = index( row, startcol );
	    const QModelIndex br = index( row, col-1 );
	    if ( tl.isValid() && br.isValid() )
		emit dataChanged( tl, br, {Qt::DisplayRole,Qt::EditRole,
					   Qt::BackgroundRole} );
	    inchangedcolrg = false;
	}
    }

    if ( inchangedcolrg )
    {
	const QModelIndex startqmi = index( row, startcol );
	const QModelIndex stopqmi = index( row, nrcols-1 );
	if ( startqmi.isValid() && stopqmi.isValid() )
	    emit dataChanged( startqmi, stopqmi, {Qt::DisplayRole,Qt::EditRole,
						  Qt::BackgroundRole} );
    }
}


bool ODAbstractTableModel::collectEditRequests(
				const TableModelEditRequest& req,
				TypeSet<TableModelEditRequest>& relatedreqs )
{
    relatedreqs.setEmpty();
    if ( req.row_ < 0 || req.col_ < 0 )
	return false;

    const QModelIndex sourceidx = index( req.row_, req.col_ );
    if ( !sourceidx.isValid() )
	return false;

    const int nrcols = columnCount( QModelIndex() );
    TypeSet<TableModel::CellData> oldrowvals;
    for ( int col=0; col<nrcols; col++ )
    {
	const QModelIndex idx = index( req.row_, col );
	oldrowvals += TableModel::CellData( idx.isValid()
						? data(idx,Qt::EditRole)
						: QVariant() );
    }

    if ( !setData(sourceidx,req.newval_.qvar_,req.role_) )
	return false;

    for ( int col=0; col<nrcols; col++ )
    {
	const QModelIndex idx = index( req.row_, col );
	if ( !idx.isValid() )
	    continue;

	const TableModel::CellData newval( data(idx,Qt::EditRole) );
	const TableModel::CellData& oldval = oldrowvals.get( col );
	if ( oldval == newval )
	    continue;

	relatedreqs += TableModelEditRequest( req.row_, col, oldval, newval,
					      req.role_ );
    }

    return true;
}


bool ODAbstractTableModel::applyEditRequest( const TableModelEditRequest& req,
					     bool useoldval )
{
    if ( req.row_ < 0 || req.col_ < 0 )
	return false;

    const QModelIndex sourceidx = index( req.row_, req.col_ );
    if ( !sourceidx.isValid() )
	return false;

    const QVariant& val = useoldval ? req.oldval_.qvar_ : req.newval_.qvar_;
    return setData( sourceidx, val, req.role_ );
}


// TableModel::CellData
TableModel::CellData::CellData()
    : qvar_(*new QVariant())
{}

TableModel::CellData::CellData( const QVariant& qvar)
    : qvar_(*new QVariant(qvar))
{}

TableModel::CellData::CellData( const QString& qstr)
    : qvar_(*new QVariant(qstr))
{}

TableModel::CellData::CellData( const char* txt )
    : qvar_(*new QVariant(txt))
{}

TableModel::CellData::CellData( int val )
    : qvar_(*new QVariant(val))
{}

TableModel::CellData::CellData( float val )
    : qvar_(*new QVariant(val))
{}

TableModel::CellData::CellData( double val )
    : qvar_(*new QVariant(val))
{}

TableModel::CellData::CellData( bool val )
    : qvar_(*new QVariant(val))
{}

TableModel::CellData::CellData( const CellData& cd )
    : qvar_(*new QVariant(cd.qvar_))
{}

TableModel::CellData::~CellData()
{ delete &qvar_; }

bool TableModel::CellData::getBoolValue() const
{ return qvar_.toBool(); }


const char* TableModel::CellData::text() const
{
    mDeclStaticString( ret );
    ret.setEmpty();
    ret = qvar_.toString();
    return ret.buf();
}


float TableModel::CellData::getFValue() const
{
    bool ok = false;
    const float fval = qvar_.toFloat( &ok );
    return ok ? fval : mUdf(float);
}


double TableModel::CellData::getDValue() const
{ return qvar_.toDouble(); }

int TableModel::CellData::getIntValue() const
{ return qvar_.toInt(); }


TableModel::CellData& TableModel::CellData::operator=( const CellData& cd )
{
    qvar_ = cd.qvar_;
    return *this;
}


bool TableModel::CellData::operator==( const CellData& oth ) const
{
    if ( qvar_.metaType() != oth.qvar_.metaType() )
	return false;

    return qvar_.toString() == oth.qvar_.toString();
}


bool TableModel::CellData::operator!=( const CellData& oth ) const
{
    return !(*this == oth);
}


void TableModel::CellData::setDate( const char* datestr )
{
    qvar_ = QDate::fromString( datestr );
}


void TableModel::CellData::setISODateTime( const char* datestr )
{
    qvar_ = QDateTime::fromString( datestr,  Qt::ISODate );
}


// TableModel
TableModelEditRequest::TableModelEditRequest( int row, int col,
					  const TableModel::CellData& oldval,
					  const TableModel::CellData& newval,
					  int role )
    : row_(row)
    , col_(col)
    , oldval_(oldval)
    , newval_(newval)
    , role_(role)
{}


bool TableModelEditRequest::operator==( const TableModelEditRequest& oth ) const
{
    return row_ == oth.row_ &&
	   col_ == oth.col_ &&
	   oldval_ == oth.oldval_ &&
	   newval_ == oth.newval_ &&
	   role_ == oth.role_ &&
	   handled_ == oth.handled_;
}


bool TableModelEditRequest::operator!=( const TableModelEditRequest& oth ) const
{
    return !(*this == oth);
}


TableModel::TableModel()
{
    hp_editrequested_.setParam( this,
	    new CNotifier<CallBacker,const TableModelEditRequest&>(nullptr) );
    odtablemodel_ = new ODAbstractTableModel(*this);
}


TableModel::~TableModel()
{
    hp_editrequested_.removeAndDeleteParam( this );
    delete odtablemodel_;
}


QAbstractTableModel* TableModel::getAbstractModel()
{
    return odtablemodel_;
}


void TableModel::beginReset()
{
    odtablemodel_->beginReset();
}


void TableModel::endReset()
{
    odtablemodel_->endReset();
}


void TableModel::rowBulkDataChanged( int row,
				     const TypeSet<int>& changedcols )
{
    if ( !odtablemodel_ || changedcols.isEmpty() )
	return;

    odtablemodel_->rowBulkDataChanged( row, changedcols );
}


bool TableModel::collectEditRequests( const TableModelEditRequest& req,
				  TypeSet<TableModelEditRequest>& relatedreqs )
{
    return odtablemodel_ && odtablemodel_->collectEditRequests( req,
								relatedreqs );
}


bool TableModel::applyEditRequest( const TableModelEditRequest& req,
				   bool useoldval )
{
    return odtablemodel_ && odtablemodel_->applyEditRequest( req, useoldval );
}


CNotifier<CallBacker,const TableModelEditRequest&>&
TableModel::editRequested()
{
    auto* notif = hp_editrequested_.getParam( this );
    if ( !notif )
    {
	notif = new CNotifier<CallBacker,const TableModelEditRequest&>(nullptr);
	hp_editrequested_.setParam( this, notif );
    }

    return *notif;
}


OD::Color TableModel::textColor( int row, int col ) const
{
    return OD::Color::Black();
}


OD::Color TableModel::cellColor( int row, int col ) const
{
    return OD::Color::NoColor();
}


PixmapDesc TableModel::pixmap( int row, int col ) const
{
    return PixmapDesc();
}


uiString TableModel::tooltip( int row, int col ) const
{
    return uiString::empty();
}


TableModel::CellType TableModel::getColumnCellType( int col ) const
{
    return Text;
}


char TableModel::getColumnFormatSpecifier( int col ) const
{
    return 'g';
}


int TableModel::getColumnPrecision( int col ) const
{
    return 6;
}
