#ifndef uiimphorizon_h
#define uiimphorizon_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          May 2002
 RCS:           $Id: uiimphorizon.h,v 1.10 2005-02-10 16:23:05 nanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "multiid.h"
#include "emposid.h"

class BinIDValueSet;
class BufferStringSet;
class CtxtIOObj;
class HorSampling;
class uiBinIDSubSel;
class uiCheckBox;
class uiFileInput;
class uiGenInput;
class uiIOObjSel;
class uiScaler;



/*! \brief Dialog for horizon selection */

class uiImportHorizon : public uiDialog
{
public:
			uiImportHorizon(uiParent*);
			~uiImportHorizon();

    bool		doDisplay() const;
    MultiID		getSelID() const;

protected:

    uiFileInput*	infld;
    uiGenInput*		xyfld;
    uiScaler*		scalefld;
    uiBinIDSubSel*	subselfld;
    uiGenInput*		udffld;
    uiGenInput*		fillholesfld;
    uiIOObjSel*		outfld;
    uiCheckBox*		displayfld;

    virtual bool	acceptOK(CallBacker*);
    bool		checkInpFlds();
    bool		getFileNames(BufferStringSet&) const;
    bool		readFiles(ObjectSet<BinIDValueSet>&,bool,
	    			  const HorSampling*);
    bool		doWork();
    BinIDValueSet*	getBidValSet(const char*,bool,const HorSampling*);

    void		scanFile(CallBacker*);
    bool		analyzeData(bool&,bool&);

    CtxtIOObj&		ctio;
    EM::ObjectID	emobjid;
};


#endif
