#ifndef uiexphorizon_h
#define uiexphorizon_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          August 2002
 RCS:           $Id: uiexphorizon.h,v 1.4 2002-09-19 14:59:07 kristofer Exp $
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
    uiGenInput*		xyfld;
    uiGenInput*		zfld;
    uiLabel*		attrlbl;

    virtual bool	acceptOK(CallBacker*);
    void		selChg(CallBacker*);

    const ObjectSet<SurfaceInfo>&	hinfos_;
    int					selinfo_;
};


#endif
