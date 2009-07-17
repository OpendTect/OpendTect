#ifndef uiexporti2dhorizon_h
#define uiexporti2dhorizon_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Raman Singh
 Date:          June 2008
 RCS:           $Id: uiexport2dhorizon.h,v 1.3 2009-07-17 10:31:28 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class SurfaceInfo;
class uiListBox;
class uiComboBox;
class uiGenInput;
class uiCheckBox;
class uiFileInput;


/*! \brief Dialog for 2D horizon export */

mClass uiExport2DHorizon : public uiDialog
{
public:
			uiExport2DHorizon(uiParent*,
					  const ObjectSet<SurfaceInfo>&);
			~uiExport2DHorizon();


protected:

    uiComboBox*		horselfld_;
    uiListBox*		linenmfld_;
    uiGenInput*		udffld_;
    uiFileInput*	outfld_;
    uiCheckBox*		wrlnmsbox_;
    uiCheckBox*		zbox_;

    const ObjectSet<SurfaceInfo>&	hinfos_;

    virtual bool	acceptOK(CallBacker*);
    void		horChg(CallBacker*);
    bool		doExport();
};


#endif
