/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "tablemodel.h"

#include "perthreadrepos.h"
#include "pixmapdesc.h"

#include <QAbstractTableModel>
#include <QByteArray>
#include <QSortFilterProxyModel>

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

protected:
    TableModel&		model_;
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
	PixmapDesc pd;
	pd.fromStringSet( model_.pixmap(qmodidx.row(), qmodidx.column()) );
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
	TableModel::CellData cd( qvar );
	model_.setCellData( qmodidx.row(), qmodidx.column(), cd );
	return true;
    }

    if ( role == Qt::CheckStateRole )
    {
	int val = -1;
	if ( qvar==Qt::Checked || qvar==Qt::Unchecked )
	    val = qvar==Qt::Checked ? 1 : 0;

	model_.setChecked( qmodidx.row(), qmodidx.column(), val );
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


// TableModel
TableModel::TableModel()
{
    odtablemodel_ = new ODAbstractTableModel(*this);
}


TableModel::~TableModel()
{
    delete odtablemodel_;
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

TableModel::CellData::CellData( float val, int )
    : qvar_(*new QVariant(val))
{}

TableModel::CellData::CellData( double val, int )
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
{ return qvar_.toFloat(); }

double TableModel::CellData::getDValue() const
{ return qvar_.toDouble(); }

int TableModel::CellData::getIntValue() const
{ return qvar_.toInt(); }

QAbstractTableModel* TableModel::getAbstractModel()
{ return odtablemodel_; }

void TableModel::beginReset()
{ odtablemodel_->beginReset(); }

void TableModel::endReset()
{ odtablemodel_->endReset(); }
