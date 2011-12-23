/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Feb 2010
 RCS:           $Id: sqlquery.cc,v 1.5 2011-12-23 15:26:46 cvskris Exp $
________________________________________________________________________

-*/

#include "sqlquery.h"
#include "sqldatabase.h"
#include "bufstringset.h"
#include <QString>

#ifdef __have_qsql__

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#else

class mQSqlQuery
{
public:
    struct ValStruct
    {
	QString toString() const
	{ BufferString ret("S "); ret += idx(); return QString(ret.buf()); }
#	define mDeclQueryNumbFn(typ,fn) \
	typ fn() const			{ return (typ)idx(); }
	mDeclQueryNumbFn(int,toInt)
	mDeclQueryNumbFn(unsigned int,toUInt)
	mDeclQueryNumbFn(od_int64,toLongLong)
	mDeclQueryNumbFn(od_uint64,toULongLong)
	mDeclQueryNumbFn(double,toDouble)
	mDeclQueryNumbFn(bool,toBool)
    };
    struct ErrStruct
    {
	QString text() const
	{ BufferString ret("E "); ret += idx(); return QString(ret.buf()); }
    };

    		mQSqlQuery(const mQSqlDatabase&)	{}
    void	setHostName(const char*)		{}
    bool	exec(const char*)			{ return false; }
    bool	next() const				{ return false; }
    ValStruct	value(int) const			{ return ValStruct(); }
    ErrStruct	lastError()				{ return ErrStruct(); }
    bool	isActive() const			{ return false; }
    void	finish() const				{}

    static od_int64	retidx_;
    static od_int64	idx()				{ return retidx_++; }

};

od_int64 mQSqlQuery::retidx_ = 1;

#endif


SqlDB::Query::Query( SqlDB::Access& acc )
{
    qsqlquery_ = new mQSqlQuery( *(acc.qDataBase()) );
}


SqlDB::Query::~Query()
{
    delete qsqlquery_;
}


bool SqlDB::Query::execute( const char* querystr )
{
    return qsqlquery_->exec( querystr );
}


bool SqlDB::Query::next() const
{
    return qsqlquery_->next();
}


BufferString SqlDB::Query::data( int colid ) const
{
    return BufferString( qsqlquery_->value(colid).toString().toAscii().data() );
}


#define mDefQueryNumbFn(typ,valfn,qfn) \
typ SqlDB::Query::valfn( int columnid ) const \
{ \
    return qsqlquery_->value(columnid).qfn(); \
}

mDefQueryNumbFn(int,iValue,toInt)
mDefQueryNumbFn(unsigned int,uiValue,toUInt)
mDefQueryNumbFn(od_int64,i64Value,toLongLong)
mDefQueryNumbFn(od_uint64,ui64Value,toULongLong)
mDefQueryNumbFn(float,fValue,toDouble)
mDefQueryNumbFn(double,dValue,toDouble)
mDefQueryNumbFn(bool,isTrue,toBool)


BufferString SqlDB::Query::errMsg() const
{
    return BufferString( qsqlquery_->lastError().text().toAscii().data() );
}


bool SqlDB::Query::isActive() const
{
    return qsqlquery_->isActive();
}


void SqlDB::Query::finish() const
{
    if ( qsqlquery_->isActive() )
	qsqlquery_->finish();
}


BufferString SqlDB::Query::getCurrentDateTime()
{
    execute( "SELECT CURRENT_TIMESTAMP()" );
    BufferString datetime;
    while ( next() )
	datetime = data(0);

    return datetime;
}


bool SqlDB::Query::insert( const BufferStringSet& colnms,
			   const BufferStringSet& values,
			   const BufferString& tablenm )
{
    BufferString qstr = getInsertString( colnms, values, tablenm );
    return execute( qstr );
}


BufferString SqlDB::Query::getInsertString( const BufferStringSet& colnms,
					    const BufferStringSet& values,
					    const BufferString& tablenm )
{
    BufferString querystr;
    if ( colnms.size() != values.size() )
	return querystr;

    const int nrvals = values.size();
    querystr = "INSERT INTO "; querystr.add( tablenm ).add( " (" );
    for ( int idx=0; idx<nrvals; idx++ )
    {
	querystr.add( colnms[idx]->buf() );
	if ( idx != nrvals-1 )
	    querystr.add( "," );
    }

    querystr.add( ")" );

    querystr.add( " VALUES (" );

    for ( int idx=0; idx<nrvals; idx++ )
    {
	querystr.add( quoteString( values[idx]->buf(), '\'' ) );
	if ( idx != nrvals-1 )
	    querystr.add( "," );
    }

    querystr.add( ")" );

    return querystr;
}


int SqlDB::Query::addToColList(BufferStringSet& columns,const char* newcol)
{
    const int res = columns.size();
    columns.add( newcol );
    return res;
}


BufferString SqlDB::Query::getUpdateString( const BufferStringSet& colnms,
					    const BufferStringSet& values,
					    const BufferString& tablenm,
					    int bugid ) const
{
    BufferString querystr;
    if ( bugid<0 || colnms.size()!=values.size() )
	return querystr;

    const int nrvals = values.size();
    querystr = "UPDATE "; querystr.add( tablenm ).add( " SET " );
    for ( int idx=0; idx<nrvals; idx++ )
    {
	querystr.add( colnms[idx]->buf() ).add( "='" )
	    	.add( values[idx]->buf() )
		.add( idx != nrvals-1 ? "'," : "'" );
    }

    querystr.add( " WHERE id=" ).add( bugid );
    return querystr;
}


BufferString SqlDB::Query::select( const BufferStringSet& colnms,
				   const BufferString& tablenm, int id,
       				   const char* idkey )
{
    if ( !idkey )
	idkey = "`id`";

    BufferString querystr;
    if ( id < 0 )
	return querystr;

    const int nrvals = colnms.size();
    querystr.add( "SELECT " );
    for ( int idx=0; idx<nrvals; idx++ )
    {
	querystr.add( colnms[idx]->buf() );
	querystr.add( idx != nrvals-1 ? "," : " " );
    }

    querystr.add( "FROM " ).add( tablenm ).add( " WHERE " ).add( idkey ).add( "=" ).add( id );
    return querystr;
}


bool SqlDB::Query::update( const BufferStringSet& colnms,
			   const BufferStringSet& values,
			   const BufferString& tablenm, int bugid )
{
    BufferString qstr = getUpdateString( colnms, values, tablenm, bugid );
    return execute( qstr );
}


bool SqlDB::Query::deleteInfo( const char* tablenm, const char* fieldnm,
			       int id )
{
    BufferString qstr( "DELETE FROM " );
    qstr.add( tablenm ).add( " WHERE " ).add( fieldnm ).add( "=" ).add( id );
    return execute( qstr );
}
