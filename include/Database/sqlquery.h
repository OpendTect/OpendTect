#ifndef sqlquery_h
#define sqlquery_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Feb 2010
 RCS:           $Id: sqlquery.h,v 1.6 2012-02-28 11:08:08 cvskris Exp $
________________________________________________________________________

-*/

#include "bufstring.h"
#include "enums.h"

#ifdef __have_qsql__
# define mQSqlQuery QSqlQuery
#else
# define mQSqlQuery dummyQSqlQuery
#endif

class mQSqlQuery;
class BufferStringSet;
class IOPar;


namespace SqlDB
{
class Access;

mClass Query
{
public:

    			Query(Access&);
    virtual		~Query();

    bool		execute(const char*);
    bool		next() const;
    bool		isActive() const;
    BufferString	errMsg() const;
    void		finish() const;
    BufferString        getCurrentDateTime();

    bool		getAllRows(IOPar&) const;

    BufferString	data(int) const;
    int			iValue(int) const;
    unsigned int	uiValue(int) const;
    od_int64		i64Value(int) const;
    od_uint64		ui64Value(int) const;
    float		fValue(int) const;
    double		dValue(int) const;
    bool		isTrue(int) const;

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
    bool		starttratsaction();
    bool		commit();
    bool		rollback();
protected:

    mQSqlQuery*		qsqlquery_;

};


/*! Helper class that creates conditions that can be put after WHERE
    in a query. */
mStruct ValueCondition
{
			enum Operator { Equals, Less, Greater, LessOrEqual,
				  GreaterOrEqual, NotEqual, Null, NotNull,
				  Or, And };
			DeclareEnumUtils(Operator);
	
			ValueCondition(const char* key = 0,
				Operator op = Equals,const char* val = 0 )
			    : key_(key)
			    , op_( op )
			    , val_(val)					{}

    BufferString	key_;
    BufferString	val_;
    Operator		op_;

    BufferString	getStr() const;
};


} // namespace

#endif
