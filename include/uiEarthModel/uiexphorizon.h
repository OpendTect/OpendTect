#ifndef uiexphorizon_h
#define uiexphorizon_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          August 2002
 RCS:           $Id: uiexphorizon.h,v 1.1 2002-08-08 10:33:12 nanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiFileInput;
class uiGenInput;
class uiLabeledListBox;
class BinIDValue;


/*! \brief Dialog for horizon export */

class uiExportHorizon : public uiDialog
{
public:
			uiExportHorizon(uiParent*,
					const ObjectSet<BufferString>&);
			~uiExportHorizon();

    const char*		selectedItem();
    bool		writeAscii(const TypeSet<BinIDValue>&,
				   const TypeSet<float>&);

protected:

    uiLabeledListBox*	inbox;
    uiFileInput*	outfld;
    uiGenInput*		xyfld;
    uiGenInput*		zfld;

    virtual bool	acceptOK(CallBacker*);
};


#endif
