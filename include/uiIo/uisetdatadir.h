#ifndef uisetdatadir_h
#define uisetdatadir_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          June 2002
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"

class uiFileInput;

mExpClass(uiIo) uiSetDataDir : public uiDialog
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

    void		updateListFld();
    void		rootSelCB(CallBacker*);
    void		rootMoveUpCB(CallBacker*);
    void		rootMoveDownCB(CallBacker*);
    void		rootRemoveCB(CallBacker*);
    bool		writeSettings();

    bool		acceptOK(CallBacker*);

    static void		offerUnzipSurv(uiParent*,const char*);

};

#endif
