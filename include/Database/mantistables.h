#ifndef mantistables_h
#define mantistables_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          April 2010
 RCS:           $Id: mantistables.h,v 1.13 2012/01/18 11:46:00 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "typeset.h"
#include "objectset.h"

class BufferString;
class BufferStringSet;


namespace SqlDB
{

mClass BugHistoryTableEntry
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


mClass BugTextTableEntry
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


mClass BugTableEntry
{
public:

    			BugTableEntry();

    static const char*  sKeyBugTable();
    static const char*	sKeyFixedInVersion();
    static const char*	sKeySevere();
    static const char*	sKeyMinor();
    static const int    cStatusNew();
    static const int    cStatusAssigned();
    static const int    cStatusFeedback();
    static const int    cStatusResolved();
    static const int    cStatusClosed();
    static const int    cResolutionOpen();
    static const int    cResolutionFixed();
    static const int    cResolutionWillNotFixed();
    static const int    cSeverityFeature();
    static const int    cSeverityTrivial();
    static const int    cSeverityText();
    static const int    cSeverityTweak();
    static const int    cSeverityMinor();
    static const int    cSeverityMajor();
    static const int    cSeverityCrash();
    static const int    cSeverityBlock();

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
