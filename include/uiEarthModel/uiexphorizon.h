#ifndef uiexphorizon_h
#define uiexphorizon_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          August 2002
 RCS:           $Id: uiexphorizon.h,v 1.11 2003-11-07 12:21:54 bert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiFileInput;
class uiGenInput;
class BinIDZValue;
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
