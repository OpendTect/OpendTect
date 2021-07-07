#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 2016
________________________________________________________________________

-*/


#include "generalmod.h"
#include "dbkey.h"
#include "monitor.h"
#include "ptrman.h"
#include "uistring.h"
#include "ioobjctxt.h"
class CommandLineParser;
class DBDir;
class DBMManager;
class IOObjSelConstraints;


/*!\brief manages the 'Meta-'data store for the IOObj's.
This info is read from the .omf files. The 'workhorse is the DBDir object.

There will be one DBMan available through the global function DBM(). Creating
more instances is probably not a good idea.
*/

extern "C" {

mGlobal(General) const char* setDBMDataSource(const char* fullpath,
					    bool refresh=false);
    /*!< Sets the current survey of the database manager IOM()
	  Returns an error message if it fails */
}

mExpClass(General) DBMan : public Monitorable
{ mODTextTranslationClass(DBMan);
public:

    typedef DBKey::DirID DirID;
    typedef DBKey::ObjID ObjID;

    bool		isPresent(const DBKey&) const;
    bool		isPresent(const char*,const char* tgname) const;
    mDeprecated BufferString nameOf( const DBKey& dbky ) const
			{ return dbky.name(); }
    BufferString	nameFor(const char* keystr) const;
			//!< if keystr is not an IOObj key, will return keystr
    BufferString	mainFileOf(const DBKey&) const;

    IOObj*		get(const DBKey&) const;
    IOObj*		getFirst(const IOObjContext&,int* nrpresent=0) const;
    IOObj*		getFromPar(const IOPar&,const char* basekey,
					const IOObjContext&,bool mknew,
					uiString& errmsg) const;
    IOObj*		getByName(IOObjContext::StdSelType,const char* objname,
				    const char* tgname=0) const;
    IOObj*		getByName(const IOObjContext&,const char* objnm) const;
    IOObj*		getByName(const char* objname,const char* tgname) const;

    void		getEntry(CtxtIOObj&,bool ifnewmakeittemp=false,
				 int translidxingroup=-1);
				//!< will create a new entry if necessary
    bool		setEntry(const IOObj&);
    bool		removeEntry(const DBKey&);

    ConstRefMan<DBDir>	fetchDir(DirID) const;
    ConstRefMan<DBDir>	fetchDir(IOObjContext::StdSelType) const;
    ConstRefMan<DBDir>	fetchDir(const IOObjContext&) const;
    ConstRefMan<DBDir>	fetchRootDir() const	{ return rootdbdir_; }
    ConstRefMan<DBDir>	findDir(const char* trgroupname) const;

    BufferString	survDir() const		{ return survdir_; }
    BufferString	surveyName() const;
    BufferString	surveyDirectoryName() const;
    bool		isKeyString(const char*) const;

    Notifier<DBMan>	surveyToBeChanged;  //!< Before the change
    Notifier<DBMan>	surveyChanged;      //!< To restore OD to normal state
    Notifier<DBMan>	afterSurveyChange;  //!< When operating in normal state
    Notifier<DBMan>	applicationClosing; //!< 'Final' call ...

    CNotifier<DBMan,DBKey>	entryAdded;
    CNotifier<DBMan,DBKey>	entryRemoved;
    CNotifier<DBMan,DBKey>	entryToBeRemoved;

    mExpClass(General) CustomDirData
    {
    public:

	typedef DBKey::DirNrType  DirNrType;

			CustomDirData( DirNrType dnr, const char* dirnm,
					const char* desc )
			    : dirnr_(dnr)
			    , dirname_(dirnm)
			    , desc_(desc)		{}

	DirNrType	dirnr_; //!< Make sure your dirnr_ > 200000
				 //!< Lower than that will be refused!
				 //!< Example: 218745
	BufferString	dirname_; //!< The subdirectory name in the tree
	BufferString	desc_; //!< Short description, mainly for error messages
			       //!< Example: "Geostatistical data"

	bool		operator ==( const CustomDirData& cdd ) const
			{ return dirnr_ == cdd.dirnr_; }
    };

    static uiRetVal	addCustomDataDir(const CustomDirData&);
			//!< Need to do this only once per OD run
			//!< At survey change, dir will automatically be added

private:

    BufferString	survdir_;
    DBDir*		rootdbdir_;
    ObjectSet<DBDir>	dbdirs_;
    mutable uiString	errmsg_;
    bool		surveychangeuserabort_;
    uiRetVal		surveychangeabortreason_;

			DBMan();
			~DBMan();
    DBMan*		getClone() const	    { return 0; }

    uiRetVal		handleNewSurvDir();
    void		leaveSurvey();
    void		readDirs();
    RefMan<DBDir>	getDBDir(DirID);
    DBDir*		gtDir(DirID) const;
    uiRetVal		setupCustomDataDirs(int);
    void		setupCustomDataDir(const CustomDirData&,uiRetVal&);
    IOObj*		crWriteIOObj(const CtxtIOObj&,DBKey,int trlidx) const;
    void		dbdirChgCB(CallBacker*);

    friend mGlobal(General) DBMan& DBM();
    friend class	GeneralModuleIniter;

    uiRetVal		setDataSource(const char*,const char*,bool);

    void		initFirst();
    uiRetVal		doReRead();

    void		applClosing() {	applicationClosing.trigger(); }

    friend class	DBMManager;
    friend class	BatchProgram;
    friend class	ServiceMgrBase;
    friend class	uiMain;
    friend class	uiMainWin;

public:

    bool		isBad() const;
    uiString		errMsg() const		{ return errmsg_; }
    static uiRetVal	isValidDataRoot(const char* dirnm);
    static uiRetVal	isValidSurveyDir(const char* dirnm);
    static uiRetVal	checkSurveySetupValid();
    BufferString	getDirectoryNameOf(DirID,bool fullpath) const;
    void		findTempObjs(ObjectSet<IOObj>&,
				const IOObjSelConstraints* cnstrts=0) const;
			//!< set filled with cloned ioobjs. Needs deepErase().
    Notifier<DBMan>	surveyChangeOK;
    void		setSurveyChangeUserAbort();
    void		setSurveyChangeAbortReason(uiRetVal);

    mDeprecated bool	permRemove( const DBKey& ky )
			{ return removeEntry(ky); }
    mDeprecated bool	commitChanges( const IOObj& ioobj )
			{ return setEntry(ioobj); }
    mDeprecated BufferString rootDir() const
			{ return survDir(); }

			// To change to another survey, probably not your thing:
    uiRetVal		setDataSource(const char* fullpath_of_survey_dir,
					bool forcerefresh=false);
			//!< Plugins need to use uiDataRootSel::setSurveyDirTo
    uiRetVal		setDataSource(const IOPar&,bool forcerefresh=false);
			//!< uses sKey::dataRoot() and sKey::Survey()
			//!< Intended for stand-alone programs
    uiRetVal		setDataSource(const CommandLineParser&,bool* ischgd=0);
			//!< Intended for stand-alone programs

    uiRetVal		reRead();
			//!< Should not be necessary

};

mGlobal(General) DBMan&	DBM();
