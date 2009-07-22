#ifndef uisetdatadir_h
#define uisetdatadir_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          June 2002
 RCS:           $Id: uisetdatadir.h,v 1.4 2009-07-22 16:01:22 cvsbert Exp $
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
