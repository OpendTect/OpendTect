#ifndef uisetdatadir_h
#define uisetdatadir_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          June 2002
 RCS:           $Id: uisetdatadir.h,v 1.3 2009-01-08 07:23:07 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class uiGenInput;
class uiFileInput;

mClass uiSetDataDir : public uiDialog
{
public:
			uiSetDataDir(uiParent*);

    static bool		isOK(const char* dirnm=0); // if null, std data dir
    static bool		setRootDataDir(const char*);

protected:

    BufferString	olddatadir;

    uiGenInput*		oddirfld;
    uiFileInput*	basedirfld;

    bool		acceptOK(CallBacker*);
};

#endif
