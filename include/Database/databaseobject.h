#ifndef databaseobject_h
#define databaseobject_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer
 Date:		Nov 2011
 RCS:		$Id$
________________________________________________________________________

-*/

#include "sets.h"
#include "fixedstring.h"
#include "sqlquery.h"

namespace SqlDB
{
class DatabaseTable;
class Access;

mClass DatabaseColumnBase
{
public:
    			DatabaseColumnBase( DatabaseTable& dobj,
			    const char* columnname,const char* columntype );

    virtual const char*	columnName() const	{ return columnname_; }
    virtual const char*	selectString() const;
    virtual const char*	columnType() const	{ return columntype_; }
    virtual const char*	columnOptions() const	{ return columnoptions_; }
    void 		setColumnOptions(const char* n){ columnoptions_=n; }
    virtual bool	isDBTypeOK(const char*) const;
    virtual const char* createColumnQuery() const;

protected:

    DatabaseTable&	table_;

    BufferString	columnname_;
    BufferString	columntype_;
    BufferString	columnoptions_;
};



template<class T>
mClass DatabaseColumn : public DatabaseColumnBase
{
public:
    inline		DatabaseColumn( DatabaseTable& dobj,
			    const char* columnname,const char* columntype );

    virtual inline bool	parse(const Query&,int column,T&) const;
    virtual inline const char*	dataString(const T&) const;
};


mClass IDDatabaseColumn : public DatabaseColumn<int>
{
public:
    		IDDatabaseColumn(DatabaseTable& dobj)
		    : DatabaseColumn<int>( dobj, sKey(), "INT(11)" )
		{ setColumnOptions("NOT NULL AUTO_INCREMENT"); }

    static const char*	sKey()	{ return "id"; }

    const char*	dataString(const int&) const { return 0; }
    		//The id should be automatically inserted
};


mClass StringDatabaseColumn : public DatabaseColumn<BufferString>
{
public:
    		StringDatabaseColumn( DatabaseTable& dobj,
			const char* columnname, int maxsize=-1);
};


mClass CreatedTimeStampDatabaseColumn : public DatabaseColumn<od_int64>
{
public:
    		CreatedTimeStampDatabaseColumn( DatabaseTable& dobj );
    const char*	selectString() const;
    bool	parse(const Query&,int column,od_int64&) const;
    const char*	dataString(const od_int64&) const { return 0; }
};


mClass DatabaseTable
{
public:
    			DatabaseTable(const char* tablename);
    			~DatabaseTable();

    enum TableStatus	{ OK, MinorError, MajorError, AccessError };
    TableStatus		getTableStatus(Access&, BufferString& errmsg) const;
    			//!<Checks that all columns exist and are of right type
    bool		fixTable(Access&, BufferString& errmsg) const;

    virtual const char*	tableName() const { return tablename_; }

    const char*		idColumnName() const;
    const char*		idSelectString() const;
    bool		parseID(const Query& q,int col, int& id) const;

protected:
    TableStatus		checkTable(bool fix,Access&,BufferString& errmsg) const;


    friend		class DatabaseColumnBase;

    const BufferString			tablename_;

    IDDatabaseColumn*			idcolumn_;
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


} //namespace


#endif
