/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "sqlquery.h"

#include "keystrs.h"
#include "sqldatabase.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QVariant>


// SqlDB::Query

SqlDB::Query::Query( Access& acc )
    : QueryAccess()
{
    mDynamicCastGet(AccessImpl*,accimpl,&acc)
    if ( accimpl )
	qsqlquery_ = new QSqlQuery( *(accimpl->qDataBase()) );
}


SqlDB::Query::~Query()
{
    delete qsqlquery_;
}


bool SqlDB::Query::isOK() const
{
    return qsqlquery_;
}


bool SqlDB::Query::execute( const char* querystr )
{
    return qsqlquery_->exec( querystr );
}


bool SqlDB::Query::next() const
{
    return qsqlquery_->next();
}


int SqlDB::Query::size() const
{
    return qsqlquery_->size();
}


bool SqlDB::Query::isNull( int column ) const
{
    return qsqlquery_->isNull( column );
}


BufferString SqlDB::Query::data( int column ) const
{
    return BufferString( qsqlquery_->value(column).toString() );
}


#define mDefQueryNumbFn(typ,valfn,qfn) \
typ SqlDB::Query::valfn( int column ) const \
{ \
    return qsqlquery_->value(column).qfn(); \
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
    return BufferString( qsqlquery_->lastError().text().toLatin1().data() );
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


// SqlDB::QueryProviderImpl

SqlDB::QueryAccess* SqlDB::QueryProviderImpl::getQuery( Access& acc ) const
{
    PtrMan<QueryAccess> query = new Query( acc );
    if ( !query || !query->isOK() )
	return nullptr;

    return query.release();
}


void SqlDB::QueryProviderImpl::initODSqlDB()
{
    initClass();
    const int defidx = factory().getNames().indexOf( sFactoryKeyword() );
    factory().setDefaultName( defidx );
}
