#ifndef uiwelltiemgrdlg_h
#define uiwelltiemgrdlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Jan 2009
 RCS:           $Id: uiwelltiemgrdlg.h,v 1.5 2009-09-03 09:41:39 cvsbruno Exp $
________________________________________________________________________

-*/


#include "uidialog.h"

class MultiID;
class CtxtIOObj;
namespace Attrib { class DescSet; }

class uiAttrSel;
class uiIOObjSel;
class uiComboBox;
class uiCheckBox;
class uiGenInput;

namespace WellTie
{
    class Setup;
    class uiTieWin;

mClass uiTieWinMGRDlg : public uiDialog
{

public:    
			uiTieWinMGRDlg(uiParent*, WellTie::Setup&,
					const Attrib::DescSet&);
			~uiTieWinMGRDlg();


protected:

    WellTie::Setup& 	wtsetup_;
    CtxtIOObj&          wllctio_;
    CtxtIOObj&          wvltctio_;
    bool		savedefaut_;
    const Attrib::DescSet& attrset_;
    ObjectSet<WellTie::uiTieWin> welltiedlgset_;
    ObjectSet<WellTie::uiTieWin> welltiedlgsetcpy_;

    uiIOObjSel*         wellfld_;
    uiIOObjSel*         wvltfld_;
    uiAttrSel*          attrfld_;
    uiComboBox*		vellogfld_;
    uiComboBox*		denlogfld_;
    uiCheckBox*		isvelbox_;

    bool		getDefaults();
    void		saveWellTieSetup(const MultiID&,const WellTie::Setup&);

    bool		acceptOK(CallBacker*);
    void		extrWvlt(CallBacker*);
    void		isSonicSel(CallBacker*);
    void		wellSel(CallBacker*);
    void 		wellTieDlgClosed(CallBacker*);
};

}; //namespace WellTie
#endif
