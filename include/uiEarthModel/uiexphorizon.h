#ifndef uiexphorizon_h
#define uiexphorizon_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          August 2002
 RCS:           $Id: uiexphorizon.h,v 1.2 2002-08-12 14:20:55 nanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiFileInput;
class uiGenInput;
class uiLabel;
class uiLabeledListBox;
class BinIDValue;


/*! \brief Dialog for horizon export */

class uiExportHorizon : public uiDialog
{
public:
			uiExportHorizon(uiParent*,
					const ObjectSet<BufferString>&,
					const TypeSet<int>&,
					const ObjectSet<BufferString>&);
			~uiExportHorizon();

    const char*		selectedItem();
    int			selHorID()		{ return selhorid; }
    bool		writeAscii(const TypeSet<BinIDValue>&,
				   const TypeSet<float>&);

protected:

    uiLabeledListBox*	inbox;
    uiFileInput*	outfld;
    uiGenInput*		xyfld;
    uiGenInput*		zfld;
    uiLabel*		attrlbl;

    virtual bool	acceptOK(CallBacker*);
    void		selChg(CallBacker*);

    int			selhorid;
    const TypeSet<int>& horids;
    const ObjectSet<BufferString>&	attribs;
};


#endif
