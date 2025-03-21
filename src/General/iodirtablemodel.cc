/*+
________________________________________________________________________

 Copyright:	(C) 1995-2024 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "iodirtablemodel.h"

#include "ctxtioobj.h"
#include "globexpr.h"
#include "iodir.h"
#include "iodirentry.h"
#include "keystrs.h"
#include "timefun.h"

#include "uistrings.h"
#include <QDate>


static const int sNameCol	= 0;
static const int sDateCol	= 1;
static const int sUserCol	= 2;


IODirTableModel::IODirTableModel( const IOObjContext& ctxt )
    : TableModel()
{
    iodir_ = new IODir( ctxt.getSelKey() );
    iodirentrylist_ = new IODirEntryList( *iodir_, ctxt );
}


IODirTableModel::~IODirTableModel()
{}


void IODirTableModel::setFilter( const char* filter )
{
    beginReset();
    BufferString namefilter( filter );
    GlobExpr::validateFilterString( namefilter );
    iodirentrylist_->fill( *iodir_, namefilter.buf() );
    endReset();
}


int IODirTableModel::nrRows() const
{
    return iodirentrylist_->size();
}


int IODirTableModel::nrCols() const
{
    return 3;
}


int IODirTableModel::flags( int row, int col ) const
{
    return ItemSelectable | ItemEnabled;
}


void IODirTableModel::setCellData(int row,int col,const CellData&)
{
}


TableModel::CellData IODirTableModel::getCellData( int row,int col ) const
{
    if ( !iodirentrylist_->validIdx(row) )
	return CellData();

    const auto* entry = iodirentrylist_->get( row );
    const IOObj* ioobj = entry->ioobj_;
    if ( !ioobj )
	return CellData();

    if ( col==sNameCol )
	return CellData( entry->getName().buf() );

    if ( col==sUserCol )
    {
	BufferString user;
	ioobj->pars().get( sKey::CrBy(), user );
	return CellData( user.buf() );
    }

    if ( col==sDateCol )
    {
	BufferString datestr;
	ioobj->pars().get( sKey::CrAt(), datestr );
	QDateTime qdatetime =
		QDateTime::fromString( datestr.buf(),  Qt::ISODate );
	return CellData( qdatetime );
    }

    return CellData();
}


OD::Color IODirTableModel::textColor( int row, int col ) const
{
    return OD::Color::Black();
}


OD::Color IODirTableModel::cellColor( int row,int col ) const
{
    return OD::Color::NoColor();
}


PixmapDesc IODirTableModel::pixmap( int row,int col ) const
{
    return PixmapDesc();
}


void IODirTableModel::setChecked( int row, int col, int val )
{}


int IODirTableModel::isChecked( int row, int col ) const
{
    return -1;
}


uiString IODirTableModel::headerText( int rowcol, OD::Orientation orient ) const
{
    if ( orient==OD::Vertical )
	return toUiString( rowcol+1 );

    const int col = rowcol;
    if ( col==sNameCol )
	return uiStrings::sName();
    if ( col==sUserCol )
	return toUiString("User");
    if ( col==sDateCol )
	return toUiString("Date");

    return uiString::empty();
}


uiString IODirTableModel::tooltip( int row, int col ) const
{
    return uiString::empty();
}


const EnumDef* IODirTableModel::getEnumDef( int col ) const
{
    return nullptr;
}


TableModel::CellType IODirTableModel::getColumnCellType( int col ) const
{
    if ( col==sNameCol || col==sUserCol )
	return Text;
    if ( col==sDateCol )
	return DateTime;

    return Other;
}
