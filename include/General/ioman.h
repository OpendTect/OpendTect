#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "ctxtioobj.h"
#include "surveydisklocation.h"

class CommandLineParser;
class IODir;
class IOMManager;
class IOObjContext;
class IOSubDir;
class SurveyInfo;
class SurveyDataTreePreparer;

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

    static bool		isOK();

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
    BufferString	curDirName() const;	//!< OS dir name
    const SurveyDiskLocation& rootDir() const		{ return rootdir_; }
			//!< Full path to the root of the current project
    bool		isKey(const char* keystr) const;
    const char*		nameOf(const char* keystr) const;
			//!< if keystr is not an IOObj key, will return keystr
    const char*		nameOf(const MultiID&) const;
    const char*		objectName(const DBKey&) const;
    static void		getObjectNames(const DBKeySet&,BufferStringSet&);

    MultiID		createNewKey(const MultiID& dirkey);

    bool		to(IOObjContext::StdSelType,bool force_reread=false);
    bool		to(const MultiID&,bool force_reread=false);
    bool		toRoot( bool force_reread=false )
			{ return to( 0, force_reread ); }

    void		getEntry(CtxtIOObj&,bool newistmp=false,
				 int translidxingroup=-1);
				//!< will create a new entry if necessary
    void		getNewEntry(CtxtIOObj&,bool newistmp=false,
				 int translidxingroup=-1);
				//!< will create a new entry if necessary
    bool		commitChanges(const IOObj&);
    bool		permRemove(const MultiID&);
				//!< Removes only entry in IODir

    BufferString	surveyName() const;

    mExpClass(General) CustomDirData
    {
    public:
			CustomDirData(int dirkey,const char* dirnm,
				      const char* desc="Custom data");
			~CustomDirData();

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

    CNotifier<IOMan,const MultiID&>	entryRemoved;
    CNotifier<IOMan,const MultiID&>	entryAdded;
    CNotifier<IOMan,const MultiID&>	entryChanged;
    CNotifier<IOMan,const MultiID&>	implUpdated;

    Notifier<IOMan>	newIODir;
    Notifier<IOMan>	prepareSurveyChange;  //!< Prepare, don't tear down
    Notifier<IOMan>	surveyToBeChanged;  //!< Before the change, tear down
    Notifier<IOMan>	surveyChanged;	    //!< To restore OD to normal state
    Notifier<IOMan>	afterSurveyChange;  //!< When operating in normal state
    Notifier<IOMan>	applicationClosing; //!< 'Final' call ...

    static Notifier<IOMan>& iomReady();

    static uiRetVal	isValidDataRoot(const char* dirnm);
			//!< Full path to a writable directory
    static uiRetVal	isValidSurveyDir(const char* dirnm);
			//!< Full path to an OpendTect project directory
    static bool		prepareDataRoot(const char* dirnm);
    static BufferString getNewTempDataRootDir();

    uiRetVal		setTempSurvey(const SurveyDiskLocation&);
    uiRetVal		cancelTempSurvey();
    bool		isUsingTempSurvey() const { return prevrootdir_; }

    static bool		isPreparedForSurveyChange();

private:

    enum State		{ Bad, NeedInit, Good };

    SurveyDiskLocation	rootdir_;
    SurveyDiskLocation* prevrootdir_ = nullptr;
    State		state_ = NeedInit;
    IODir*		dirptr_ = nullptr;
    int			curlvl_;
    uiString		msg_;
    bool		survchgblocked_ = false;
    mutable Threads::Lock lock_;

			IOMan(const FilePath& rootdir);
			~IOMan();

    uiRetVal		init(SurveyInfo* newsi);
			//<! newsi becomes mine
    uiRetVal		reInit(SurveyInfo* newsi);
			//<! newsi becomes mine
    bool		close(bool dotrigger);

    static void		setupCustomDataDirs(int);

    uiRetVal		setRootDir(const FilePath&,bool ischecked=false);
			//!< fullPath to the survey directory
    uiRetVal		setDir(const char*);
    int			levelOf(const char* dirnm) const;
    int			curLevel() const	{ return curlvl_; }
    bool		to(const IOSubDir*,bool);
    bool		doReloc(const MultiID&,Translator*,
				IOStream&,IOStream&);
    IOObj*		crWriteIOObj(const CtxtIOObj&,const MultiID&,int) const;
    static bool		writeSettingsSurveyFile(const char* dirnm,uiRetVal&);
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

    static bool		validSurveySetup(BufferString& errmsg);
    static IOSubDir*	getIOSubDir(const CustomDirData&);

    void		setChangeSurveyBlocked( bool yn )
					{ survchgblocked_ = yn; }
    bool		changeSurveyBlocked() const
					{ return survchgblocked_; }
    static uiRetVal	newSurvey(SurveyInfo* newsi=nullptr);
			/*!< set new SurveyInfo; force re-read the data tree.
			     newsi becomes mine				 */
    mDeprecated("Use IOMan::setDataSource")
    static uiRetVal	setSurvey(const char* surveydirnm);
			/*!< will remove existing IO manager and
			     set the survey to 'name', thus bypassing the
			     .od/survey file */
    static void		surveyParsChanged();
			/*! Triggers the post-survey change notifiers */

    static uiRetVal	setDataSource(const char* dataroot,const char* survdir,
				      bool refresh=false);
    static uiRetVal	setDataSource(const char* fullpath,bool refresh=false);
    static uiRetVal	setDataSource(const IOPar&,bool refresh=false);
    static uiRetVal	setDataSource(const CommandLineParser&,
				      bool refresh=false);
    static bool		recordDataSource(const SurveyDiskLocation&,uiRetVal&);
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

    uiRetVal			message() const { return msg_; }

    static bool			hasChanged();
    static SurveyDiskLocation	changedToSurvey();

private:

    uiRetVal			msg_;

};
