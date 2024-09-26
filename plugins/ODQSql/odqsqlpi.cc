/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "odplugin.h"

#include "sqldatabase.h"
#include "sqlquery.h"

mDefODPluginEarlyLoad(ODQSql)
mDefODPluginInfo(ODQSql)
{
    static PluginInfo retpi( "ODQSql (Base)",
			     "Implementations of 'Qt SQL' for OpendTect" );
    return &retpi;
}


const char* initODQSqlPlugin()
{
    SqlDB::MySqlAccess::initClass();
    SqlDB::ODBCAccess::initClass();
    SqlDB::QueryProviderImpl::initODSqlDB();

    return nullptr;
}


mDefODInitPlugin(ODQSql)
{
    return initODQSqlPlugin();
}
