/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Feb 2010
 RCS:           $Id$
________________________________________________________________________

-*/

#include "sqlquery.h"
#include "keystrs.h"
#include "sqldatabase.h"
#include "bufstringset.h"
#include <QString>


const char* paranstart = " ( ";
const char* paranend = " ) ";

DefineEnumNames(SqlDB::ValueCondition,Operator,0,"Operators" )
{ "=", "<", ">", "<=", ">=", "!=",
  "IS NULL", "IS NOT NULL", "OR", "AND", 0 };



#ifdef __have_qsql__

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#else

class mQSqlQuery
{
public:
    struct ValStruct
    {
	QString toString() const
	{ BufferString ret("S "); ret += idx(); return QString(ret.buf()); }
#	define mDeclQueryNumbFn(typ,fn) \
	typ fn() const			{ return (typ)idx(); }
	mDeclQueryNumbFn(int,toInt)
	mDeclQueryNumbFn(unsigned int,toUInt)
	mDeclQueryNumbFn(od_int64,toLongLong)
	mDeclQueryNumbFn(od_uint64,toULongLong)
	mDeclQueryNumbFn(double,toDouble)
	mDeclQueryNumbFn(bool,toBool)
    };
    struct ErrStruct
    {
	QString text() const
	{ BufferString ret("E "); ret += idx(); return QString(ret.buf()); }
    };

    		mQSqlQuery(const mQSqlDatabase&)	{}
    void	setHostName(const char*)		{}
    bool	exec(const char*)			{ return false; }
    bool	next() const				{ return false; }
    ValStruct	value(int) const			{ return ValStruct(); }
    ErrStruct	lastError()				{ return ErrStruct(); }
    bool	isActive() const			{ return false; }
    void	finish() const				{}

    static od_int64	retidx_;
    static od_int64	idx()				{ return retidx_++; }

};

od_int64 mQSqlQuery::retidx_ = 1;

#endif


SqlDB::Query::Query( SqlDB::Access& acc )
{
    qsqlquery_ = new mQSqlQuery( *(acc.qDataBase()) );
}


SqlDB::Query::~Query()
{
    delete qsqlquery_;
}


bool SqlDB::Query::execute( const char* querystr )
{
    return qsqlquery_->exec( querystr );
}


bool SqlDB::Query::next() const
{
    return qsqlquery_->next();
}


BufferString SqlDB::Query::data( int colid ) const
{
    return BufferString( qsqlquery_->value(colid).toString().toAscii().data() );
}


#define mDefQueryNumbFn(typ,valfn,qfn) \
typ SqlDB::Query::valfn( int columnid ) const \
{ \
    return qsqlquery_->value(columnid).qfn(); \
}

mDefQueryNumbFn(int,iValue,toInt)
mDefQueryNumbFn(unsigned int,uiValue,toUInt)
mDefQueryNumbFn(od_int64,i64Value,toLongLong)
mDefQueryNumbFn(od_uint64,ui64Value,toULongLong)
mDefQueryNumbFn(float,fValue,toDouble)
mDefQueryNumbFn(double,dValue,toDouble)
mDefQueryNumbFn(bool,isTrue,toBool)


BufferString SqlDB::Query::errMsg() const
{
    return BufferString( qsqlquery_->lastError().text().toAscii().data() );
}


bool SqlDB::Query::isActive() const
{
    return qsqlquery_->isActive();
}


void SqlDB::Query::finish() const
{
    if ( qsqlquery_->isActive() )
	qsqlquery_->finish();
}


BufferString SqlDB::Query::getCurrentDateTime()
{
    execute( "SELECT CURRENT_TIMESTAMP()" );
    BufferString datetime;
    while ( next() )
	datetime = data(0);

    return datetime;
}


bool SqlDB::Query::starttratsaction()
{
    return execute( "START TRANSACTION" );
}


bool SqlDB::Query::commit()
{
    return execute( "COMMIT" );
}


bool SqlDB::Query::rollback()
{
    return execute( "ROLLBACK" );
}


bool SqlDB::Query::insert( const BufferStringSet& colnms,
			   const BufferStringSet& values,
			   const BufferString& tablenm )
{
    BufferString qstr = getInsertString( colnms, values, tablenm );
    return execute( qstr );
}


