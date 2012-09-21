#ifndef uiimphorizon_h
#define uiimphorizon_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2002
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"

class Array2DInterpol;
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
class uiCompoundParSel;
class uiIOObjSel;
class uiPushButton;
class uiPosSubSel;
class uiScaler;
class uiStratLevelSel;
class uiTableImpDataSel;
namespace Table { class FormatDesc; }
namespace EM { class Horizon3D; }

/*! \brief Dialog for Horizon Import */

mClass(uiEarthModel) uiImportHorizon : public uiDialog
{
public:
			uiImportHorizon(uiParent*,bool);
			~uiImportHorizon();

    bool                doDisplay() const;
    MultiID             getSelID() const;

    Notifier<uiImportHorizon>	importReady;

protected:

    uiFileInput*	inpfld_;
    uiPushButton*      	scanbut_;
    uiLabeledListBox*	attrlistfld_;
    uiPushButton*	addbut_;
    uiPosSubSel*	subselfld_;
    uiGenInput*		filludffld_;
    uiPushButton*	interpolparbut_;
    Array2DInterpol*	interpol_;
    uiTableImpDataSel* 	dataselfld_;
    uiColorInput*      	colbut_;
    uiStratLevelSel*   	stratlvlfld_;
    uiIOObjSel*		outputfld_;
    uiCheckBox*        	displayfld_;

    virtual bool	acceptOK(CallBacker*);
    void                descChg(CallBacker*);
    void		inputChgd(CallBacker*);
    void		addAttrib(CallBacker*);
    void		scanPush(CallBacker*);
    void                fillUdfSel(CallBacker*);
    void                stratLvlChg(CallBacker*);
    void		interpolSettingsCB(CallBacker*);

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

