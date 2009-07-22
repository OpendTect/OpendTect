#ifndef uiwelltiemgrdlg_h
#define uiwelltiemgrdlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Jan 2009
 RCS:           $Id: uiwelltiemgrdlg.h,v 1.4 2009-07-22 16:01:24 cvsbert Exp $
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
