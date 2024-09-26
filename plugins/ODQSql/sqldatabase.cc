/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "sqldatabase.h"

#include "keystrs.h"
#include "settings.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QString>


// SqlDB::Access

SqlDB::AccessImpl::AccessImpl( const char* qtyp, const char* key )
    : Access(qtyp,key)
{
    qdb_ = new QSqlDatabase( QSqlDatabase::addDatabase(qtyp) );
}


SqlDB::AccessImpl::~AccessImpl()
{
    delete qdb_;
}


bool SqlDB::AccessImpl::isOK() const
{
    return qdb_;
}


bool SqlDB::AccessImpl::open()
{
    if ( !isOK() )
	return false;

    qdb_->setHostName( cd_.hostname_.buf() );
    qdb_->setDatabaseName( cd_.dbname_.buf() );
    qdb_->setUserName( cd_.username_.buf() );
    qdb_->setPassword( cd_.pwd_.buf() );
    if ( !mIsUdf(cd_.port_) )
	qdb_->setPort( cd_.port_ );

    return qdb_->open();
}


bool SqlDB::AccessImpl::isOpen() const
{
    return isOK() ? qdb_->isOpen() : false;
}


BufferString SqlDB::AccessImpl::errMsg() const
{
    if ( !isOK() )
	return BufferString( "Cannot get access for ", dbType() );

    BufferString err( qdb_->lastError().text() );
    err.trimBlanks();
    return err;
}


bool SqlDB::AccessImpl::commit()
{
    return isOK() ? qdb_->commit() : false;
}


void SqlDB::AccessImpl::close()
{
    if ( isOK() )
	qdb_->close();
}


// SqlDB::MySqlAccess

SqlDB::MySqlAccess::MySqlAccess( const char* key )
    : AccessImpl(MySqlAccess::sFactoryKeyword(),key)
{}


SqlDB::MySqlAccess::~MySqlAccess()
{}


// SqlDB::ODBCAccess

SqlDB::ODBCAccess::ODBCAccess( const char* key )
    : AccessImpl(ODBCAccess::sFactoryKeyword(),key)
{}


SqlDB::ODBCAccess::~ODBCAccess()
{}


bool SqlDB::ODBCAccess::open()
{
    QString str = QStringLiteral(
	"DRIVER={SQL Server};"
	"SERVER={%1};"
	"DATABASE={%2};"
	"UID={%3};"
	"PWD={%4};"
    ).arg( cd_.hostname_.buf() )
     .arg( cd_.dbname_.buf() )
     .arg( cd_.username_.buf() )
     .arg( cd_.pwd_.buf() );

    QSqlDatabase* qdb = qDataBase();
    qdb->setDatabaseName( str );
    if ( !mIsUdf(cd_.port_) )
	qdb->setPort( cd_.port_ );

    return qdb->open();
}
