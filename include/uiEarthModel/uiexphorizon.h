#ifndef uiexphorizon_h
#define uiexphorizon_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2002
 RCS:           $Id: uiexphorizon.h,v 1.18 2010-02-23 10:17:22 cvsraman Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "fixedstring.h"

class uiFileInput;
class uiGenInput;
class uiSurfaceRead;
class uiCheckBox;
class uiUnitSel;
class uiPushButton;
class uiZAxisTransformSel;


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
    uiZAxisTransformSel* transfld_;
    uiPushButton*	settingsbutt_;
    uiUnitSel*		unitsel_;
    uiGenInput*		udffld_;

    BufferString	gfname_;
    BufferString	gfcomment_;

    virtual bool	acceptOK(CallBacker*);
    void		typChg(CallBacker*);
    void		addZChg(CallBacker*);
    void		attrSel(CallBacker*);
    void		settingsCB(CallBacker*);
    void		inpSel(CallBacker*);
    bool		writeAscii();

    FixedString		getZDomain() const;
};


#endif
