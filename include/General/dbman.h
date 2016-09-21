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
class DBDir;
class IOObj;
class CtxtIOObj;
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

    bool		isPresent(const DBKey&) const;
    bool		isPresent(const char*,const char* tgname) const;
    IOObj*		get(DBKey) const;
    IOObj*		getByName(const char* objname,const char* tgname) const;
    BufferString	nameOf(const DBKey&) const;
    BufferString	nameFor(const char* keystr) const;
			//!< if keystr is not an IOObj key, will return keystr

    void		getEntry(CtxtIOObj&,bool newistmp=false,
				 int translidxingroup=-1);
				//!< will create a new entry if necessary
    bool		setEntry(const IOObj&);
    bool		removeEntry(const DBKey&);

    ConstRefMan<DBDir>	fetchRoot() const	{ return rootdbdir_; }
    ConstRefMan<DBDir>	fetch(DirID);

    BufferString	rootDir() const		{ return rootdir_; }
    bool		setRootDir(const char*);
    BufferString	surveyName() const;
    bool		isKeyString(const char*) const;

    Notifier<DBMan>	surveyToBeChanged;  //!< Before the change
    Notifier<DBMan>	surveyChanged;      //!< To restore OD to normal state
    Notifier<DBMan>	afterSurveyChange;  //!< When operating in normal state
    Notifier<DBMan>	applicationClosing; //!< 'Final' call ...

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
    static DirID	addCustomDataDir(const CustomDirData&,
					   uiString& errmsg);
			//!< Need to do this only once per OD run
			//!< At survey change, dir will automatically be added

private:

    BufferString	rootdir_;
    DBDir*		rootdbdir_;
    ObjectSet<DBDir>	dirs_;
    mutable uiString	errmsg_;

			DBMan();
			~DBMan();
    DBMan*		clone() const	    { return 0; }

    void		handleNewRootDir();
    void		leaveSurvey();
    void		readDirs();
    static void		setupCustomDataDirs(int,uiString& errmsg);
    BufferString	surveyDirectory() const;
    DBDir*		gtDir(DirID);
    const DBDir*	gtDir(DirID) const;

    friend mGlobal(General) DBMan& DBM();

    void		findTempObjs(ObjectSet<IOObj>&,
				const IOObjSelConstraints* cnstrts=0) const;
			//!< set filled with cloned ioobjs. Needs deepErase().

public:

    DBKey		createNewKey(DirID);
    bool		isBad() const;
    uiString		errMsg() const		{ return errmsg_; }
    static bool		isValidDataRoot(const char* dirnm);
    static bool		isValidSurveyDir(const char* dirnm);

};


mGlobal(General) DBMan&	DBM();
// inline mGlobal(General) DBMan&	IOM()	{ return DBM(); }
