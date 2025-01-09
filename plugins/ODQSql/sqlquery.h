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

    bool		execute(const char*) override;
    bool		next() const override;
    bool		isActive() const override;
    BufferString	errMsg() const override;
    void		finish() const override;

    int			size() const override;
    bool		isNull(int) const override;
    BufferString	data(int) const override;
    int			iValue(int) const override;
    unsigned int	uiValue(int) const override;
    od_int64		i64Value(int) const override;
    od_uint64		ui64Value(int) const override;
    float		fValue(int) const override;
    double		dValue(int) const override;
    bool		isTrue(int) const override;

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
