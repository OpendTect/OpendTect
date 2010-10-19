#ifndef uiwelltiemgrdlg_h
#define uiwelltiemgrdlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Jan 2009
 RCS:           $Id: uiwelltiemgrdlg.h,v 1.12 2010-10-19 12:57:31 cvsbruno Exp $
________________________________________________________________________

-*/


#include "uidialog.h"

class MultiID;
class CtxtIOObj;

class uiIOObjSel;
class uiComboBox;
class uiCheckBox;
class uiGenInput;
class uiSeisSel;
class uiSeis2DLineNameSel;
class uiWaveletExtraction;

namespace WellTie
{
    class Setup;
    class uiTieWin;

mClass uiTieWinMGRDlg : public uiDialog
{

public:    
			uiTieWinMGRDlg(uiParent*, WellTie::Setup&);
			~uiTieWinMGRDlg();

    void		delWins(); 

protected:

    WellTie::Setup& 	wtsetup_;
    CtxtIOObj&          wllctio_;
    CtxtIOObj&          wvltctio_;
    CtxtIOObj&          seisctio2d_;
    CtxtIOObj&          seisctio3d_;
    bool		savedefaut_;
    bool		is2d_;
    ObjectSet<WellTie::uiTieWin> welltiedlgset_;
    ObjectSet<WellTie::uiTieWin> welltiedlgsetcpy_;

    uiIOObjSel*         wellfld_;
    uiIOObjSel*         wvltfld_;
    uiGenInput*		typefld_;
    uiGenInput*		seisextractfld_;
    uiSeisSel* 		seis2dfld_;
    uiSeisSel* 		seis3dfld_;
    uiSeis2DLineNameSel* seislinefld_;
    uiComboBox*		vellogfld_;
    uiComboBox*		denlogfld_;
    uiCheckBox*		isvelbox_;
    uiCheckBox*		used2tmbox_;
    uiWaveletExtraction* extractwvltdlg_;

    bool		getDefaults();
    void		saveWellTieSetup(const MultiID&,const WellTie::Setup&);
    
    bool		acceptOK(CallBacker*);
    void		extrWvlt(CallBacker*);
    void                extractWvltDone(CallBacker*);
    void		isSonicSel(CallBacker*);
    void		selChg( CallBacker* );
    void		wellSel(CallBacker*);
    void 		wellTieDlgClosed(CallBacker*);
};

}; //namespace WellTie
#endif
