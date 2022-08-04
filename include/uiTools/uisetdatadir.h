#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          June 2002
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"

class uiFileInput;
class uiListBox;

mExpClass(uiTools) uiSetDataDir : public uiDialog
{ mODTextTranslationClass(uiSetDataDir);
public:
			uiSetDataDir(uiParent*);
			~uiSetDataDir();

    const char*		selectedDir() const	{ return seldir_; }
    static bool		setRootDataDir(uiParent*,const char*);

protected:

    BufferString	seldir_;
    const BufferString	curdatadir_;
    uiFileInput*	basedirfld_;

    BufferStringSet	dirlist_;
    uiListBox*		dirlistfld_;

    void		updateListFld();
    void		rootCheckCB(CallBacker*);
    void		rootSelCB(CallBacker*);
    void		rootMoveUpCB(CallBacker*);
    void		rootMoveDownCB(CallBacker*);
    void		rootRemoveCB(CallBacker*);
    bool		writeSettings();

    bool		acceptOK(CallBacker*) override;

    static void		offerUnzipSurv(uiParent*,const char*);

};

