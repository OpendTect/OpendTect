#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "databasemod.h"
#include "bufstring.h"
#include "bufstringset.h"
#include "enums.h"

#ifdef OD_NO_QSQL
# define mQSqlQuery dummyQSqlQuery
#else
# define mQSqlQuery QSqlQuery
#endif

class mQSqlQuery;
class BufferStringSet;

/*!\brief SQL Database*/

namespace SqlDB
{
class Access;

/*!
\brief Execution of SQL Query.
*/

mExpClass(Database) Query
{
public:

    			Query(Access&);
    virtual		~Query();

    bool		execute(const char*);
    bool		next() const;
    bool		isActive() const;
    BufferString	errMsg() const;
    void		finish() const;
    BufferString	getCurrentDateTime();

    bool		getAllRows(IOPar&) const;

    int			size() const;
    bool		isNull(int column) const;
    BufferString	data(int column) const;
    int			iValue(int column) const;
    unsigned int	uiValue(int column) const;
    od_int64		i64Value(int column) const;
    od_uint64		ui64Value(int column) const;
    float		fValue(int column) const;
    double		dValue(int column) const;
    bool		isTrue(int column) const;

    static BufferString	getInsertString(const BufferStringSet& colnms,
					const BufferStringSet& values,
					const BufferString& tablenm);
    bool		insert(const BufferStringSet& colnms,
			       const BufferStringSet& values,
			       const BufferString& tablenm);
    static int		addToColList(BufferStringSet& columns,const char*);
			//!<Returns the column index in the list
    static BufferString	select(const BufferStringSet& colnms,
				const BufferString& tablenm,
				const char* condstr);
    BufferString	getUpdateString(const BufferStringSet& colnms,
					const BufferStringSet& values,
					const BufferString& tablenm,
					int bugid) const;
    bool		update(const BufferStringSet& colnms,
			       const BufferStringSet& values,
			       const BufferString& tablenm,int bugid);
    bool		deleteInfo(const char* tablenm,const char* fldnm,
				   int id);

    bool		startTransaction();
    bool		commit();
    bool		rollback();

protected:

    mQSqlQuery*		qsqlquery_;

};


/*!
\brief Helper class that creates conditions that can be put after WHERE in a
query.
*/

mExpClass(Database) Condition
{
public:
    virtual			~Condition() {}
    virtual BufferString	getStr() const			= 0;
};


/*!
\brief Condition to check for a value in a Query.
*/

mExpClass(Database) ValueCondition : public Condition
{
public:
			enum Operator { Equals, Less, Greater, LessOrEqual,
				  GreaterOrEqual, NotEqual, Null, NotNull,
				  Or, And };
			DeclareEnumUtils(Operator);
	
			ValueCondition(const char* col = 0,
				Operator op = Equals,const char* val = 0 );

    BufferString	getStr() const;

protected:
    BufferString	col_;
    BufferString	val_;
    Operator		op_;

};


/*!
\brief Condition with multiple logics in a Query.
*/

mExpClass(Database) MultipleLogicCondition : public Condition
{
public:
    			MultipleLogicCondition(bool isand)
			    : isand_( isand )			{}

    void		addStatement( const char* stmnt )
			{ statements_.add( stmnt ); }

    BufferString	getStr() const;
protected:
    BufferStringSet	statements_;
    bool		isand_;
};


/*!
\brief Condition to string check in a Query.
*/

mExpClass(Database) StringCondition : public Condition
{
public:
    			StringCondition( const char* col,
					 const char* searchstr,
					 bool exact );
    BufferString	getStr() const;
protected:

    BufferString	col_;
    BufferString	searchstr_;
    bool		exact_;
};


/*!
\brief Condition to check for fulltext in a Query.
*/

mExpClass(Database) FullTextCondition : public Condition
{
public:
			FullTextCondition( BufferStringSet& cols,
					   const char* searchstr );
			FullTextCondition( const char* searchstr );
    BufferString	getStr() const;
    FullTextCondition&	addColumn(const char*);

protected:
    BufferString	searchstr_;
    BufferStringSet	cols_;
};

} // namespace SqlDB
