#ifndef mantisdatabase_h
#define mantisdatabase_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Feb 2010
 RCS:           $Id: mantisdatabase.h,v 1.1 2010-09-14 10:32:32 cvsbert Exp $
________________________________________________________________________

-*/

#include "sqldatabase.h"
#include "sqlquery.h"
#include "bufstringset.h"
#include "typeset.h"


namespace SqlDB
{
class BugTableEntry;
class BugTextTableEntry;
class BugHistoryTableEntry;


mClass MantisAccess : public MySqlAccess
{
public:

    			MantisAccess()
			    : MySqlAccess("Mantis")	{}

};


mClass MantisQuery : public Query
{
public:
			MantisQuery(MantisAccess&);

    bool		updateTables(BugTableEntry&,BugTextTableEntry&);

};


mClass MantisDBMgr
{
public:

    				MantisDBMgr(const ConnectionData* cd=0);
    				~MantisDBMgr();

    inline MantisAccess&	access() 	{ return acc_; }
    inline const MantisAccess&	access() const 	{ return acc_; }
    inline MantisQuery&		query()		{ return *query_; }
    inline const MantisQuery&	query() const	{ return *query_; }
    bool			isOK() const	{ return acc_.isOpen(); }
    const char*			errMsg() const;

    int				getUserID(bool isdeveloper) const;
    int				getMaxBugIDFromBugTable() const;
    int				getMaxNoteIDFromBugNoteTable() const;
    BugTableEntry*		getBugTableEntry(int tableidx);
    BugTextTableEntry*		getBugTextTableEntry(int tableidx);
    inline int			nrBugs() const		{ return bugs_.size(); }
    const TypeSet<int>&		userIDs() const		{ return userids_; }
    const BufferStringSet&	developers() const	{ return developers_; }
    const BufferStringSet&	userNames() const	{ return usernames_; }
    const BufferStringSet&	versions() const	{ return versions_; }
    const BufferStringSet&	categories() const	{ return categories_; }
    void			getSummaries(BufferStringSet& summaries,
	    				     bool assigned);

    bool			addBug(BugTableEntry&,BugTextTableEntry&,
	    			       const char* note);
    bool			editBug(BugTableEntry&,BugTextTableEntry&,
	    				const char* note);
    void			addBugTableEntryToSet(BugTableEntry&);
    void			addBugTextTableEntryToSet(BugTextTableEntry&);
    void			removeBugTableEntryFromSet(int tableidx);
    void			removeBugTextTableEntryFromSet(int tableidx);

    void			eraseCurrentEntries(bool isfix);
    void			updateBugTableEntryHistory(int idx,
	    						   bool isadded,
							   bool isnoteempty);
    void			updateBugTextTableEntryHistory(int idx);
    int				getBugTableID(int bugid);
    bool			getInfoFromTables();
    TypeSet<int>&		getBugsMySelf();

    static void			prepareForQuery(BufferString&);

    static const char*	sKeyBugNoteTable();
    static const char*	sKeyBugNoteTextTable();
    static const char*	sKeyProjectCategoryTable();
    static const char*	sKeyUserTable();
    static const char*	sKeyProjectUserListTable();
    static const char*	sKeyProjectVersionTable();
    static const int 	cOpenDtectProjectID();
    static const int 	cAccessLevelDeveloper();
    static const int 	cAccessLevelCaseStudy();

protected:

    bool		addToBugTable(BugTableEntry&);
    bool		addToBugTextTable(BugTextTableEntry&);
    bool		addToBugNoteTable(const char*,int);
    bool		addToBugNoteTextTable(const char*);
    bool		fillCategories();
    bool		fillBugTableEntries();
    bool		fillBugsInfoSet();
    bool		fillUserNamesIDs();
    bool		fillVersions();
    void		addHistoryToSet(BugHistoryTableEntry&);

    bool		updateBugHistoryTable(ObjectSet<BugHistoryTableEntry>&,
	    				      bool isadded);

    MantisAccess	acc_;
    mutable MantisQuery* query_;

    TypeSet<int>	userids_;
    BufferStringSet	usernames_;
    BufferStringSet	developers_;
    BufferStringSet	versions_;
    BufferStringSet	categories_;
    BugTableEntry*	bugtable_;
    BugTextTableEntry*	bugtexttable_;
    ObjectSet<BugTableEntry>	bugs_;
    ObjectSet<BugTextTableEntry> texttables_;
    TypeSet<int>	bugidsetmyself_;

    mutable BufferString errmsg_;

};


} // namespace

#endif
