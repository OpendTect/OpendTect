/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Feb 2010
 RCS:           $Id$
________________________________________________________________________

-*/

#include "sqldatabase.h"
#include "settings.h"
#include <QString>

#ifdef __have_qsql__

#include <QSqlDatabase>
#include <QSqlError>

#else

class mQSqlDatabase
{
public:

    		mQSqlDatabase(int)		{}
    static int	addDatabase(const char*)	{ return 0; }
    void	setHostName(const char*)	{}
    void	setDatabaseName(const char*)	{}
    void	setUserName(const char*)	{}
    void	setPassword(const char*)		{}
    void	setPort(int)			{}
    bool	open() const			{ return false; }
    struct ErrStruct { QString text() const	{ return "Dummy database"; } };
    ErrStruct	lastError()			{ return ErrStruct(); }
    bool	commit()			{ return false; }
    bool	isOpen() const			{ return false; }
    void	close()				{}

};

#endif


SqlDB::ConnectionData::ConnectionData( const char* dbtype )
{
    if ( !dbtype || !*dbtype ) return;

    IOPar* iop = Settings::fetch("DB").subselect( dbtype );
    if ( iop )
	usePar( *iop );
}


void SqlDB::ConnectionData::fillPar( IOPar& iop ) const
{
#define mSetInPar(memb,ky) \
    if ( memb##_.isEmpty() ) \
        iop.removeWithKey( sKey##ky() ); \
    else \
	iop.set( sKey##ky(), memb##_ )

    mSetInPar(hostname,HostName);
    mSetInPar(username,UserName);
    mSetInPar(pwd,Password);
    mSetInPar(dbname,DBName);
    iop.set( sKeyPort(), port_ );
}


bool SqlDB::ConnectionData::usePar( const IOPar& iop )
{
#define mGetFromPar(memb,ky) \
    memb##_ = iop.find( sKey##ky() )
    mGetFromPar(hostname,HostName);
    mGetFromPar(username,UserName);
    mGetFromPar(pwd,Password);
    mGetFromPar(dbname,DBName);
    iop.get( sKeyPort(), port_ );

    return isOK();
}


SqlDB::Access::Access( const char* qtyp, const char* dbtyp )
    : dbtype_(dbtyp)
    , cd_(dbtyp)
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
    qdb_->setPort( cd_.port_ );
    return qdb_->open();
}


bool SqlDB::Access::isOpen() const
{
    return qdb_->isOpen();
}


BufferString SqlDB::Access::errMsg() const
{
    BufferString err( qdb_->lastError().text().toAscii().data() );
    removeTrailingBlanks( err.buf() );
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
