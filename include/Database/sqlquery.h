#ifndef sqlquery_h
#define sqlquery_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Feb 2010
 RCS:           $Id: sqlquery.h,v 1.1 2010-09-14 10:32:32 cvsbert Exp $
________________________________________________________________________

-*/

#include "bufstring.h"

#ifdef __have_msql__
# define mQSqlQuery QSqlQuery
#else
# define mQSqlQuery dummyQSqlQuery
#endif

class mQSqlQuery;
class BufferStringSet;


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

    BufferString	data(int) const;
    int			iValue(int) const;
    unsigned int	uiValue(int) const;
    od_int64		i64Value(int) const;
    od_uint64		ui64Value(int) const;
    float		fValue(int) const;
    double		dValue(int) const;
    bool		isTrue(int) const;

    BufferString	getInsertString(const BufferStringSet& colnms,
	    				const BufferStringSet& values,
					const BufferString& tablenm) const;
    bool		insert(const BufferStringSet& colnms,
	    		       const BufferStringSet& values,
	    		       const BufferString& tablenm);
    BufferString	select(const BufferStringSet& colnms,
				const BufferString& tablenm,
				int id);
    BufferString	getUpdateString(const BufferStringSet& colnms,
	    				const BufferStringSet& values,
					const BufferString& tablenm,
					int bugid) const;
    bool		update(const BufferStringSet& colnms,
	    		       const BufferStringSet& values,
			       const BufferString& tablenm,int bugid);
protected:

    mQSqlQuery*		qsqlquery_;

};

} // namespace

#endif
