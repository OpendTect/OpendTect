/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "sqldatabase.h"

#include "ascstream.h"
#include "bufstringset.h"
#include "keystrs.h"
#include "oddirs.h"
#include "settings.h"
#include <QString>

#ifndef OD_NO_QSQL

# include <QSqlDatabase>
# include <QSqlError>
# include <QSqlRecord>

#else

class mQSqlDatabase
{
public:

		mQSqlDatabase(int)		{}
    static int	addDatabase(const QString&)	{ return 0; }
    void	setHostName(const QString&)	{}
    void	setDatabaseName(const QString&)	{}
    void	setUserName(const QString&)	{}
    void	setPassword(const QString&)	{}
    void	setPort(int)			{}
    bool	open() const			{ return false; }
    struct ErrStruct { QString text() const	{ return "Dummy database"; } };
    ErrStruct	lastError()			{ return ErrStruct(); }
    bool	commit()			{ return false; }
    bool	isOpen() const			{ return false; }
    void	close()				{}

};

#endif


SqlDB::ConnectionData::ConnectionData( const char* key )
{
    if ( StringView(key).isEmpty() )
	return;

    const BufferString fnm = mGetSetupFileName( "SqlDB" );
    od_istream strm( fnm );
    if ( strm.isOK() )
    {
	IOPar dbiopar;
	ascistream astrm( strm );
	dbiopar.getFrom( astrm );

	ConstPtrMan<IOPar> iop = dbiopar.subselect( key );
	if ( iop && usePar(*iop) )
	    return;
    }

    Settings& settings = Settings::fetch( "sqldb" );
    settings.reRead();
    ConstPtrMan<IOPar> iop = settings.subselect( key );
    if ( iop )
	usePar( *iop );
}


void SqlDB::ConnectionData::fillPar( IOPar& iop ) const
{
    iop.update( sKey::Hostname(), hostname_ );
    iop.update( sKeyUserName(), username_ );
    iop.update( sKeyPassword(), pwd_ );
    iop.update( sKeyDBName(), dbname_ );
    iop.set( sKeyPort(), port_ );
}


bool SqlDB::ConnectionData::usePar( const IOPar& iop )
{
    hostname_ = iop.find( sKey::Hostname() );
    username_ = iop.find( sKeyUserName() );
    pwd_ = iop.find( sKeyPassword() );
    dbname_ = iop.find( sKeyDBName() );
    iop.get( sKeyPort(), port_ );

    return isOK();
}


SqlDB::Access::Access( const char* qtyp, const char* dbtyp )
    : cd_(dbtyp)
    , dbtype_(dbtyp)
{
    qdb_ = new mQSqlDatabase( mQSqlDatabase::addDatabase(qtyp) );
}


SqlDB::Access::~Access()
{
    delete qdb_;
}


bool SqlDB::Access::open()
{
    qdb_->setHostName( cd_.hostname_.buf() );
    qdb_->setDatabaseName( cd_.dbname_.buf() );
    qdb_->setUserName( cd_.username_.buf() );
    qdb_->setPassword( cd_.pwd_.buf() );
    if ( !mIsUdf(cd_.port_) )
	qdb_->setPort( cd_.port_ );

    return qdb_->open();
}


bool SqlDB::Access::isOpen() const
{
    return qdb_->isOpen();
}


BufferString SqlDB::Access::errMsg() const
{
    BufferString err( qdb_->lastError().text() );
    err.trimBlanks();
    return err;
}


bool SqlDB::Access::commit()
{
    return qdb_->commit();
}


void SqlDB::Access::close()
{
    qdb_->close();
}


void SqlDB::Access::getColumnNames( const char* tablename,
				    BufferStringSet& nms ) const
{
    QSqlRecord record = qdb_->record( tablename );
    const int nrrecords = record.count();
    for ( int idx= 0; idx<nrrecords; idx++ )
    {
	QString nm = record.fieldName( idx );
	nms.add( nm );
    }
}



// SqlDB::MySqlAccess
SqlDB::MySqlAccess::MySqlAccess( const char* key )
    : Access("QMYSQL",key)
{}


SqlDB::MySqlAccess::~MySqlAccess()
{}


// SqlDB::ODBCAccess
SqlDB::ODBCAccess::ODBCAccess( const char* key )
    : Access("QODBC",key)
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

    qdb_->setDatabaseName( str );
    if ( !mIsUdf(cd_.port_) )
	qdb_->setPort( cd_.port_ );

    return qdb_->open();
}
