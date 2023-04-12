#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"

#include "odjson.h"
#include "survinfo.h"

#include "uistringset.h"

class TaskRunner;

/*!\brief Utility class to work with OpendTect survey/project zip files.

  Encapsulates actions on a OpendTect survey/project zip file including:
  -  unpacking to a temporary base data folder (mount)
  -  making the unpacked survey the active survey (activate)
  -  cleaning up the unpacked folder back and optionally saving the survey back
     to a zip file with backup (unmount)
  The default extension for the survey/project files is ".odz". Backup files
  have an extension of "odz_bck".
  The destructor automatically deletes the temporary base folder and unpacked
  survey without saving.
 */

mExpClass(General) SurveyFile
{ mODTextTranslationClass(SurveyFile);
public:
		SurveyFile(const char* zipfile,bool automount=false);
		mDeprecatedObs
		SurveyFile(const char* zipfile,const char*);
		~SurveyFile();

    uiRetVal	activate();
    uiRetVal	deactivate();
    uiRetVal	save(TaskRunner* trun=nullptr);
    uiRetVal	mount(bool isNew=false, TaskRunner* trun=nullptr);
    uiRetVal	unmount(bool save=true, TaskRunner* trun=nullptr);

    bool	isMounted() const	{ return mounted_; }
    bool	isOK() const		{ return lasterrs_.isOK(); }
    bool	isManaged() const;

    BufferString	getTempBaseDir() const	{ return tmpbasedir_; }
    BufferString	getSurveyDir() const	{ return surveydirnm_; }
    BufferString	getSurveyFile() const	{ return surveyfile_; }
    const BufferString	getZipArchiveLocation() const
						    { return surveyfile_;  }
    uiRetVal		errMsg() const		{ return lasterrs_; }

    void		setManaged(bool yn);

    static const char* extStr()		{ return "odz"; }
    static const char* bckupExtStr();
    static BufferString filtStr();

protected:
    BufferString	tmpbasedir_;
    BufferString	surveyfile_;
    BufferString	surveydirnm_;
    bool		mounted_ = false;
    uiRetVal		lasterrs_;

    void		readSurveyDirNameFromFile();
    bool		createSurvey(TaskRunner*);

};


mExpClass(General) EmptyTempSurvey
{ mODTextTranslationClass(EmptyTempSurvey)
public:
			EmptyTempSurvey(const char* survnm=nullptr,
					const char* dataroot=nullptr,
					bool automount=false);
			EmptyTempSurvey(const OD::JSON::Object&);
			~EmptyTempSurvey();

    static const char*	    sKeyCRSID()     { return "CRSID"; }
    static const char*	    sKeySaveLoc()   { return "Save Location"; }

    uiRetVal		activate();
    uiRetVal		deactivate();
    uiRetVal		save(TaskRunner* trun=nullptr);
    uiRetVal		mount(bool isNew=false, TaskRunner* trun=nullptr);
    uiRetVal		unmount(bool save=true, TaskRunner* trun=nullptr);
    const BufferString	getZipArchiveLocation() const
						    { return zipfileloc_;  }
    void		setSaveLocation(const char* saveloc);

    inline void		setManaged( bool ismanaged ) { ismanaged_ = ismanaged; }

    bool		isMounted() const	{ return mounted_; }
    bool		isOK() const;
    bool		isManaged() const	{ return ismanaged_; }

    BufferString	getTempBaseDir() const;
    BufferString	getSurveyDir() const;
    BufferString	getSurveyNm() const	{ return surveydirnm_; }

    uiRetVal		errMsg() const		{ return lasterrs_; }
    uiRetVal		warningMsg() const	{ return lastwarning_; }

    mDeprecatedObs
    BufferString	getSurveyFile() const	{ return surveyfile_; }

protected:

    bool		    createOMFFile();
    bool		    createTempSurveySetup(bool hasomf);
    bool		    fillSurveyInfo(const OD::JSON::Object* obj);
    bool		    initSurvey(const OD::JSON::Object* obj=nullptr);

    BufferString		origsurveyfp_;
    SurveyInfo			si_;
    bool			ismanaged_	= true;
    BufferString		saveloc_;
    BufferString		zipfileloc_;
    BufferString		tmpbasedir_;
    BufferString		surveyfile_;
    BufferString		surveydirnm_;
    bool			mounted_ = false;
    uiRetVal			lasterrs_;
    uiRetVal			lastwarning_;
};
