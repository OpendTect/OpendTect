#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"

#include "enums.h"
#include "factory.h"

/*!\brief SQL Database*/

namespace SqlDB
{

class Access;
class ConnectionData;

/*!
\brief To access a connected Database.
*/

mExpClass(General) Access
{
public:

    virtual		~Access();
			mOD_DisableCopy(Access);

    mDefineFactory1ParamInClass(Access,const char*,factory)

    ConnectionData&	connectionData()		{ return cd_; }
    const ConnectionData& connectionData() const	{ return cd_; }

    virtual bool	open()				= 0;
    virtual bool	commit()			= 0;
    virtual void	close()				= 0;

    virtual bool	isOK() const			= 0;
    virtual bool	isOpen() const			= 0;
    virtual BufferString errMsg() const			= 0;
    const char*		dbType() const		{ return dbtype_.buf(); }

protected:
			Access(const char* qtyp,const char* dbtype);

    ConnectionData&	cd_;
    BufferString	dbtype_;

};


/*!
\brief Execution of SQL Query.
*/

mExpClass(General) QueryAccess
{ mODTextTranslationClass(SqlDB::QueryAccess);
public:;
    virtual			~QueryAccess();

    virtual bool		isOK() const	    { return false; }

    virtual bool		execute(const char*)		= 0;
    virtual bool		next() const			= 0;
    virtual bool		isActive() const		= 0;
    virtual BufferString	errMsg() const			= 0;
    virtual void		finish() const			= 0;

    virtual bool		getAllRows(IOPar&) const { return false; }

    virtual int			size() const			= 0;
    virtual BufferString	data(int) const			= 0;
    virtual int			iValue(int) const		= 0;
    virtual unsigned int	uiValue(int) const		= 0;
    virtual od_int64		i64Value(int) const		= 0;
    virtual od_uint64		ui64Value(int) const		= 0;
    virtual float		fValue(int) const		= 0;
    virtual double		dValue(int) const		= 0;
    virtual bool		isTrue(int) const		= 0;

    static BufferString getInsertString(const BufferStringSet& colnms,
					const BufferStringSet& values,
					const BufferString& tablenm);
    bool		insert(const BufferStringSet& colnms,
			       const BufferStringSet& values,
			       const BufferString& tablenm);
    static int		addToColList(BufferStringSet& columns,const char*);
			//!<Returns the column index in the list
    static BufferString select(const BufferStringSet& colnms,
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
    BufferString	getCurrentDateTime();

    bool		startTransaction();
    bool		commit();
    bool		rollback();

protected:
			QueryAccess();

};


/*!
\brief Helper class that creates conditions that can be put after WHERE in a
query.
*/

mExpClass(General) Condition
{
public:
    virtual			~Condition();
    virtual BufferString	getStr() const			= 0;
protected:
				Condition();
};


/*!
\brief Condition to check for a value in a Query.
*/

mExpClass(General) ValueCondition : public Condition
{
public:
			enum class Operator { Equals, Less, Greater,
				LessOrEqual, GreaterOrEqual, NotEqual,
				Null, NotNull, Or, And };
			DeclareEnumUtils(Operator);

			ValueCondition(const char* col = nullptr,
				       Operator = Operator::Equals,
				       const char* val = nullptr);
			~ValueCondition();

    BufferString	getStr() const override;

private:

    BufferString	col_;
    BufferString	val_;
    Operator		op_;

};


/*!
\brief Condition with multiple logics in a Query.
*/

mExpClass(General) MultipleLogicCondition : public Condition
{
public:
			MultipleLogicCondition(bool isand);
			~MultipleLogicCondition();

    MultipleLogicCondition& addStatement(const char* stmnt);

    BufferString	getStr() const override;

private:

    BufferStringSet	statements_;
    bool		isand_;
};


/*!
\brief Condition to string check in a Query.
*/

mExpClass(General) StringCondition : public Condition
{
public:
			StringCondition(const char* col,
					const char* searchstr,
					bool exact);
			~StringCondition();

    BufferString	getStr() const override;

private:

    BufferString	col_;
    BufferString	searchstr_;
    bool		exact_;
};


/*!
\brief Condition to check for fulltext in a Query.
*/

mExpClass(General) FullTextCondition : public Condition
{
public:
			FullTextCondition(BufferStringSet& cols,
					  const char* searchstr);
			FullTextCondition(const char* searchstr);
			~FullTextCondition();

    FullTextCondition&	addColumn(const char*);

    BufferString	getStr() const override;

private:

    BufferString	searchstr_;
    BufferStringSet	cols_;
};


mExpClass(General) QueryProvider
{
public:

    virtual			~QueryProvider()		    {}

    mDefineFactoryInClass(QueryProvider,factory)

    static PtrMan<QueryAccess>	mkQuery(Access&,int n=-1);

private:

    virtual QueryAccess*	getQuery(Access&) const		    = 0;

    static QueryProvider*	mkProv(int);
};

mGlobal(General) PtrMan<QueryAccess> mkQuery(Access&);

} // namespace SqlDB
