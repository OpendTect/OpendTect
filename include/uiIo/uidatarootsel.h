#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2016
________________________________________________________________________

-*/

#include "uiiocommon.h"
#include "uigroup.h"

class uiComboBox;


mExpClass(uiIo) uiDataRootSel : public uiGroup
{ mODTextTranslationClass(uiDataRootSel);
public:

			uiDataRootSel(uiParent*,const char* defdir=0);

    BufferString	getDir();
			//!< non-empty is a valid dir
			//!< if invalid, errors have been presented to the user

    static const char*	sKeyRootDirs()	    { return "Known DATA directories"; }
    static const char*	sKeyDefRootDir()    { return "Default DATA directory"; }
    static uiString	userDataRootString();

    Notifier<uiDataRootSel> selectionChanged;

protected:

    uiComboBox*		dirfld_;
    BufferString	previnput_;

    BufferString	getInput() const;
    void		selButCB(CallBacker*);
    void		dirChgCB(CallBacker*);
    void		checkAndSetCorrected(const char*);
    bool		getUsableDir(BufferString&) const;
    bool		isValidFolder(const char*) const;
    static void		addDirNameToSettingsIfNew(const char*,bool);

    friend class	uiFixInvalidDataRoot;
    static bool		setRootDirOnly(const char* dirnm);
    static void		writeDefSurvFile(const char* survdirnm);

public:

    static uiRetVal	setSurveyDirTo(const char* dirnm);
			//!< if not current, will close all scenes and viewers
			//!< thus this is probably not a function for _you_

};
