#ifndef uiexphorizon_h
#define uiexphorizon_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          August 2002
 RCS:           $Id: uiexphorizon.h,v 1.3 2002-09-17 13:26:13 bert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiFileInput;
class uiGenInput;
class uiLabel;
class uiLabeledListBox;
class BinIDZValue;
class HorizonInfo;


/*! \brief Dialog for horizon export */

class uiExportHorizon : public uiDialog
{
public:
			uiExportHorizon(uiParent*,
					const ObjectSet<HorizonInfo>&);
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

    const ObjectSet<HorizonInfo>&	hinfos_;
    int					selinfo_;
};


#endif
