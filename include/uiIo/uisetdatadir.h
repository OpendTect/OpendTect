#ifndef uisetdatadir_h
#define uisetdatadir_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          June 2002
 RCS:           $Id: uisetdatadir.h,v 1.5 2010/09/07 04:38:43 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiFileInput;

mClass uiSetDataDir : public uiDialog
{
public:
			uiSetDataDir(uiParent*);

    static bool		isOK(const char* dirnm=0); // if null, std data dir
    static bool		setRootDataDir(const char*);

protected:

    BufferString	olddatadir;
    uiFileInput*	basedirfld;

    bool		acceptOK(CallBacker*);
};

#endif
