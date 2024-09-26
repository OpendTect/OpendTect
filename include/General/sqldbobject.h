#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"

#include "convert.h"
#include "sets.h"
#include "sqldbaccess.h"
#include "stringview.h"

class DateInfo;

namespace SqlDB
{

class Access;
class DatabaseTable;

/*!
\brief Credentials to connect to a Database.
*/

mExpClass(General) ConnectionData
{
public:

			ConnectionData(const char* dbtype=nullptr);
			~ConnectionData();

    bool		isOK() const;

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);	//!< returns isOK()

    BufferString	hostname_;
    PortNr_Type		port_			= mUdf(PortNr_Type);
    BufferString	username_;
    BufferString	pwd_;
    BufferString	dbname_;

    static const char*	sKeyUserName()		{ return "Username"; }
    static const char*	sKeyPassword()		{ return "Password"; }
    static const char*	sKeyPort()		{ return "Port"; }
    static const char*	sKeyDBName()		{ return "Database"; }

};


/*!
\brief Base class for SQL Database columns.
*/

mExpClass(General) DatabaseColumnBase
{
public:
    virtual		~DatabaseColumnBase();

    virtual const char*	columnName() const	{ return columnname_; }
    virtual const char*	selectString() const;
    virtual const char*	columnType() const	{ return columntype_; }
    virtual const char*	columnOptions() const	{ return columnoptions_; }
    void		setColumnOptions(const char* n){ columnoptions_=n; }
    virtual bool	isDBTypeOK(const char*) const;
    virtual const char* createColumnQuery() const;

protected:
			DatabaseColumnBase(DatabaseTable&,
				const char* columnname,const char* columntype);

    DatabaseTable&	table_;

    BufferString	columnname_;
    BufferString	columntype_;
    BufferString	columnoptions_;
};


/*!
\brief Template class for SQL Database column.
*/

template<class T>
mExpClass(General) DatabaseColumn : public DatabaseColumnBase
{
public:
    inline		DatabaseColumn(DatabaseTable&,const char* columnname,
				       const char* columntype);
    inline		~DatabaseColumn();

    virtual inline bool parse(const QueryAccess&,int column,T&) const;
    virtual inline const char*	dataString(const T&) const;
};


/*!
\brief SQL DatabaseColumn of IDs.
*/

mExpClass(General) IDDatabaseColumn : public DatabaseColumn<int>
{
public:
		IDDatabaseColumn(DatabaseTable& dobj)
		    : DatabaseColumn<int>( dobj, sKey(), "INT(11)" )
		{ setColumnOptions("NOT NULL AUTO_INCREMENT"); }

    static const char*	sKey()	{ return "id"; }

    const char* dataString(const int&) const override { return nullptr; }
		//The id should be automatically inserted
};


/*!
\brief SQL DatabaseColumn of strings.
*/

mExpClass(General) StringDatabaseColumn : public DatabaseColumn<BufferString>
{
public:
		StringDatabaseColumn(DatabaseTable&,const char* columnname,
				     int maxsize=-1);
		~StringDatabaseColumn();
};


/*!
\brief SQL DatabaseColumn of date and time.
*/

mExpClass(General) CreatedTimeStampDatabaseColumn : public DatabaseColumnBase
{
public:
		CreatedTimeStampDatabaseColumn(DatabaseTable&);
		~CreatedTimeStampDatabaseColumn();

    const char*	selectString() const override;
    bool	parse(const QueryAccess&,int column,time_t&) const;
    const char* dataString(const time_t&) const { return nullptr; }
};


/*!
\brief A DatabaseColumn of DateInfo objects.
*/

mExpClass(General) DateDatabaseColumn : public DatabaseColumnBase
{
public:
		DateDatabaseColumn(DatabaseTable&,const char* columnname);
		~DateDatabaseColumn();

    bool	parse(const QueryAccess&,int column,DateInfo&) const;
    const char*	dataString(const DateInfo&) const;
};


/*!
\brief A Database where each row has a unique id. A row is never deleted, by a
new row is added where entryidcol is set to the id of the row it is replacing,
and a timestamp will tell which row that is the current.
*/

mExpClass(General) DatabaseTable
{
public:
			DatabaseTable(const char* tablename);
			~DatabaseTable();

    enum TableStatus	{ OK, MinorError, MajorError, AccessError };
    TableStatus		getTableStatus(Access&,BufferString& errmsg) const;
			//!<Checks that all columns exist and are of right type
    bool		fixTable(Access&,BufferString& errmsg) const;

    virtual const char*	tableName() const { return tablename_; }

    const char*		rowIDSelectString() const;
    bool		parseRowID(const QueryAccess&,int col,int& id) const;

    const char*		entryIDSelectString() const;
    bool		parseEntryID(const QueryAccess&,int col,int& id) const;

    const char*		timeStampSelectString() const;
    bool		parseTimeStamp(const QueryAccess&,int col,
				       time_t&) const;

    bool		searchTable(Access&,int entryid,bool onlylatest,
				    TypeSet<int>& rowids,
				    BufferString& errmsg);

    bool		insertRow(Access&,const BufferStringSet& cols,
				const BufferStringSet& vals,int entryid,
				int& rowid,BufferString& errmsg );

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
{}


template <class T> inline
DatabaseColumn<T>::~DatabaseColumn()
{}


#define mImplColumnSpecialization( type, func ) \
template <> inline \
bool DatabaseColumn<type>::parse( const QueryAccess& query, int column, \
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
bool DatabaseColumn<T>::parse( const QueryAccess& query, int column,
			       T& val ) const
{
    Conv::set( val, query.data(column) );
    return !mIsUdf(val);
}


template <class T> inline
const char* DatabaseColumn<T>::dataString( const T& val ) const
{
    return toString( val );
}

} // namespace SqlDB
