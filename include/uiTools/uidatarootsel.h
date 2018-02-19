#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2016
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"

class uiComboBox;


mExpClass(uiTools) uiDataRootSel : public uiGroup
{ mODTextTranslationClass(uiDataRootSel);
public:

			uiDataRootSel(uiParent*,const char* defdir=0);

    void 		setDir(const char*);
    BufferString	getDir();
			//!< non-empty is a valid dir
			//!< if invalid, errors have been presented to the user

    static const char*	sKeyRootDirs()	    { return "Known DATA directories"; }
    static const char*	sKeyDefRootDir()    { return "Default DATA directory"; }
    static uiString	userDataRootString();
    BufferString	getInput() const;
    static bool		setRootDirOnly(const char* dirnm);

    Notifier<uiDataRootSel> selectionChanged;

protected:

    uiComboBox*		dirfld_;
    BufferString	previnput_;

    void		selButCB(CallBacker*);
    void		dirChgCB(CallBacker*);
    void		checkAndSetCorrected(const char*);
    uiRetVal		getUsableDir(BufferString&) const;
    uiRetVal		isValidFolder(const char*) const;
    static void		addDirNameToSettingsIfNew(const char*,bool);
    BufferString	addChoice(const char*,bool);
    void		setChoice(const char*);

    friend class	uiFixInvalidDataRoot;
    friend class	uiSurveyManager;
    static void		writeDefSurvFile(const char* survdirnm);
};
