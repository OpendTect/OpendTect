#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "odqsqlmod.h"

#include "sqldbaccess.h"
#include "sqldbobject.h"

class QSqlDatabase;

namespace SqlDB
{

/*!
\brief To access a connected Database.
*/

mExpClass(ODQSql) AccessImpl : public Access
{
public:

			~AccessImpl();

    bool		open() override;
    bool		commit() override;
    void		close() override;

    bool		isOK() const override;
    bool		isOpen() const override;
    BufferString	errMsg() const override;

protected:
			AccessImpl(const char* qtyp,const char* dbtype);

private:

    QSqlDatabase*	qdb_;

public:

    QSqlDatabase*	qDataBase()	{ return qdb_; }

};


/*!
\brief Access to a connected MySql Database.
*/

mExpClass(ODQSql) MySqlAccess : public AccessImpl
{
public:
    mDefaultFactoryInstantiation1Param(Access,MySqlAccess,const char*,
				       "QMYSQL",toUiString("Qt::MySQL"));
			~MySqlAccess();

private:
			MySqlAccess(const char*);
};


/*!
\brief Access to a connected ODBC Database.

Open Database Connectivity (ODBC)
*/

mExpClass(ODQSql) ODBCAccess : public AccessImpl
{
public:
    mDefaultFactoryInstantiation1Param(Access,ODBCAccess,const char*,
				       "QODBC",toUiString("QODBC"));
			~ODBCAccess();

private:
			ODBCAccess(const char*);

    bool		open() override;
};

} // namespace SqlDB
