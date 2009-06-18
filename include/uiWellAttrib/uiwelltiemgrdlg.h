#ifndef uiwelltiemgrdlg_h
#define uiwelltiemgrdlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bruno
 Date:          Jan 2009
 RCS:           $Id: uiwelltiemgrdlg.h,v 1.3 2009-06-18 07:41:52 cvsbruno Exp $
________________________________________________________________________

-*/


#include "uidialog.h"

class IOObj;
class MultiID;
class CtxtIOObj;
class WellTieSetup;
namespace Attrib { class DescSet; }

class uiAttrSel;
class uiIOObjSel;
class uiComboBox;
class uiCheckBox;
class uiWellTieToSeismicDlg;
class uiGenInput;


mClass uiWellTieMGRDlg : public uiDialog
{

public:    
			uiWellTieMGRDlg(uiParent*, WellTieSetup&,
					const Attrib::DescSet&);
			~uiWellTieMGRDlg();


protected:

    WellTieSetup& 	wtsetup_;
    CtxtIOObj&          wllctio_;
    CtxtIOObj&          wvltctio_;
    bool		savedefaut_;
    const Attrib::DescSet& attrset_;
    ObjectSet<uiWellTieToSeismicDlg> welltiedlgset_;
    ObjectSet<uiWellTieToSeismicDlg> welltiedlgsetcpy_;

    uiIOObjSel*         wellfld_;
    uiIOObjSel*         wvltfld_;
    uiAttrSel*          attrfld_;
    uiComboBox*		vellogfld_;
    uiComboBox*		denlogfld_;
    uiCheckBox*		isvelbox_;

    bool		getDefaults();
    void		saveWellTieSetup(const MultiID&,const WellTieSetup&);

    bool		acceptOK(CallBacker*);
    void		extrWvlt(CallBacker*);
    void		isSonicSel(CallBacker*);
    void		wellSel(CallBacker*);
    void 		wellTieDlgClosed(CallBacker*);
};

#endif
