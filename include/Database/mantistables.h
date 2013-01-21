#ifndef mantistables_h
#define mantistables_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          April 2010
 RCS:           $Id$
________________________________________________________________________

-*/

#include "databasemod.h"
#include "typeset.h"
#include "objectset.h"

class BufferString;
class BufferStringSet;


namespace SqlDB
{

/*!
\ingroup Database
\brief Bug history table entry in SQL Database.
*/

mExpClass(Database) BugHistoryTableEntry
{
public:    

    			BugHistoryTableEntry();

    static const char*	sKeyBugHistoryTable();
    void		init();
    void		getQueryInfo(BufferStringSet& colnms,
	    			     BufferStringSet& values);

    int			userid_;
    int			bugid_;
    BufferString	date_;
    BufferString	fieldnm_;
    BufferString	oldvalue_;
    BufferString	newvalue_;
    int			type_;

};


/*!
\ingroup Database
\brief Bug text table entry in SQL Database.
*/

mExpClass(Database) BugTextTableEntry
{
public:

    			BugTextTableEntry();

    static const char*  sKeyBugTextTable();

    void		init();
    void		setDescription(BufferString& desc);
    void		setStepsReproduce(BufferString& reproduce);
    void		setReporter(BufferString& reporter);
    void		getQueryInfo(BufferStringSet& colnms,
	    			     BufferStringSet& values);
    void		addToHistory(const char* fieldnm);
    void		deleteHistory();
    ObjectSet<BugHistoryTableEntry>&	getHistory()
    					{ return btthistoryset_; }

    BufferString	description_;
    BufferString	stepsreproduce_;
    BufferString	reporter_;

protected:

    ObjectSet<BugHistoryTableEntry> btthistoryset_;
};


/*!
\ingroup Database
\brief 
*/

mExpClass(Database) BugTableEntry
{
public:

    			BugTableEntry();

    static const char*  sKeyBugTable();
    static const char*	sKeyFixedInVersion();
    static const char*	sKeySevere();
    static const char*	sKeyMinor();
    static int		cStatusNew();
    static int		cStatusAssigned();
    static int		cStatusFeedback();
    static int 		cStatusResolved();
    static int		cStatusClosed();
    static int		cResolutionOpen();
    static int		cResolutionFixed();
    static int		cResolutionWillNotFixed();
    static int		cSeverityFeature();
    static int		cSeverityTrivial();
    static int		cSeverityText();
    static int		cSeverityTweak();
    static int		cSeverityMinor();
    static int		cSeverityMajor();
    static int		cSeverityCrash();
    static int		cSeverityBlock();

    void		getQueryInfo(BufferStringSet& colnms,
	    			     BufferStringSet& values,bool isedit);
    void		init();
    void		setSeverity(int);
    void		setHandlerID(int);
    void		setStatus(int);
    void		setPlatform(const char* plf);
    void		setVersion(const char* version);
    void		setSummary(BufferString&);
    void		setResolution(int resolution);
    void		addToHistory(const char* fldnm,const char* oldval,
	    			     const char* newval);
    void		deleteHistory();
    ObjectSet<BugHistoryTableEntry>&	getHistory()	{ return historyset_; }
    static bool		isSevere(int);

    int			id_;
    int			projectid_;
    int			reporterid_;
    int			handlerid_;
    int			severity_;
    int			status_;
    int			resolution_;
    BufferString	category_;
    BufferString	lastupddate_;
    BufferString	platform_;
    BufferString	version_;
    BufferString	fixedinversion_;
    BufferString	summary_;

protected:

    ObjectSet<BugHistoryTableEntry> historyset_;
};

} // namespace

#endif

