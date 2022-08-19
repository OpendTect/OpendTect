#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "databasemod.h"
#include "sets.h"
#include "stringview.h"
#include "sqlquery.h"
#include "convert.h"

class DateInfo;

namespace SqlDB
{

class DatabaseTable;
class Access;

/*!
\brief Base class for SQL Database columns.
*/

mExpClass(Database) DatabaseColumnBase
{
public:
			DatabaseColumnBase( DatabaseTable& dobj,
			    const char* columnname,const char* columntype );
    virtual		~DatabaseColumnBase()	{}

    virtual const char*	columnName() const	{ return columnname_; }
    virtual const char*	selectString() const;
    virtual const char*	columnType() const	{ return columntype_; }
    virtual const char*	columnOptions() const	{ return columnoptions_; }
    void		setColumnOptions(const char* n){ columnoptions_=n; }
    virtual bool	isDBTypeOK(const char*) const;
    virtual const char* createColumnQuery() const;

protected:

    DatabaseTable&	table_;

    BufferString	columnname_;
    BufferString	columntype_;
    BufferString	columnoptions_;
};


/*!
\brief SQL Database column
*/

#define mEnumDatabaseColumn( mod, clssnm, enmcls, enm )			\
mExpClass(mod) clssnm : public ::SqlDB::DatabaseColumnBase		\
{									\
public:									\
		clssnm( ::SqlDB::DatabaseTable& dobj,	\
				    const char* columnname )		\
		    : ::SqlDB::DatabaseColumnBase( dobj, columnname,	\
					  "VARCHAR(50)" )		\
		{}							\
									\
    bool	parse(const ::SqlDB::Query& q,int column,enmcls::enm& e) const \
		{ return enmcls::parseEnum( q.data(column).buf(), e ); }\
    const char*	dataString(const enmcls::enm& e) const			\
		{ return enmcls::toString( e ); }			\
}


/*!
\brief Template class for SQL Database column.
*/

template<class T>
mExpClass(Database) DatabaseColumn : public DatabaseColumnBase
{
public:
    inline		DatabaseColumn( DatabaseTable& dobj,
			    const char* columnname,const char* columntype );

    virtual inline bool	parse(const Query&,int column,T&) const;
    virtual inline const char*	dataString(const T&) const;
};


/*!
\brief SQL DatabaseColumn of IDs.
*/

mExpClass(Database) IDDatabaseColumn : public DatabaseColumn<int>
{
public:
		IDDatabaseColumn(DatabaseTable& dobj)
		    : DatabaseColumn<int>( dobj, sKey(), "INT(11)" )
		{ setColumnOptions("NOT NULL AUTO_INCREMENT"); }

    static const char*	sKey()	{ return "id"; }

    const char*	dataString(const int&) const { return 0; }
		//The id should be automatically inserted
};


/*!
\brief SQL DatabaseColumn of strings.
*/

mExpClass(Database) StringDatabaseColumn : public DatabaseColumn<BufferString>
{
public:
		StringDatabaseColumn( DatabaseTable& dobj,
			const char* columnname, int maxsize=-1);
};


/*!
\brief SQL DatabaseColumn of date and time.
*/

mExpClass(Database) CreatedTimeStampDatabaseColumn : public DatabaseColumnBase
{
public:
		CreatedTimeStampDatabaseColumn( DatabaseTable& dobj );
    const char*	selectString() const;
    bool	parse(const Query&,int column,time_t&) const;
    const char*	dataString(const time_t&) const { return 0; }
};


/*!
\brief A DatabaseColumn of DateInfo objects.
*/

mExpClass(Database) DateDatabaseColumn : public DatabaseColumnBase
{
public:
		DateDatabaseColumn( DatabaseTable& dobj,
				    const char* columnname );
    bool	parse( const Query&, int column, DateInfo& ) const;
    const char*	dataString(const DateInfo&) const;
};


/*!
\brief A Database where each row has a unique id. A row is never deleted, by a
new row is added where entryidcol is set to the id of the row it is replacing,
and a timestamp will tell which row that is the current.
*/

mExpClass(Database) DatabaseTable
{
public:
			DatabaseTable(const char* tablename);
			~DatabaseTable();

    enum TableStatus	{ OK, MinorError, MajorError, AccessError };
    TableStatus		getTableStatus(Access&, BufferString& errmsg) const;
			//!<Checks that all columns exist and are of right type
    bool		fixTable(Access&, BufferString& errmsg) const;

    virtual const char*	tableName() const { return tablename_; }

    const char*		rowIDSelectString() const;
    bool		parseRowID(const Query& q,int col, int& id) const;

    const char*		entryIDSelectString() const;
    bool		parseEntryID(const Query& q,int col, int& id) const;

    const char*		timeStampSelectString() const;
    bool		parseTimeStamp(const Query& q,int col, time_t&) const;

    bool		searchTable( Access&,int entryid, bool onlylatest,
				     TypeSet<int>& rowids,
				     BufferString& errmsg );

    bool		insertRow( Access&,const BufferStringSet& cols,
				const BufferStringSet& vals, int entryid,
				int& rowid, BufferString& errmsg );

protected:
    TableStatus		checkTable(bool fix,Access&,BufferString& errmsg) const;


    friend		class DatabaseColumnBase;

    const BufferString			tablename_;

    IDDatabaseColumn*			rowidcolumn_;
    CreatedTimeStampDatabaseColumn*	timestampcolumn_;
    DatabaseColumn<int>*		entryidcolumn_;
    ObjectSet<DatabaseColumnBase>	columns_;
};


template <class T> inline
DatabaseColumn<T>::DatabaseColumn( DatabaseTable& dobj,
    const char* columnname,const char* columntype )
    : DatabaseColumnBase( dobj, columnname, columntype )
{ }

#define mImplColumnSpecialization( type, func ) \
template <> inline \
bool DatabaseColumn<type>::parse( const Query& query, int column, \
					  type& val ) const  \
{ val = query.func( column ); return true; }


mImplColumnSpecialization( BufferString, data )
mImplColumnSpecialization( int, iValue )
mImplColumnSpecialization( unsigned int, uiValue )
mImplColumnSpecialization( od_int64, i64Value )
mImplColumnSpecialization( od_uint64, ui64Value )
mImplColumnSpecialization( float, fValue )
mImplColumnSpecialization( double, dValue )
mImplColumnSpecialization( bool, isTrue )

template <class T> inline
bool DatabaseColumn<T>::parse( const Query& query, int column, T& val ) const
{ Conv::set( val, query.data( column ) ); return !mIsUdf(val); }


template <class T> inline
const char* DatabaseColumn<T>::dataString( const T& val ) const
{
    return toString( val );
}

} // namespace SqlDB
