#ifndef sqlquery_h
#define sqlquery_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Feb 2010
 RCS:           $Id: sqlquery.h,v 1.4 2011-12-23 15:26:46 cvskris Exp $
________________________________________________________________________

-*/

#include "bufstring.h"

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
				int id, const char* idkey=0);

    int			getLastAutoID();

    BufferString	getUpdateString(const BufferStringSet& colnms,
	    				const BufferStringSet& values,
					const BufferString& tablenm,
					int bugid) const;
    bool		update(const BufferStringSet& colnms,
	    		       const BufferStringSet& values,
			       const BufferString& tablenm,int bugid);
    bool		deleteInfo(const char* tablenm,const char* fldnm,
	    			   int id);
protected:

    mQSqlQuery*		qsqlquery_;

};

} // namespace

#endif
