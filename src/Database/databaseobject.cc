/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "databaseobject.h"

#include "dateinfo.h"
#include "separstr.h"
#include "perthreadrepos.h"
#include "sqlquery.h"

#define mCrBackQuoteString(nm,str) BufferString nm( str ); nm.quote( '`' );

namespace SqlDB
{

DatabaseColumnBase::DatabaseColumnBase( DatabaseTable& dobj,
	const char* columnname,const char* columntype )
    : table_( dobj )
    , columnname_( columnname )
    , columntype_( columntype )
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

    mDeclStaticString( ret );
    mCrBackQuoteString( bqcolnm, columnName() );

    ret.set( bqcolnm ).add( " " ).add( columnType() );
    const char* options = columnOptions();
    if ( options && *options )
	ret.add( " " ).add( options );

    return ret.buf();
}


const char* DatabaseColumnBase::selectString() const
{
    mCrBackQuoteString( bqtblnm, table_.tableName() );
    mCrBackQuoteString( bqcolnm, columnName() );

    mDeclStaticString( ret );
    ret.set( bqtblnm ).add( "." ).add( bqcolnm );
    return ret;
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

    int year=0, month=0, day=0;
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

    mDeclStaticString( ret );
    ret = datestr.buf();
    return ret.buf();
}


CreatedTimeStampDatabaseColumn::CreatedTimeStampDatabaseColumn(
	DatabaseTable& dobj )
    : DatabaseColumnBase( dobj, "created", "timestamp" )
{
    columnoptions_ = "DEFAULT CURRENT_TIMESTAMP";
}


bool CreatedTimeStampDatabaseColumn::parse( const Query& query,
					    int column, time_t& time ) const
{
    time = query.i64Value( column );
    return true;
}


const char* CreatedTimeStampDatabaseColumn::selectString() const
{
    mDeclStaticString( ret );

    mCrBackQuoteString( bqtblnm, table_.tableName() );
    mCrBackQuoteString( bqcolnm, columnName() );

    ret.set( "UNIX_TIMESTAMP(" )
	.add( bqtblnm ).add( "." ).add( bqcolnm )
	.add( ")" );

    return ret.buf();
}



DatabaseTable::DatabaseTable( const char* tablename )
    : tablename_( tablename )
{
    rowidcolumn_ = new IDDatabaseColumn( *this );
    timestampcolumn_ = new CreatedTimeStampDatabaseColumn( *this );
    entryidcolumn_ = new DatabaseColumn<int>( *this, "entryid", "INT(11)" );
}


DatabaseTable::~DatabaseTable()
{
    delete rowidcolumn_;
    delete timestampcolumn_;
    delete entryidcolumn_;
}


DatabaseTable::TableStatus DatabaseTable::getTableStatus( SqlDB::Access& access,
						BufferString& errmsg ) const
{
    return checkTable( false, access, errmsg );
}


bool DatabaseTable::fixTable( SqlDB::Access& access, BufferString& errmsg) const
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

    mCrBackQuoteString( bqtblnm, tableName() );

    if ( !tables.isPresent(tableName()) )
    {
	if ( !fix )
	    return MinorError;

	mCrBackQuoteString( bqcolnm, rowidcolumn_->columnName() );

	BufferString querystring( "CREATE TABLE ", bqtblnm );
	querystring.add( " (" )
		   .add( rowidcolumn_->createColumnQuery() )
		   .add( " , PRIMARY KEY(" ).add( bqcolnm )
		   .add( ") )" );

	if ( !query.execute( querystring.buf() ) )
	{
	    errmsg = query.errMsg();
	    return AccessError;
	}
    }

    BufferString querystring( "SHOW COLUMNS IN ", bqtblnm );

    if ( !query.execute( querystring ) )
	{ errmsg = query.errMsg(); return AccessError; }

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

	    querystring.set( "ALTER TABLE " ).add( bqtblnm ).add( " ADD " );
	    querystring += columns_[idx]->createColumnQuery();
	    if ( !query.execute( querystring ) )
		{ errmsg = query.errMsg(); return AccessError; }

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


const char* DatabaseTable::rowIDSelectString() const
{ return rowidcolumn_->selectString(); }


bool DatabaseTable::parseRowID(const Query& q,int col, int& id) const
{ return rowidcolumn_->parse( q, col, id ); }



const char* DatabaseTable::entryIDSelectString() const
{ return entryidcolumn_->selectString(); }


bool DatabaseTable::parseEntryID(const Query& q,int col, int& id) const
{ return entryidcolumn_->parse( q, col, id ); }


const char* DatabaseTable::timeStampSelectString() const
{ return timestampcolumn_->selectString(); }


bool DatabaseTable::parseTimeStamp(const Query& q,int col, time_t& ts) const
{ return timestampcolumn_->parse( q, col, ts ); }



bool DatabaseTable::searchTable( Access& access,int entryid, bool onlylatest,
				 TypeSet<int>& rowids, BufferString& errmsg )
{
    //Either replacesid or id can be identical to entryid
    ValueCondition idcond( rowIDSelectString(),
            ValueCondition::Equals, toString(entryid) );
    ValueCondition entryidcond( entryidcolumn_->selectString(),
            ValueCondition::Equals, toString(entryid) );
    ValueCondition cond( idcond.getStr(),
            ValueCondition::Or, entryidcond.getStr() );

    SqlDB::Query query( access );


    BufferString condstring;
    if ( onlylatest )
    {
        BufferStringSet subcolumns;
        BufferString timestamp( "MAX(", timestampcolumn_->selectString(), ")" );
        query.addToColList( subcolumns, timestamp );
        const BufferString subquery( "(",
            query.select(subcolumns, tableName(), cond.getStr() ),
            ")" );

        SqlDB::ValueCondition latestcond( timestampcolumn_->selectString(),
                SqlDB::ValueCondition::Equals, subquery );

	SqlDB::ValueCondition combinedcomb ( latestcond.getStr(),
		ValueCondition::And, cond.getStr() );


        condstring = combinedcomb.getStr();
    }
    else
        condstring = cond.getStr();

    BufferStringSet columns;
    const int idcolidx
        = query.addToColList( columns, rowIDSelectString() );

    if ( !query.execute( query.select( columns, tableName(), condstring ) ) )
    {
        errmsg = query.errMsg();
        return false;
    }

    while ( query.next() )
    {
        int rowid;
        if ( !parseRowID( query, idcolidx, rowid ) )
            continue;

        rowids += rowid;
    }

    return true;

}


bool DatabaseTable::insertRow( Access& access,const BufferStringSet& cols,
			       const BufferStringSet& vals, int entryid,
			       int& rowid, BufferString& errmsg )
{
    SqlDB::Query query( access );
    BufferStringSet usedvals( vals );
    BufferStringSet usedcols( cols );

    usedvals.add( entryidcolumn_->dataString( entryid ) );
    usedcols.add( entryidcolumn_->columnName() );

    if ( !query.insert( usedcols, usedvals, tableName() ) )
	{ errmsg = query.errMsg(); return false; }

    mCrBackQuoteString( bqtblnm, tableName() );
    BufferString querystring = "SELECT LAST_INSERT_ID() AS ";
    querystring.add( IDDatabaseColumn::sKey() )
		.add( " FROM " ).add( bqtblnm )
		.add( " LIMIT 1" );

    if ( !query.execute(querystring) )
	{ errmsg = query.errMsg(); return false; }

    if ( query.next() )
    {
	const int newid = query.iValue( 0 );
	if ( newid<0 )
	    return false;

	rowid = newid;
	return true;
    }

    return false;
}

} // namespace SqlDB
