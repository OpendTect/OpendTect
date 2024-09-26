#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "odqsqlmod.h"

#include "sqldbaccess.h"

class QSqlQuery;

/*!\brief SQL Database */

namespace SqlDB
{

/*!
\brief Execution of SQL Query.
*/

mExpClass(ODQSql) Query : public QueryAccess
{
public:
			Query(Access&);
			~Query();

    bool		isOK() const override;

    bool		execute(const char*);
    bool		next() const;
    bool		isActive() const;
    BufferString	errMsg() const;
    void		finish() const;

    int			size() const;
    BufferString	data(int) const;
    int			iValue(int) const;
    unsigned int	uiValue(int) const;
    od_int64		i64Value(int) const;
    od_uint64		ui64Value(int) const;
    float		fValue(int) const;
    double		dValue(int) const;
    bool		isTrue(int) const;

private:

    QSqlQuery*		qsqlquery_;

};


mExpClass(ODQSql) QueryProviderImpl : public QueryProvider
{
public:

    mDefaultFactoryInstantiation( QueryProvider, QueryProviderImpl,
				  "OD", toUiString("OD") );

private:

    QueryAccess*    getQuery(Access&) const override;

public:

    static void     initODSqlDB(); //!< class initClass()
};

} // namespace SqlDB
