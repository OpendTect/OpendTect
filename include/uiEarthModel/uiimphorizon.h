#ifndef uiimphorizon_h
#define uiimphorizon_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2002
 RCS:           $Id: uiimphorizon.h,v 1.24 2008-02-18 11:00:47 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class BinIDValueSet;
class BufferStringSet;
class CtxtIOObj;
class HorizonScanner;
class IOObj;
class MultiID;
class uiCheckBox;
class uiColorInput;
class uiLabeledListBox;
class uiFileInput;
class uiGenInput;
class uiImpHorArr2DInterpPars;
class uiIOObjSel;
class uiPushButton;
class uiPosSubSel;
class uiScaler;
class uiStratLevelSel;
class uiTableImpDataSel;
namespace Table { class FormatDesc; }
namespace EM { class Horizon3D; }

/*! \brief Dialog for Horizon Import */

class uiImportHorizon : public uiDialog
{
public:
			uiImportHorizon(uiParent*,bool);
			~uiImportHorizon();

    bool                doDisplay() const;
    MultiID             getSelID() const;

protected:

    uiFileInput*	inpfld_;
    uiPushButton*       scanbut_;
    uiLabeledListBox*	attrlistfld_;
    uiPushButton*	addbut_;
    uiPosSubSel*	subselfld_;
    uiGenInput*		filludffld_;
    uiImpHorArr2DInterpPars*	arr2dinterpfld_;
    uiTableImpDataSel*  dataselfld_;
    uiColorInput*       colbut_;
    uiStratLevelSel*    stratlvlfld_;
    uiIOObjSel*		outputfld_;
    uiCheckBox*         displayfld_;

    virtual bool	acceptOK(CallBacker*);
    void                descChg(CallBacker*);
    void		formatSel(CallBacker*);
    void		addAttrib(CallBacker*);
    void		scanPush(CallBacker*);
    void                fillUdfSel(CallBacker*);
    void                stratLvlChg(CallBacker*);

    bool		getFileNames(BufferStringSet&) const;
    bool		checkInpFlds();
    bool		doScan();
    bool		doImport();
    bool                fillUdfs(ObjectSet<BinIDValueSet>&);
    EM::Horizon3D*	createHor() const;
    EM::Horizon3D*      loadHor();

    CtxtIOObj&		ctio_;
    Table::FormatDesc&  fd_;
    HorizonScanner*	scanner_;
    bool		isgeom_;
};


#endif