BufferString SqlDB::Query::getInsertString( const BufferStringSet& colnms,
					    const BufferStringSet& values,
					    const BufferString& tablenm )
{
    BufferString querystr;
    if ( colnms.size() != values.size() )
	return querystr;

    const int nrvals = values.size();
    querystr = "INSERT INTO "; querystr.add( tablenm ).add( paranstart );
    for ( int idx=0; idx<nrvals; idx++ )
    {
	querystr.add( colnms[idx]->buf() );
	if ( idx != nrvals-1 )
	    querystr.add( "," );
    }

    querystr.add( paranend );

    querystr.add( " VALUES (" );

    for ( int idx=0; idx<nrvals; idx++ )
    {
	BufferString str( values.get( idx ) );
	if ( str.isEmpty() )
	    querystr.add( "''" );
	else
	{
	    str.setBufSize( 2*str.size()+2 );
	    replaceString( str.buf(), "\\", "\\\\" );
	    replaceString( str.buf(), "'", "\\'" );
	    querystr.add( "'" ).add ( str ).add( "'" );
	}

	if ( idx != nrvals-1 )
	    querystr.add( "," );
    }

    querystr.add( paranend );

    return querystr;
}


int SqlDB::Query::addToColList(BufferStringSet& columns,const char* newcol)
{
    const int res = columns.size();
    columns.add( newcol );
    return res;
}


BufferString SqlDB::Query::getUpdateString( const BufferStringSet& colnms,
					    const BufferStringSet& values,
					    const BufferString& tablenm,
					    int bugid ) const
{
    BufferString querystr;
    if ( bugid<0 || colnms.size()!=values.size() )
	return querystr;
//TODO
//Better to get bug_text_id from mantis_bug_table by using bugid and update
//mantis_bug_text_table by using bug_text_id which is id in this table    
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


BufferString SqlDB::Query::select( const BufferStringSet& colnms,
				   const BufferString& tablenm,
				   const char* condstr )
{
    BufferString querystr;

    const int nrvals = colnms.size();
    querystr.add( "SELECT " );
    for ( int idx=0; idx<nrvals; idx++ )
    {
	querystr.add( colnms[idx]->buf() );
	querystr.add( idx != nrvals-1 ? "," : sKey::SpaceString );
    }

    querystr.add( "FROM " ).add( tablenm );
    if ( condstr )
    {
	querystr.add( " WHERE " ).add( condstr );
    }

    return querystr;
}


bool SqlDB::Query::update( const BufferStringSet& colnms,
			   const BufferStringSet& values,
			   const BufferString& tablenm, int bugid )
{
    BufferString qstr = getUpdateString( colnms, values, tablenm, bugid );
    return execute( qstr );
}


bool SqlDB::Query::deleteInfo( const char* tablenm, const char* fieldnm,
			       int id )
{
    BufferString qstr( "DELETE FROM " );
    qstr.add( tablenm ).add( " WHERE " ).add( fieldnm ).add( "=" ).add( id );
    return execute( qstr );
}


SqlDB::ValueCondition::ValueCondition(const char* key,
				      SqlDB::ValueCondition::Operator op,
				      const char* val )
    : col_(key)
    , op_( op )
    , val_(val)
{}


BufferString SqlDB::ValueCondition::getStr() const
{
    BufferString res( col_ );
    res.add( sKey::SpaceString).add( toString( op_ ) )
        .add( sKey::SpaceString ).add( val_ );
    return res;
}


BufferString SqlDB::MultipleLogicCondition::getStr() const
{
    const char* op = ValueCondition::toString( isand_
	    ? ValueCondition::And
	    : ValueCondition::Or );

    BufferString res( paranstart );
    for ( int idx=0; idx<statements_.size(); idx++ )
    {
	res.add( paranstart ).add( statements_[idx]->buf() ).add( paranend );
	if ( idx!=statements_.size()-1 )
	{
	    res.add( op );
	}
    }

    res.add( paranend );

    return res;
}


SqlDB::StringCondition::StringCondition( const char* col,
					 const char* searchstr,
					 bool exact )
    : col_( col )
    , searchstr_( searchstr )
    , exact_( exact )
{}


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


SqlDB::FullTextCondition::FullTextCondition( BufferStringSet& cols,
					     const char* searchstr )
    : cols_( cols )
    , searchstr_( searchstr )
{}


SqlDB::FullTextCondition::FullTextCondition( const char* searchstr )
    : searchstr_( searchstr )
{}


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
