#ifndef uiimphorizon_h
#define uiimphorizon_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          May 2002
 RCS:           $Id: uiimphorizon.h,v 1.12 2005-04-12 11:08:50 cvsnanne Exp $
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

    uiGroup*		grp;
    uiFileInput*	infld;
    uiGenInput*		xyfld;
    uiScaler*		scalefld;
    uiBinIDSubSel*	subselfld;
    uiGenInput*		udffld;
    uiGenInput*		interpolfld;
    uiGenInput*		stepoutfld;
    uiIOObjSel*		outfld;
    uiCheckBox*		displayfld;

    virtual bool	acceptOK(CallBacker*);
    bool		checkInpFlds();
    bool		getFileNames(BufferStringSet&) const;
    bool		readFiles(ObjectSet<BinIDValueSet>&,bool,
	    			  const HorSampling*);
    bool		doWork();
    BinIDValueSet*	getBidValSet(const char*,bool,const HorSampling*);
    bool		interpolateGrid(ObjectSet<BinIDValueSet>&);

    void		inputCB(CallBacker*);
    void		interpolSel(CallBacker*);
    void		scanFile(CallBacker*);

    CtxtIOObj&		ctio;
    EM::ObjectID	emobjid;
};


#endif
