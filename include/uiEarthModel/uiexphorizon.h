#ifndef uiexphorizon_h
#define uiexphorizon_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          August 2002
 RCS:           $Id: uiexphorizon.h,v 1.15 2009-07-16 12:41:41 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiFileInput;
class uiGenInput;
class uiSurfaceRead;
class uiCheckBox;


/*! \brief Dialog for horizon export */

mClass uiExportHorizon : public uiDialog
{
public:
			uiExportHorizon(uiParent*);
			~uiExportHorizon();


protected:

    uiSurfaceRead*	infld_;
    uiFileInput*	outfld_;
    uiGenInput*		typfld_;
    uiGenInput*		zfld_;
    uiCheckBox*		zbox_;
    uiGenInput*		udffld_;
    uiGenInput*		gfnmfld_;
    uiGenInput*		gfcommfld_;
    uiGroup*		gfgrp_;

    virtual bool	acceptOK(CallBacker*);
    void		typChg(CallBacker*);
    void		addZChg(CallBacker*);
    void		attrSel(CallBacker*);
    void		inpSel(CallBacker*);
    bool		writeAscii();
};


#endif
