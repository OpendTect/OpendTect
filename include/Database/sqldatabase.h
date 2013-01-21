#ifndef sqldatabase_h
#define sqldatabase_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Feb 2010
 RCS:           $Id$
________________________________________________________________________

-*/

#include "databasemod.h"
#include "bufstring.h"
class IOPar;

#ifdef __have_qsql__
# define mQSqlDatabase QSqlDatabase
#else
# define mQSqlDatabase dummyQSqlDatabase
#endif

class mQSqlDatabase;
class BufferStringSet;


namespace SqlDB
{

/*!
\ingroup Database
\brief Credentials to connect to a Database.
*/

mExpClass(Database) ConnectionData
{
public:

    			ConnectionData(const char* dbtype=0);

    bool		isOK() const
    			{ return !dbname_.isEmpty()
			       && !pwd_.isEmpty() && !username_.isEmpty(); }

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);	//!< returns isOK()

    BufferString	hostname_;
    int			port_;
    BufferString	username_;
    BufferString	pwd_;
    BufferString	dbname_;

    static const char*	sKeyHostName()		{ return "Hostname"; }
    static const char*	sKeyUserName()		{ return "Username"; }
    static const char*	sKeyPassword()		{ return "Password"; }
    static const char*	sKeyPort()		{ return "Port"; }
    static const char*	sKeyDBName()		{ return "Database"; }

};


/*!
\ingroup Database
\brief To access a connected Database.
*/

mExpClass(Database) Access
{
public:

    virtual		~Access();

    ConnectionData&	connectionData()		{ return cd_; }
    const ConnectionData& connectionData() const	{ return cd_; }

    bool		open();
    bool		commit();
    BufferString	errMsg() const;

    bool		isOpen() const;
    void		close();
    const char*		dbType() const		{ return dbtype_; }

protected:

    			Access(const char* qtyp,const char* dbtype);

    mQSqlDatabase*	qdb_;
    ConnectionData	cd_;
    BufferString	dbtype_;

public:

    mQSqlDatabase*	qDataBase()	{ return qdb_; }

};


/*!
\ingroup Database
\brief Access to a connected MySql Database.
*/

mExpClass(Database) MySqlAccess : public Access
{
public:
    			MySqlAccess( const char* dbtype )
			    : Access("QMYSQL",dbtype)	{}
};

} // namespace

#endif

