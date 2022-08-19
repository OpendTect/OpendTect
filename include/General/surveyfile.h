#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"

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
		SurveyFile(const char*,bool automount=false);
		SurveyFile(const char*,const char*);
		~SurveyFile();

    uiRetVal	activate();
    uiRetVal	save(TaskRunner* trun=nullptr);
    uiRetVal	mount(bool isNew=false, TaskRunner* trun=nullptr);
    uiRetVal	unmount(bool save=true, TaskRunner* trun=nullptr);
    bool	isMounted() const	{ return mounted_; }
    bool	isOK() const		{ return lasterrs_.isOK(); }

    BufferString	getTempBaseDir() const	{ return tmpbasedir_; }
    BufferString	getSurveyDir() const	{ return surveydirnm_; }
    BufferString	getSurveyFile() const	{ return surveyfile_; }
    uiRetVal		errMsg() const		{ return lasterrs_; }

    static const char* extStr()		{ return "odz"; }
    static const char* bckupExtStr()	{ return BufferString(extStr(),"_bck");}
    static BufferString filtStr();

protected:
    BufferString	tmpbasedir_;
    BufferString	surveyfile_;
    BufferString	surveydirnm_;
    bool		mounted_ = false;
    uiRetVal		lasterrs_;

    void	readSurveyDirNameFromFile();

};
