#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"
#include "survinfo.h"

#include "uistringset.h"

class SurveyChanger;
class TaskRunner;
namespace OD { namespace JSON { class Object; } }

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

mExpClass(General) SurveyCreator
{ mODTextTranslationClass(SurveyCreator)
public:

    virtual		~SurveyCreator();

    virtual uiRetVal	mount(bool isnew=false,TaskRunner* =nullptr);
			//<! Creates the project at the target location
    virtual uiRetVal	activate();
			//<! The project becomes the current project for IOM()
    virtual uiRetVal	deactivate();
			//<! Restores the previous active project (if any)
    virtual uiRetVal	save(TaskRunner* =nullptr);
			//<! Saves the mounted project in it current state
    virtual uiRetVal	unmount(bool save=true,TaskRunner* =nullptr);
			//<! Removes the entire data root folder

    inline void		setManaged( bool yn )	{ ismanaged_ = yn; }
			/*<! Managed projects get deleted with their data root
			     upon unmounting. */

    bool		isOK() const;
    bool		isMounted() const	{ return mounted_; }
    bool		isManaged() const	{ return ismanaged_; }

    BufferString	getTempBaseDir() const;
			/*<! Full path to the temporary folder containing the
			     project data */
    BufferString	getSurveyDir() const;
			//<! Survey directory name (only), not a full path
    BufferString	getSurveyNm() const	{ return surveynm_; }

    uiRetVal		errMsg() const		{ return lasterrs_; }
    uiRetVal		warningMsg() const	{ return lastwarning_; }

    virtual BufferString getZipArchiveLocation() const			= 0;
    static const char*	extStr()			{ return "odz"; }
    static BufferString bckupExtStr()
				{ return BufferString( extStr(), "_bck" ); }

protected:
			SurveyCreator(const char* survnm,const char* dataroot,
				      bool ismanaged);

    virtual bool	createSurvey(TaskRunner*)			= 0;

    BufferString	surveynm_;
    SurveyDiskLocation* surveyloc_	= nullptr;
    bool		ismanaged_	= true;
    bool		owndataroot_	= false;
    bool		mounted_ = false;
    SurveyChanger*	changer_	= nullptr;

    mutable uiRetVal	lasterrs_;
    mutable uiRetVal	lastwarning_;
};


mExpClass(General) SurveyFile : public SurveyCreator
{ mODTextTranslationClass(SurveyFile)
public:
			SurveyFile(const char* zipfile,bool automount=false,
				   bool ismanaged=true);
			mOD_DisableCopy(SurveyFile);
			~SurveyFile();

    uiRetVal		activate() override;

    BufferString	getSurveyFile() const		{ return surveyfile_; }
    BufferString	getZipArchiveLocation() const override
							{ return surveyfile_;  }

    static BufferString filtStr();

protected:

    void		readSurveyDirNameFromFile();
    bool		createSurvey(TaskRunner*) override;

    BufferString	surveyfile_;

};


mExpClass(General) EmptyTempSurvey : public SurveyCreator
{ mODTextTranslationClass(EmptyTempSurvey)
public:

			EmptyTempSurvey(const char* survnm =nullptr,
					const char* dataroot =nullptr,
					bool automount=false,
					bool ismanaged=true);
			EmptyTempSurvey(const OD::JSON::Object&,
					bool automount=false,
					bool ismanaged=false);
			mOD_DisableCopy(EmptyTempSurvey);
			~EmptyTempSurvey();

    BufferString	getZipArchiveLocation() const override
						{ return zipfileloc_;  }
    void		setSaveLocation(const char* saveloc =nullptr);

    static const char*	sKeyCRSID()	{ return "CRSID"; }
    static const char*	sKeySaveLoc()	{ return "Save Location"; }

protected:

    bool		createSurvey(TaskRunner*) override;
    bool		writeSurveyInfo();

    OD::JSON::Object*	createpars_	= nullptr;
    BufferString	zipfileloc_;
};
