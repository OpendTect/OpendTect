#ifndef uisetdatadir_h
#define uisetdatadir_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          June 2002
 RCS:           $Id: uisetdatadir.h,v 1.1 2004-01-12 15:02:44 bert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class uiGenInput;
class uiFileInput;

class uiSetDataDir : public uiDialog
{
public:
			uiSetDataDir(uiParent*);

    static bool		isOK(const char* dirnm=0); // if null, std data dir

protected:

    BufferString	olddatadir;

    uiGenInput*		oddirfld;
    uiFileInput*	basedirfld;

    bool		acceptOK(CallBacker*);
};

#endif
