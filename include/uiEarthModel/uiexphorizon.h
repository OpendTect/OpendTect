#ifndef uiexphorizon_h
#define uiexphorizon_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          August 2002
 RCS:           $Id: uiexphorizon.h,v 1.8 2003-07-29 13:03:19 nanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiFileInput;
class uiGenInput;
class BinIDZValue;
class uiSurfaceSel;


/*! \brief Dialog for horizon export */

class uiExportHorizon : public uiDialog
{
public:
			uiExportHorizon(uiParent*);
			~uiExportHorizon();

    bool		writeAscii(const ObjectSet< TypeSet<BinIDZValue> >&);

protected:

    uiSurfaceSel*	infld;
    uiFileInput*	outfld;
    uiGenInput*		typfld;
    uiGenInput*		zfld;
    uiGenInput*		gfnmfld;
    uiGenInput*		gfcommfld;
    uiGenInput*		gfunfld;
    uiGroup*		gfgrp;

    virtual bool	acceptOK(CallBacker*);
    void		typChg(CallBacker*);
};


#endif
