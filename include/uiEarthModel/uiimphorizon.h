#ifndef uiimphorizon_h
#define uiimphorizon_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          May 2002
 RCS:           $Id: uiimphorizon.h,v 1.13 2005-09-06 09:35:19 cvsnanne Exp $
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
class uiColorInput;
class uiFileInput;
class uiGenInput;
class uiIOObjSel;
class uiPushButton;
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

    uiGroup*		midgrp;
    uiFileInput*	infld;
    uiGenInput*		xyfld;
    uiScaler*		scalefld;
    uiBinIDSubSel*	subselfld;
    uiGenInput*		udffld;
    uiGenInput*		interpolfld;
    uiGenInput*		stepoutfld;
    uiIOObjSel*		outfld;
    uiCheckBox*		displayfld;
    uiPushButton*	attribbut;
    uiColorInput*	colbut;

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
    void		attribSel(CallBacker*);

    CtxtIOObj&		ctio_;
    EM::ObjectID	emobjid_;
    BufferStringSet&	attribnames_;
    BoolTypeSet		attribsel_;
};


#endif
