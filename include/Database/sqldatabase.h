#ifndef sqldatabase_h
#define sqldatabase_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Feb 2010
 RCS:           $Id: sqldatabase.h,v 1.1 2010-09-10 13:26:03 cvsbert Exp $
________________________________________________________________________

-*/

#include "commondefs.h"

#ifdef __have_msql__
# define mQSqlDatabase QSqlDatabase
# define mQSqlQuery QSqlQuery
#else
# define mQSqlDatabase dummyQSqlDatabase
# define mQSqlQuery dummyQSqlQuery
#endif

class mQSqlDatabase;
class mQSqlQuery;
class BufferStringSet;
class BufferString;

mClass SqlDataBase
{
public:

    			SqlDataBase(const char* dbtype);
    virtual		~SqlDataBase();

    mQSqlDatabase*	qDataBase()	{ return qsqldatabase_; }
    void		setHostName(const char*);
    void		setDatabaseName(const char*);
    void		setUserName(const char*);
    void		setPassword(const char*);
    void		setPort(int);
    bool		open();
    bool		isOpen() const;
    void		close() const;
    bool		commit();
    BufferString	errorMsg() const;

protected:

    mQSqlDatabase*	qsqldatabase_;
};


mClass MySqlDataBase : public SqlDataBase
{
public:
    			MySqlDataBase();
};


mClass Query
{
public:

    			Query(SqlDataBase&);
    virtual		~Query();

    bool		execute(const char*);
    BufferString	data(int) const;
    bool		next() const;
    bool		isActive() const;
    BufferString	errorMsg() const;
    void		finish() const;
    BufferString        getCurrentDateTime();
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


#endif
