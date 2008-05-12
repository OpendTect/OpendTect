#ifndef uiexpfault_h
#define uiexpfault_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          May 2008
 RCS:           $Id: uiexpfault.h,v 1.1 2008-05-12 03:57:24 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiFileInput;
class uiGenInput;
class uiSurfaceRead;


/*! \brief Dialog for horizon export */

class uiExportFault : public uiDialog
{
public:
			uiExportFault(uiParent*);
			~uiExportFault();

protected:

    uiSurfaceRead*	infld_;
    uiGenInput*		typfld_;
    uiFileInput*	outfld_;

    virtual bool	acceptOK(CallBacker*);
    void		typeChg(CallBacker*);
    bool		writeAscii();
};

#endif
