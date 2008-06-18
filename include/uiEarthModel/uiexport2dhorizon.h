#ifndef uiexporti2dhorizon_h
#define uiexporti2dhorizon_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Raman Singh
 Date:          June 2008
 RCS:           $Id: uiexport2dhorizon.h,v 1.1 2008-06-18 11:42:47 cvsraman Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class SurfaceInfo;
class uiComboBox;
class uiFileInput;
class uiGenInput;
class uiListBox;


/*! \brief Dialog for 2D horizon export */

class uiExport2DHorizon : public uiDialog
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

    const ObjectSet<SurfaceInfo>&	hinfos_;

    virtual bool	acceptOK(CallBacker*);
    void		horChg(CallBacker*);
    bool		doExport();
};


#endif
