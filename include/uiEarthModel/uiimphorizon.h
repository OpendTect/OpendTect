#ifndef uiimphorizon_h
#define uiimphorizon_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          May 2002
 RCS:           $Id: uiimphorizon.h,v 1.16 2006-07-20 16:30:21 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "multiid.h"
#include "emposid.h"
#include "cubesampling.h"

class BinIDValueSet;
class BufferStringSet;
class CtxtIOObj;
class uiBinIDSubSel;
class uiCheckBox;
class uiColorInput;
class uiFileInput;
class uiGenInput;
class uiIOObjSel;
class uiPushButton;
class uiScaler;
class uiImpHorArr2DInterpPars;


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
    uiGenInput*		udfvalfld;
    uiImpHorArr2DInterpPars* arr2dinterpfld;
    uiGenInput*		filludffld;
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
    bool		fillUdfs(ObjectSet<BinIDValueSet>&);

    void		inputCB(CallBacker*);
    void		fillUdfSel(CallBacker*);
    void		scanFile(CallBacker*);
    void		attribSel(CallBacker*);

    CtxtIOObj&		ctio_;
    EM::ObjectID	emobjid_;
    BufferStringSet&	attribnames_;
    BoolTypeSet		attribsel_;
    HorSampling		filehs_;
};


#endif
