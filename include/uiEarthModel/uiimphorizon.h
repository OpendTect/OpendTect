#ifndef uiimphorizon_h
#define uiimphorizon_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2002
 RCS:           $Id: uiimphorizon.h,v 1.19 2007-09-14 06:59:41 cvsraman Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class BinIDValueSet;
class BufferStringSet;
class CtxtIOObj;
class IOObj;
class MultiID;
class uiBinIDSubSel;
class uiCheckBox;
class uiColorInput;
class uiLabeledComboBox;
class uiLabeledListBox;
class uiFileInput;
class uiGenInput;
class uiImpHorArr2DInterpPars;
class uiIOObjSel;
class uiPushButton;
class uiScaler;
class uiStratLevelSel;
class uiTableImpDataSel;
namespace Table { class FormatDesc; }

/*! \brief Dialog for Horizon Import */

class uiImportHorizon : public uiDialog
{
public:
			uiImportHorizon(uiParent*);
			~uiImportHorizon();

    bool                doDisplay() const;
    MultiID             getSelID() const;

protected:

    uiFileInput*	inpfld_;
    uiGenInput*		xyfld_;
    uiLabeledListBox*	attrlistfld_;
    uiPushButton*	addbut_;
    uiScaler*		scalefld_;
    uiBinIDSubSel*	subselfld_;
    uiGenInput*		filludffld_;
    uiImpHorArr2DInterpPars*	arr2dinterpfld_;
    uiTableImpDataSel*  dataselfld_;
    uiColorInput*       colbut_;
    uiStratLevelSel*    stratlvlfld_;
    uiIOObjSel*		outputfld_;
    uiCheckBox*         displayfld_;

    virtual bool	acceptOK(CallBacker*);
    void		formatSel(CallBacker*);
    void		addAttrib(CallBacker*);
    void                fillUdfSel(CallBacker*);
    void                stratLvlChg(CallBacker*);

    bool		getFileNames(BufferStringSet&) const;
    bool		checkInpFlds();
    bool		doImport();
    bool                fillUdfs(ObjectSet<BinIDValueSet>&);

    CtxtIOObj&		ctio_;
    Table::FormatDesc&  fd_;
};


#endif
