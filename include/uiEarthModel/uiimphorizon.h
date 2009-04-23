#ifndef uiimphorizon_h
#define uiimphorizon_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2002
 RCS:           $Id: uiimphorizon.h,v 1.26 2009-04-23 18:08:50 cvskris Exp $
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
class uiArray2DInterpolSel;
class uiIOObjSel;
class uiPushButton;
class uiPosSubSel;
class uiScaler;
class uiStratLevelSel;
class uiTableImpDataSel;
namespace Table { class FormatDesc; }
namespace EM { class Horizon3D; }

/*! \brief Dialog for Horizon Import */

mClass uiImportHorizon : public uiDialog
{
public:
			uiImportHorizon(uiParent*,bool);
			~uiImportHorizon();

    bool                doDisplay() const;
    MultiID             getSelID() const;

protected:

    uiFileInput*	inpfld_;
    uiPushButton*      	scanbut_;
    uiLabeledListBox*	attrlistfld_;
    uiPushButton*	addbut_;
    uiPosSubSel*	subselfld_;
    uiGenInput*		filludffld_;
    uiArray2DInterpolSel*	arr2dinterpfld_;
    uiTableImpDataSel* 	dataselfld_;
    uiColorInput*      	colbut_;
    uiStratLevelSel*   	stratlvlfld_;
    uiIOObjSel*		outputfld_;
    uiCheckBox*        	displayfld_;

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
