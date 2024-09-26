/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "sqldbaccess.h"

#include "bufstringset.h"
#include "keystrs.h"
#include "sqldbobject.h"

mImplFactory1Param( SqlDB::Access, const char*, SqlDB::Access::factory );
mImplFactory( SqlDB::QueryProvider, SqlDB::QueryProvider::factory );

static const char* parenstart = " ( ";
static const char* parenend = " ) ";

DefineEnumNames(SqlDB::ValueCondition,Operator,0,"Operators" )
{ "=", "<", ">", "<=", ">=", "!=",
  "IS NULL", "IS NOT NULL", "OR", "AND", nullptr };


// SqlDB::Access

SqlDB::Access::Access( const char* qtyp, const char* key )
    : cd_(*new ConnectionData(key))
    , dbtype_(key)
{
}


SqlDB::Access::~Access()
{
    delete &cd_;
}


// SqlDB::QueryAccess

SqlDB::QueryAccess::QueryAccess()
{
}


SqlDB::QueryAccess::~QueryAccess()
{
}


BufferString SqlDB::QueryAccess::getCurrentDateTime()
{
    if ( !execute("SELECT CURRENT_TIMESTAMP()") )
	return BufferString::empty();

    BufferString datetime;
    while ( next() )
	datetime = data(0);

    return datetime;
}


bool SqlDB::QueryAccess::startTransaction()
{
    return execute( "START TRANSACTION" );
}


bool SqlDB::QueryAccess::commit()
{
    return execute( "COMMIT" );
}


bool SqlDB::QueryAccess::rollback()
{
    return execute( "ROLLBACK" );
}


bool SqlDB::QueryAccess::insert( const BufferStringSet& colnms,
				 const BufferStringSet& values,
				 const BufferString& tablenm )
{
    const BufferString qstr = getInsertString( colnms, values, tablenm );
    return execute( qstr.buf() );
}


BufferString SqlDB::QueryAccess::getInsertString( const BufferStringSet& colnms,
						  const BufferStringSet& values,
						  const BufferString& tablenm )
{
    BufferString querystr;
    if ( colnms.size() != values.size() )
	return querystr;

    const int nrvals = values.size();
    querystr.set( "INSERT INTO " ).add( tablenm ).add( parenstart );
    for ( int idx=0; idx<nrvals; idx++ )
    {
	querystr.add( colnms[idx]->buf() );
	if ( idx != nrvals-1 )
	    querystr.add( "," );
    }

    querystr.add( parenend );

    querystr.add( " VALUES (" );
    for ( int idx=0; idx<nrvals; idx++ )
    {
	BufferString str( values.get( idx ) );
	if ( str.isEmpty() )
	    querystr.add( "''" );
	else
	{
	    str.setBufSize( 2*str.size()+2 );
	    str.replace( "\\", "\\\\" );
	    str.replace( "'", "\\'" );
	    querystr.add( "'" ).add ( str ).add( "'" );
	}

	if ( idx != nrvals-1 )
	    querystr.add( "," );
    }

    querystr.add( parenend );

    return querystr;
}


int SqlDB::QueryAccess::addToColList( BufferStringSet& columns,
				      const char* newcol )
{
    const int res = columns.size();
    columns.add( newcol );
    return res;
}


BufferString SqlDB::QueryAccess::getUpdateString( const BufferStringSet& colnms,
					    const BufferStringSet& values,
					    const BufferString& tablenm,
					    int id ) const
{
    BufferString querystr;
    if ( id<0 || colnms.size()!=values.size() )
	return querystr;

    const int nrvals = values.size();
    querystr = "UPDATE "; querystr.add( tablenm ).add( " SET " );
    for ( int idx=0; idx<nrvals; idx++ )
    {
	querystr.add( colnms[idx]->buf() ).add( "='" )
		.add( values[idx]->buf() )
		.add( idx != nrvals-1 ? "'," : "'" );
    }

    querystr.add( " WHERE id=" ).add( id );
    return querystr;
}


BufferString SqlDB::QueryAccess::select( const BufferStringSet& colnms,
					 const BufferString& tablenm,
					 const char* condstr )
{
    BufferString querystr;

    const int nrvals = colnms.size();
    querystr.add( "SELECT " );
    for ( int idx=0; idx<nrvals; idx++ )
    {
	querystr.add( colnms[idx]->buf() );
	querystr.add( idx != nrvals-1 ? "," : sKey::SpaceString() );
    }

    querystr.add( "FROM " ).add( tablenm );
    if ( condstr )
	querystr.add( " WHERE " ).add( condstr );

    return querystr;
}


bool SqlDB::QueryAccess::update( const BufferStringSet& colnms,
				 const BufferStringSet& values,
				 const BufferString& tablenm, int id )
{
    const BufferString qstr = getUpdateString( colnms, values, tablenm, id );
    return execute( qstr.buf() );
}


