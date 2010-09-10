/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Feb 2010
 RCS:           $Id: sqldatabase.cc,v 1.1 2010-09-10 13:26:03 cvsbert Exp $
________________________________________________________________________

-*/

#include "sqldatabase.h"
#include "bufstring.h"
#include "bufstringset.h"
#include <QString>

#ifdef __have_sql__

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#else

class mQSqlDatabase
{
public:

    		mQSqlDatabase(int)	{}
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

class mQSqlQuery
{
public:

    		mQSqlQuery(const mQSqlDatabase&)	{}
    void	setHostName(const char*)		{}
    bool	exec(const char*)			{ return false; }
    bool	next() const				{ return false; }
    struct ValStruct { QString toString() const	{ return "Dummy value"; } };
    ValStruct	value(int) const			{ return ValStruct(); }
    struct ErrStruct { QString text() const	{ return "Dummy database"; } };
    ErrStruct	lastError()				{ return ErrStruct(); }
    bool	isActive() const			{ return false; }
    void	finish() const				{}

};

#endif


//SqlDataBase
SqlDataBase::SqlDataBase( const char* database )
{
    qsqldatabase_ = new mQSqlDatabase( mQSqlDatabase::addDatabase(database) );
}


SqlDataBase::~SqlDataBase()
{
    delete qsqldatabase_;
}

void SqlDataBase::setHostName( const char* hostname )
{
    qsqldatabase_->setHostName( hostname );
}


void SqlDataBase::setDatabaseName( const char* dbname )
{
    qsqldatabase_->setDatabaseName( dbname );
}


void SqlDataBase::setUserName( const char* username )
{
    qsqldatabase_->setUserName( username );
}


void SqlDataBase::setPassword( const char* passwd )
{
    qsqldatabase_->setPassword( passwd );
}


void SqlDataBase::setPort( int port )
{
    qsqldatabase_->setPort( port );
}


bool SqlDataBase::open()
{
    return qsqldatabase_->open();
}


bool SqlDataBase::isOpen() const
{
    return qsqldatabase_->isOpen();
}


BufferString SqlDataBase::errorMsg() const
{
    QString qerror = qsqldatabase_->lastError().text();
    BufferString error = qerror.toAscii().data();
    return error;
}


bool SqlDataBase::commit()
{
    return qsqldatabase_->commit();
}


void SqlDataBase::close() const
{
    qsqldatabase_->close();
}


//MySqlDataBase
MySqlDataBase::MySqlDataBase()
    : SqlDataBase("QMYSQL")
{
}


//Query
Query::Query( SqlDataBase& db )
{
    qsqlquery_ = new mQSqlQuery( *(db.qDataBase()) );
}


Query::~Query()
{
    delete qsqlquery_;
}


bool Query::execute( const char* querystr )
{
    return qsqlquery_->exec( querystr );
}


bool Query::next() const
{
    return qsqlquery_->next();
}


BufferString Query::data( int columnid ) const
{
    QString record = qsqlquery_->value(columnid).toString();
    BufferString datastr( record.toAscii().data() );
    return datastr;
}


BufferString Query::errorMsg() const
{
    QString qerror = qsqlquery_->lastError().text();
    BufferString error = qerror.toAscii().data();
    return error;
}


bool Query::isActive() const
{
    return qsqlquery_->isActive();
}


void Query::finish() const
{
    if ( qsqlquery_->isActive() )
	qsqlquery_->finish();
}


BufferString Query::getCurrentDateTime()
{
    execute( "SELECT CURRENT_TIMESTAMP()" );
    BufferString datetime;
    while ( next() )
	datetime = data(0);

    return datetime;
}


bool Query::insert( const BufferStringSet& colnms,
                    const BufferStringSet& values,
                    const BufferString& tablenm )
{
    BufferString qstr = getInsertString( colnms, values, tablenm );
    return execute( qstr );
}


BufferString Query::getInsertString( const BufferStringSet& colnms,
				     const BufferStringSet& values,
				     const BufferString& tablenm ) const
{
    BufferString querystr;
    if ( colnms.size() != values.size() )
	return querystr;

    const int nrvals = values.size();
    querystr = "INSERT INTO "; querystr.add( tablenm ).add( " (" );
    for ( int idx=0; idx<nrvals; idx++ )
    {
	querystr.add( colnms[idx]->buf() )
		.add( idx != nrvals-1 ? "," : ")" );
    }

    querystr.add( " VALUES (" );

    for ( int idx=0; idx<nrvals; idx++ )
    {
	querystr.add( "'" ).add( values[idx]->buf() )
	    	.add( idx != nrvals-1 ? "'," : "' )" );
    }

    return querystr;
}


BufferString Query::getUpdateString( const BufferStringSet& colnms,
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


BufferString Query::select( const BufferStringSet& colnms,
			    const BufferString& tablenm, int id )
{
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

    querystr.add( "FROM " ).add( tablenm ).add( " WHERE id=" ).add( id );

    return querystr;
}


bool Query::update( const BufferStringSet& colnms,
		    const BufferStringSet& values,
		    const BufferString& tablenm, int bugid )
{
    BufferString qstr = getUpdateString( colnms, values, tablenm, bugid );
    return execute( qstr );
}
