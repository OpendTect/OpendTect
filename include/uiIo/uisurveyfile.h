#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"

#include "uistring.h"

class uiParent;
class SurveyFile;

/*!\brief Manage OpendTect survey/project files (SurveyFiles) in a UI setting

  Provides UI methods to work with SurveyFile's (an OpendTect survey/project
  within a zip archive). Actions include:
  - opening an archive via a file selection dialog. After selection the survey
  will be unpacked into a temporary base data folder and made the current active
  survey in the application.
  - creating a new survey via a file selection dialog. The new survey is in a
  temporary base data folder and is made the current active survey in the
  application.
  - closing an archive giving the user the option to save the survey back to a
  survey/project file and cleaning up the temporary base folder

 */

mExpClass(uiIo) uiSurveyFile
{ mODTextTranslationClass(uiSurveyFile);
public:
			uiSurveyFile(uiParent*);
			~uiSurveyFile();

    bool		newFile();
    bool		openFile();
    bool		openFile(const char*);
    bool		closeFile();

protected:
    uiParent*		parent_		= nullptr;
    SurveyFile*		survfile_	= nullptr;
};