bool SqlDB::QueryAccess::deleteInfo( const char* tablenm, const char* fieldnm,
				     int id )
{
    BufferString qstr( "DELETE FROM " );
    qstr.add( tablenm ).add( " WHERE " ).add( fieldnm ).add( "=" ).add( id );
    return execute( qstr.buf() );
}


// SqlDB::Condition

SqlDB::Condition::Condition()
{
}


SqlDB::Condition::~Condition()
{
}


// SqlDB::ValueCondition

SqlDB::ValueCondition::ValueCondition( const char* key, Operator op,
				       const char* val )
    : Condition()
    , col_(key)
    , op_( op )
    , val_(val)
{
}


SqlDB::ValueCondition::~ValueCondition()
{
}


BufferString SqlDB::ValueCondition::getStr() const
{
    BufferString res( parenstart );
    res.add ( col_ );
    res.add( sKey::SpaceString() ).add( toString( op_ ) )
       .add( sKey::SpaceString() ).add( val_ ).add( parenend );
    return res;
}


// SqlDB::MultipleLogicCondition

SqlDB::MultipleLogicCondition::MultipleLogicCondition( bool isand )
    : Condition()
    , isand_(isand)
{
}


SqlDB::MultipleLogicCondition::~MultipleLogicCondition()
{
}


SqlDB::MultipleLogicCondition&
SqlDB::MultipleLogicCondition::addStatement( const char* stmnt )
{
    statements_.add( stmnt );
    return *this;
}


BufferString SqlDB::MultipleLogicCondition::getStr() const
{
    const char* op = ValueCondition::toString( isand_
			    ? ValueCondition::Operator::And
			    : ValueCondition::Operator::Or );

    BufferString res( parenstart );
    for ( int idx=0; idx<statements_.size(); idx++ )
    {
	res.add( parenstart ).add( statements_[idx]->buf() ).add( parenend );
	if ( idx!=statements_.size()-1 )
	{
	    res.add( op );
	}
    }

    res.add( parenend );

    return res;
}


// SqlDB::StringCondition

SqlDB::StringCondition::StringCondition( const char* col,
					 const char* searchstr,
					 bool exact )
    : Condition()
    , col_( col )
    , searchstr_( searchstr )
    , exact_( exact )
{
}


SqlDB::StringCondition::~StringCondition()
{
}


BufferString SqlDB::StringCondition::getStr() const
{
    BufferString res( col_ );
    res.add( " LIKE( '" );
    if ( !exact_ )
	res.add( "%");
    res.add( searchstr_ );
    if ( !exact_ )
	res.add( "%" );
    res.add( "' )" );
    return res;
}


// SqlDB::FullTextCondition

SqlDB::FullTextCondition::FullTextCondition( BufferStringSet& cols,
					     const char* searchstr )
    : Condition()
    , cols_( cols )
    , searchstr_( searchstr )
{
}


SqlDB::FullTextCondition::FullTextCondition( const char* searchstr )
    : Condition()
    , searchstr_( searchstr )
{
}


SqlDB::FullTextCondition::~FullTextCondition()
{
}


SqlDB::FullTextCondition& SqlDB::FullTextCondition::addColumn( const char* col )
{
    cols_.add( col );
    return *this;
}


BufferString SqlDB::FullTextCondition::getStr() const
{
    BufferString res( "MATCH( ");
    for ( int idx=0; idx<cols_.size(); idx++ )
    {
	res.add( cols_[idx]->buf() );
	if ( idx!=cols_.size()-1 )
	    res.add( ", " );
    }

    res.add( " ) AGAINST ( ").add( searchstr_ ).add( " )" );
    return res;
}


// SqlDB::QueryProvider

SqlDB::QueryProvider* SqlDB::QueryProvider::mkProv( int idx )
{
    const FactoryBase& sqldbfact = factory();
    if ( sqldbfact.isEmpty() )
	return nullptr;

    if ( idx<0 || idx>=sqldbfact.size() )
	idx = sqldbfact.getNames().indexOf( sqldbfact.getDefaultName() );

    if ( idx<0 )
	return nullptr;

    return factory().create( sqldbfact.getNames().get(idx) );
}


namespace SqlDB
{
    static Threads::Lock& getLock()
    {
	static PtrMan<Threads::Lock> lock = new Threads::Lock( false );
	return *lock.ptr();
    }
} // namespace SqlDB


PtrMan<SqlDB::QueryAccess> SqlDB::QueryProvider::mkQuery( Access& acc, int idx )
{
    Threads::Locker locker( getLock() );
    PtrMan<QueryProvider> prov = mkProv( idx );
    PtrMan<QueryAccess> query;
    if ( prov )
	query = prov->getQuery( acc );

    return query;
}


PtrMan<SqlDB::QueryAccess> mkQuery( SqlDB::Access& acc )
{
    return SqlDB::QueryProvider::mkQuery( acc );
}
