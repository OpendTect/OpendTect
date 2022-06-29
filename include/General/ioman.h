#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		3-8-1995
________________________________________________________________________

-*/


#include "generalmod.h"
#include "ctxtioobj.h"
#include "multiid.h"
#include "namedobj.h"
#include "surveydisklocation.h"

class CommandLineParser;
class CtxtIOObj;
class IODir;
class IOMManager;
class IOObj;
class IOObjContext;
class IOSubDir;
class SurveyInfo;
class SurveyDataTreePreparer;
class SurveyDiskLocation;

/*!
\brief manages the 'Meta-'data store for the IOObj's.
This info is read from the .omf files.

There will be one IOMan available through the global function IOM(). Creating
more instances is probably not a good idea.
*/

extern "C" {

mGlobal(General) const char* setDBMDataSource(const char* fullpath,
					      bool refresh=false);
    /*!< Sets the current survey of the database manager IOM()
	  Returns an error message if it fails */
}

mExpClass(General) IOMan final : public NamedCallBacker
{ mODTextTranslationClass(IOMan);
public:

    bool		isBad() const		{ return state_ != Good; }
    bool		isReady() const;
    const OD::String&	message() const		{ return msg_.getFullString(); }
    const uiString&	uiMessage() const	{ return msg_; }

    bool		isUsable(const DBKey&) const;
    bool		isUsable(const MultiID&) const;
    bool		implIsLink(const MultiID&) const;
    bool		implExists(const MultiID&) const;
    bool		isReadOnly(const MultiID&) const;
    bool		implRename(const MultiID&,const char* newnm);
    bool		implReloc(const MultiID&,const char* newdir);
    bool		implRemove(const MultiID&,
				   bool rmentry=false,uiRetVal* uirv=nullptr);
    bool		implRemove(const IOObj&,bool deep=true) const;

    void		removeUnusable(DBKeySet&);
			//! Next functions return a new (unmanaged) IOObj
    IOObj*		get(const DBKey&) const;
    IOObj*		get(const MultiID&) const;
    IOObj*		getLocal(const char* objname,const char* tgname) const;
    IOObj*		getOfGroup(const char* tgname,bool first=true,
				   bool onlyifsingle=false) const;
    IOObj*		getIfOnlyOne( const char* trgroupname )
			{ return getOfGroup(trgroupname,true,true); }
    IOObj*		getFirst(const IOObjContext&,int* nrpresent=0) const;
			//!< if interested in nrpresent pass valid address
    IOObj*		getFromPar(const IOPar&,const char* basekey,
				   const IOObjContext&,bool mknew,
				   BufferString& errmsg) const;
    IOObj*		get(const char* objname,const char* tgname) const;
    IOObj*		get(const IOObjContext&,const char* objnm) const;

    bool		isPresent(const MultiID&) const;
    bool		isPresent(const char*,const char* tgname=0) const;
			/*!< Use before creating a named object
			    \param tgname: example:
			     EMHorizon3DTranslatorGroup::sGroupName().str() */

    const MultiID&	key() const;		//!< of current IODir
    const char*		curDirName() const;	//!< OS dir name
    const char*		rootDir() const		{ return rootdir_; }
    bool		isKey(const char* keystr) const;
    const char*		nameOf(const char* keystr) const;
			//!< if keystr is not an IOObj key, will return keystr
    const char*		nameOf(const MultiID&) const;
    const char*		objectName(const DBKey&) const;

    MultiID		createNewKey(const MultiID& dirkey);

    bool		to(IOObjContext::StdSelType,bool force_reread=false);
    bool		to(const MultiID&,bool force_reread=false);
    bool		toRoot(bool force_reread=false)
			{ return to(0,force_reread); }

    void		getEntry(CtxtIOObj&,bool newistmp=false,
				 int translidxingroup=-1);
				//!< will create a new entry if necessary
    void		getNewEntry(CtxtIOObj&,bool newistmp=false,
				 int translidxingroup=-1);
				//!< will create a new entry if necessary
    bool		commitChanges(const IOObj&);
    bool		permRemove(const MultiID&);
				//!< Removes only entry in IODir

    const char*		surveyName() const;

    mExpClass(General) CustomDirData
    {
    public:
			CustomDirData( int dirkey, const char* dirnm,
					const char* desc="Custom data" )
			    : selkey_(dirkey,0)
			    , dirname_(dirnm)
			    , desc_(desc)		{}

	MultiID		selkey_; //!< Make sure your selkey.groupID() > 200000
				 //!< Lower than that will be refused!
				 //!< Example: "218745"
	BufferString	dirname_; //!< The subdirectory name in the tree
	BufferString	desc_; //!< Short description, mainly for error messages
			       //!< Example: "Geostatistical data"

	bool		operator ==( const CustomDirData& cdd ) const
			{ return selkey_.groupID() == cdd.selkey_.groupID(); }
    };

    static const MultiID& addCustomDataDir(const CustomDirData&);
			//!< Need to do this only once per OD run
			//!< At survey change, dir will automatically be added

    CNotifier<IOMan, const MultiID&>	entryRemoved;
    CNotifier<IOMan, const MultiID&>	entryAdded;
    CNotifier<IOMan, const MultiID&>	entryChanged;

    Notifier<IOMan>	newIODir;
    Notifier<IOMan>	surveyToBeChanged;  //!< Before the change
    Notifier<IOMan>	surveyChanged;	    //!< To restore OD to normal state
    Notifier<IOMan>	afterSurveyChange;  //!< When operating in normal state
    Notifier<IOMan>	applicationClosing; //!< 'Final' call ...

    static bool		isValidDataRoot(const char* dirnm);
    static bool		isValidSurveyDir(const char* dirnm);
    static BufferString getNewTempDataRootDir();

    static void		setTempSurvey(const SurveyDiskLocation&);
    static void		cancelTempSurvey();

private:

    enum State		{ Bad, NeedInit, Good };

    BufferString	rootdir_;
    State		state_ = NeedInit;
    IODir*		dirptr_ = nullptr;
    int			curlvl_;
    uiString		msg_;
    bool		survchgblocked_ = false;
    mutable Threads::Lock lock_;

    void		init();
    void		reInit(bool dotrigger);
			IOMan(const char* rd=0);
			~IOMan();

    static IOMan*	theinst_;
    static void		setupCustomDataDirs(int);

    bool		setDir(const char*);
    int			levelOf(const char* dirnm) const;
    int			curLevel() const	{ return curlvl_; }
    bool		to(const IOSubDir*,bool);
    IOObj*		crWriteIOObj(const CtxtIOObj&,const MultiID&,int) const;
    bool		writeSettingsSurveyFile(const char* dirnm,
						uiRetVal&) const;
    void		applClosing() { applicationClosing.trigger(); }
    void		getObjEntry(CtxtIOObj&,bool isnew, bool newistmp=false,
				 int translidxingroup=-1);

    friend class	SurveyDataTreePreparer;
    friend class	BatchProgram;
    friend class	ServiceMgrBase;
    friend class	uiMain;
    friend class	IOMManager;
    friend mGlobal(General)	IOMan&	IOM();

public:

    // Don't use these functions unless you really know what you're doing

    bool		setRootDir(const char*);
    static bool		validSurveySetup(BufferString& errmsg);
    static IOSubDir*	getIOSubDir(const CustomDirData&);

    void		setChangeSurveyBlocked( bool yn )
					{ survchgblocked_ = yn; }
    bool		changeSurveyBlocked() const
					{ return survchgblocked_; }
    static bool		newSurvey(SurveyInfo* newsi=0);
			/*!< set new SurveyInfo; force re-read the data tree. */
    static bool		setSurvey(const char*);
			/*!< will remove existing IO manager and
			     set the survey to 'name', thus bypassing the
			     .od/survey file */
    static void		surveyParsChanged();
			/*! Triggers the post-survey change notifiers */

    uiRetVal		setDataSource(const char* dataroot,const char* survdir,
				      bool refresh=false);
    uiRetVal		setDataSource(const char* fullpath,bool refresh=false);
    uiRetVal		setDataSource(const IOPar&,bool refresh=false);
    uiRetVal		setDataSource(const CommandLineParser&,
				      bool refresh=false);
    BufferString	fullSurveyPath() const;
    bool		recordDataSource(const SurveyDiskLocation&,
					 uiRetVal&) const;
			/*!< records dataroot in settings,
			     and project in survey file */
    IODir*		getDir(const char* trlgrpnm) const;
    IODir*		getDir(IOObjContext::StdSelType) const;
    IODir*		getDir(const MultiID&) const;
    bool		ensureUniqueName(IOObj&);
};


mGlobal(General) IOMan& IOM();


mExpClass(General) SurveyChanger
{
public:
				SurveyChanger(const SurveyDiskLocation&);
				~SurveyChanger();

    static bool			hasChanged();
    static SurveyDiskLocation	changedToSurvey();

private:
    bool			needscleanup_	= false;
};
