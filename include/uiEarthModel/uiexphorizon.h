#ifndef uiexphorizon_h
#define uiexphorizon_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          August 2002
 RCS:           $Id: uiexphorizon.h,v 1.12 2004-07-14 15:26:50 bert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiFileInput;
class uiGenInput;
class uiSurfaceRead;


/*! \brief Dialog for horizon export */

class uiExportHorizon : public uiDialog
{
public:
			uiExportHorizon(uiParent*);
			~uiExportHorizon();


protected:

    uiSurfaceRead*	infld;
    uiFileInput*	outfld;
    uiGenInput*		typfld;
    uiGenInput*		zfld;
    uiGenInput*		gfnmfld;
    uiGenInput*		gfcommfld;
    uiGenInput*		gfunfld;
    uiGroup*		gfgrp;

    virtual bool	acceptOK(CallBacker*);
    void		typChg(CallBacker*);
    bool		writeAscii();
};


#endif
