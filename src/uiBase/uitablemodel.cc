/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          October 2019
________________________________________________________________________

-*/


#include "uitablemodel.h"

#include "uiobjbody.h"

#include <QAbstractTableModel>
#include <QTableView>


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
 
protected:
    uiTableModel&	model_;
};


Qt::ItemFlags ODAbstractTableModel::flags( const QModelIndex& ) const
{
    return Qt::ItemIsEditable; // Qt::ItemIsEnabled Qt::ItemIsSelectable;
}


int ODAbstractTableModel::rowCount( const QModelIndex& ) const
{
    return model_.nrRows();
}


int ODAbstractTableModel::columnCount( const QModelIndex& ) const
{
    return model_.nrCols();
}


QVariant ODAbstractTableModel::data( const QModelIndex& index, int role ) const
{
    if ( !index.isValid() )
	return QVariant();

    if ( role == Qt::DisplayRole )
    {
	return model_.text(index.row(),index.column()).buf();
    }

    return QVariant();
}


bool ODAbstractTableModel::setData( const QModelIndex& index,
				    const QVariant& value, int role )
{
    if ( !index.isValid() )
	return false;

    return true;
}


QVariant ODAbstractTableModel::headerData( int rowcol, Qt::Orientation orient,
					   int role ) const
{
    if ( role == Qt::DisplayRole )
    {
	uiString str = model_.headerText( rowcol,
	    orient==Qt::Horizontal ? OD::Horizontal : OD::Vertical );
	return str.getQString();
    }

    return QVariant();
}


// uiTableModel
uiTableModel::uiTableModel()
{
    odtablemodel_ = new ODAbstractTableModel(*this);
}


uiTableModel::~uiTableModel()
{
    delete odtablemodel_;
}



class ODTableView : public uiObjBodyImpl<uiTableView,QTableView>
{
public:
ODTableView( uiTableView& hndl, uiParent* parent, const char* nm )
    : uiObjBodyImpl<uiTableView,QTableView>(hndl,parent,nm)
{}

protected:
};


uiTableView::uiTableView( uiParent* p, const char* nm )
    : uiObject(p,nm,mkView(p,nm))
    , tablemodel_(nullptr)
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
    if ( tablemodel_ )
	odtableview_->setModel( tablemodel_->getAbstractModel() );
}
