#ifndef uiexphorizon_h
#define uiexphorizon_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          August 2002
 RCS:           $Id: uiexphorizon.h,v 1.5 2002-09-20 13:51:51 bert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiFileInput;
class uiGenInput;
class uiLabel;
class uiLabeledListBox;
class BinIDZValue;
class SurfaceInfo;


/*! \brief Dialog for horizon export */

class uiExportHorizon : public uiDialog
{
public:
			uiExportHorizon(uiParent*,
					const ObjectSet<SurfaceInfo>&);
			~uiExportHorizon();

    const char*		selectedItem();
    int			selHorID()const;
    bool		writeAscii(const ObjectSet< TypeSet<BinIDZValue> >&);

protected:

    uiLabeledListBox*	inbox;
    uiFileInput*	outfld;
    uiGenInput*		typfld;
    uiGenInput*		zfld;
    uiGenInput*		gfnmfld;
    uiGenInput*		gfcommfld;
    uiGenInput*		gfunfld;
    uiGroup*		gfgrp;
    uiLabel*		attrlbl;

    virtual bool	acceptOK(CallBacker*);
    void		selChg(CallBacker*);
    void		typChg(CallBacker*);

    const ObjectSet<SurfaceInfo>&	hinfos_;
    int					selinfo_;
};


#endif
