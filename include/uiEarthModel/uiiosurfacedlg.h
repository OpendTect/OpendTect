#ifndef uiiosurfacedlg_h
#define uiiosurfacedlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          July 2003
 RCS:           $Id: uiiosurfacedlg.h,v 1.7 2003-11-07 12:21:54 bert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class IOObj;
class uiSurfaceRead;
class uiSurfaceWrite;
class MultiID;

namespace EM { class Horizon; class SurfaceIODataSelection; };


/*! \brief Dialog for horizon export */

class uiWriteSurfaceDlg : public uiDialog
{
public:
			uiWriteSurfaceDlg(uiParent*,const EM::Horizon&);

    bool		auxDataOnly() const;
    bool		surfaceOnly() const;
    bool		surfaceAndData() const;
    void		getSelection(EM::SurfaceIODataSelection&);
    IOObj*		ioObj() const;

protected:

    int			auxdataidx;

    uiSurfaceWrite*	iogrp;
    const EM::Horizon&	hor;

    bool		checkIfAlreadyPresent(const char*);
    bool		acceptOK(CallBacker*);
};


class uiReadSurfaceDlg : public uiDialog
{
public:
                        uiReadSurfaceDlg(uiParent*);

    IOObj*		ioObj() const;
    void		getSelection(EM::SurfaceIODataSelection&);

protected:

    uiSurfaceRead*        iogrp;

    bool                acceptOK(CallBacker*);
};


#endif
