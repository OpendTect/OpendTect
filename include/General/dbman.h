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
class DBDir;
class IOObjSelConstraints;


/*!\brief manages the 'Meta-'data store for the IOObj's.
This info is read from the .omf files. The 'workhorse is the DBDir object.

There will be one DBMan available through the global function DBM(). Creating
more instances is probably not a good idea.
*/


mExpClass(General) DBMan : public Monitorable
{ mODTextTranslationClass(DBMan);
public:

    typedef DBKey::DirID DirID;
    typedef DBKey::ObjID ObjID;

    bool		isPresent(DBKey) const;
    bool		isPresent(const char*,const char* tgname) const;
    BufferString	nameOf(DBKey) const;
    BufferString	nameFor(const char* keystr) const;
			//!< if keystr is not an IOObj key, will return keystr

    IOObj*		get(DBKey) const;
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
    bool		removeEntry(DBKey);

    ConstRefMan<DBDir>	fetchDir(DirID) const;
    ConstRefMan<DBDir>	fetchDir(IOObjContext::StdSelType) const;
    ConstRefMan<DBDir>	fetchDir(const IOObjContext&) const;
    ConstRefMan<DBDir>	fetchRootDir() const	{ return rootdbdir_; }

    BufferString	rootDir() const		{ return rootdir_; }
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

    BufferString	rootdir_;
    DBDir*		rootdbdir_;
    ObjectSet<DBDir>	dirs_;
    mutable uiString	errmsg_;
    bool		surveychangeuserabort_;

			DBMan();
			~DBMan();
    DBMan*		clone() const	    { return 0; }

    uiRetVal		handleNewRootDir();
    void		leaveSurvey();
    void		readDirs();
    RefMan<DBDir>	getDBDir(DirID);
    DBDir*		gtDir(DirID) const;
    uiRetVal		setupCustomDataDirs(int);
    void		setupCustomDataDir(const CustomDirData&,uiRetVal&);
    IOObj*		crWriteIOObj(const CtxtIOObj&,DBKey,int trlidx) const;
    void		dbdirChgCB(CallBacker*);

    friend mGlobal(General) DBMan& DBM();
    friend class	uiSurvey;

    uiRetVal		setDataSource(const char*,const char*);

public:

    bool		isBad() const;
    uiString		errMsg() const		{ return errmsg_; }
    static uiRetVal	isValidDataRoot(const char* dirnm);
    static uiRetVal	isValidSurveyDir(const char* dirnm);
    static uiRetVal	checkSurveySetupValid();
    void		findTempObjs(ObjectSet<IOObj>&,
				const IOObjSelConstraints* cnstrts=0) const;
			//!< set filled with cloned ioobjs. Needs deepErase().
    Notifier<DBMan>	surveyChangeOK;
    void		doNotChangeTheSurveyNow();
    void		applClosing()		{ applicationClosing.trigger();}

    mDeprecated bool	permRemove( const DBKey& ky )
			{ return removeEntry(ky); }
    mDeprecated bool	commitChanges( const IOObj& ioobj )
			{ return setEntry(ioobj); }

    void		initFirst();
			//!< Not for you. Don't use.

			// Be careful. Stuff won't hold up in MT situations
    uiRetVal		setDataSource(const IOPar&);
			//!< To change to the required survey
    static DBMan*	getEmpty()		{ return new DBMan; }
    static void		retire( DBMan* dbm )	{ delete dbm; }
    static void		pushDBM(DBMan*);
    static bool		popDBM();
};


mGlobal(General) DBMan&	DBM();
