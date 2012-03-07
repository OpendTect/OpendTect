/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Feb 2010
 RCS:           $Id: databaseobject.cc,v 1.5 2012-03-07 16:17:15 cvskris Exp $
________________________________________________________________________

-*/

#include "databaseobject.h"

#include "dateinfo.h"
#include "errh.h"
#include "separstr.h"
#include "staticstring.h"
#include "price.h"
#include "sqlquery.h"

namespace SqlDB
{

DatabaseColumnBase::DatabaseColumnBase( DatabaseTable& dobj,
	const char* columnname,const char* columntype )
    : columnname_( columnname )
    , columntype_( columntype )
    , table_( dobj )
{
    dobj.columns_ += this;
}


bool DatabaseColumnBase::isDBTypeOK( const char* dbtype ) const
{
    if ( !columnType() )
    {
	pErrMsg("Cannot use default impl of isDBTypeOK without "
		"columnType() implemented." );
	return false;
    }

    return caseInsensitiveEqual( dbtype, columnType() );
}


const char* DatabaseColumnBase::createColumnQuery() const
{
    if ( !columnType() )
    {
	pErrMsg("Cannot use default impl of createColumnText without "
		"columnType() implemented." );
	return 0;
    }

    static StaticStringManager stm;
    BufferString& str = stm.getString();

    str = backQuoteString( columnName() );
    str += " ";
    str += columnType();
    
    const char* options = columnOptions();
    if ( options )
    {
	str += " ";
	str += options;
    }

    return str.buf();
}


const char* DatabaseColumnBase::selectString() const
{
    static StaticStringManager stm;
    BufferString& str = stm.getString();

    str = backQuoteString( table_.tableName() );
    str.add( "." ).add( backQuoteString( columnName() ) );
    return str;
}


StringDatabaseColumn::StringDatabaseColumn( DatabaseTable& dobj,
    const char* columnname, int maxsize )
    : DatabaseColumn<BufferString>( dobj, columnname, 0 )
{
    if ( maxsize==-1 )
    {
	columntype_ = "TEXT";
    }
    else
    {
	columntype_ = "VARCHAR(";
	columntype_ += maxsize;
	columntype_ += ")";
    }
}


DateDatabaseColumn::DateDatabaseColumn( DatabaseTable& dobj,
					const char* columnname )
    : DatabaseColumnBase( dobj, columnname, "DATE" )
{}


bool DateDatabaseColumn::parse( const Query& query, int column,
				DateInfo& di ) const
{

    SeparString datestr( query.data( column ), '-' );
    if ( datestr.size()!=3 )
	return false;

    int year, month, day;
    if ( !getFromString( year, datestr[0], mUdf(int) )  ||
	 !getFromString( month, datestr[1], mUdf(int) ) ||
	 !getFromString( day, datestr[2], mUdf(int) ) )
	return false;

    di.setDay( day );
    di.setMonth( month );
    di.setYear( year );

    return true;
}

const char* DateDatabaseColumn::dataString(const DateInfo& di) const
{
    SeparString datestr( toString( di.year() ), '-' );
    datestr.add( toString( di.usrMonth() ) );
    datestr.add( toString( di.day() ) );

    static StaticStringManager stm;
    BufferString& res = stm.getString();
    res = datestr.buf();
    return res.buf();
}


PriceDatabaseColumn::PriceDatabaseColumn( DatabaseTable& dobj,
					const char* columnname )
    : DatabaseColumnBase( dobj, columnname, "VARCHAR(50)" )
{}


bool PriceDatabaseColumn::parse( const Query& query, int column,
				Price& price ) const
{
    SeparString pricestr( query.data( column ), ' ' );
    if ( pricestr.size()!=2 )
	return false;

    const Currency* currency = Currency::getCurrency( pricestr[0] );
    if ( !currency )
	return false;

    int amount;
    if ( !getFromString( amount, pricestr[1], mUdf(int) ) )
	return false;

    price.currency_ = currency;
    price.amount_ = amount;

    return true;
}

const char* PriceDatabaseColumn::dataString( const Price& price ) const
{

    SeparString pricestr( price.currency_->abrevation_, ' ' );
    pricestr.add( toString( price.amount_ ) );

    static StaticStringManager stm;
    BufferString& res = stm.getString();
    res = pricestr.buf();
    return res.buf();
}


CreatedTimeStampDatabaseColumn::CreatedTimeStampDatabaseColumn(
	DatabaseTable& dobj )
    : DatabaseColumn<od_int64>( dobj, "created", "timestamp" )
{
    columnoptions_ = "DEFAULT CURRENT_TIMESTAMP";
}


bool CreatedTimeStampDatabaseColumn::parse( const Query& query,
					    int column, od_int64& time ) const
{
    time = query.i64Value( column );
    return true;
}


const char* CreatedTimeStampDatabaseColumn::selectString() const
{
    static StaticStringManager stm;
    BufferString& str = stm.getString();

    str = "UNIX_TIMESTAMP(";
    str.add( backQuoteString( table_.tableName() ) ).add(".");
    str += backQuoteString( columnName() );
    str += ")";

    return str.buf();
}



DatabaseTable::DatabaseTable( const char* tablename )
    : idcolumn_( 0 )
    , tablename_( tablename )
{
    idcolumn_ = new IDDatabaseColumn( *this );
}


DatabaseTable::~DatabaseTable()
{
    delete idcolumn_;
}


DatabaseTable::TableStatus DatabaseTable::getTableStatus( SqlDB::Access& access,
						BufferString& errmsg ) const
{
    return checkTable( false, access, errmsg );
}


bool DatabaseTable::fixTable( SqlDB::Access& access, BufferString& errmsg ) const
{
    return checkTable( true, access, errmsg );
}


DatabaseTable::TableStatus DatabaseTable::checkTable( bool fix,
	SqlDB::Access& access, BufferString& errmsg ) const
{
    Query query( access );

    if ( !query.execute("SHOW TABLES") )
    {
	errmsg = query.errMsg();
	return AccessError;
    }

    BufferStringSet tables;
    while ( query.next() )
	tables.add( query.data(0) );

    if ( tables.indexOf( tableName() )==-1 )
    {
	if ( !fix )
	    return MinorError;

	BufferString querystring = "CREATE TABLE ";
	querystring += backQuoteString( tableName() );
	querystring += " (";
	querystring += idcolumn_->createColumnQuery();
	querystring += " , PRIMARY KEY(";
	querystring += backQuoteString( idcolumn_->columnName() );
	querystring += ") )";

	if ( !query.execute( querystring.buf() ) )
	{
	    errmsg = query.errMsg();
	    return AccessError;
	}
    }

    BufferString querystring = "SHOW COLUMNS IN ";
    querystring += backQuoteString( tableName() );

    if ( !query.execute( querystring ) )
    {
	errmsg = query.errMsg();
	return AccessError;
    }

    BufferStringSet columns;
    BufferStringSet types;
    while ( query.next() )
    {
	columns.add( query.data(0) );
	types.add( query.data(1) );
    }

    bool minorerror = false;
    for ( int idx=0; idx<columns_.size(); idx++ )
    {
	const int rowidx = columns.indexOf( columns_[idx]->columnName() );
	if ( rowidx==-1 )
	{
	    if ( !fix )
	    {
		minorerror = true;
		continue;
	    }

	    querystring = "ALTER TABLE ";
	    querystring += backQuoteString( tableName() );
	    querystring += " ADD ";
	    querystring += columns_[idx]->createColumnQuery();
	    if ( !query.execute( querystring ) )
	    {
		errmsg = query.errMsg();
		return AccessError;
	    }

	    continue;
	}

	if ( !columns_[idx]->isDBTypeOK( types[rowidx]->str() ) )
	{
	    errmsg = "Column ";
	    errmsg += columns_[idx]->columnName();
	    errmsg += " is not the correct type";
	    return MajorError;
	}
    }

    return minorerror ? MinorError : OK; 
}


const char* DatabaseTable::idColumnName() const
{ return idcolumn_->columnName(); }


const char* DatabaseTable::idSelectString() const
{ return idcolumn_->selectString(); }


bool DatabaseTable::parseID(const Query& q,int col, int& id) const
{ return idcolumn_->parse( q, col, id ); }

} //namespace
