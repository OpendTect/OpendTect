#ifndef uiiosurfacedlg_h
#define uiiosurfacedlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          July 2003
 RCS:           $Id: uiiosurfacedlg.h,v 1.1 2003-07-16 09:56:15 nanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class CtxtIOObj;
class IOObj;
class uiIOSurface;

namespace EM { class Horizon; };


/*! \brief Dialog for horizon export */

class uiSaveSurfaceDlg : public uiDialog
{
public:
			uiSaveSurfaceDlg(uiParent*,const EM::Horizon&);

    bool		doWrite();

protected:

    uiIOSurface*	iogrp;
    const EM::Horizon&	hor;

    bool		acceptOK(CallBacker*);
};


class uiIOSurfaceDlg : public uiDialog
{
public:
                        uiIOSurfaceDlg(uiParent*,CtxtIOObj&);

    IOObj*              ioObj() const;

protected:

    uiIOSurface*        iogrp;

    bool                acceptOK(CallBacker*);
};


#endif
