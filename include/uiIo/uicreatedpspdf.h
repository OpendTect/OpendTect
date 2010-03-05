#ifndef uicreatedpspdf_h
#define uicreatedpspdf_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          Jan 2010
 RCS:           $Id: uicreatedpspdf.h,v 1.3 2010-03-05 13:57:38 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiButton;
class uiComboBox;
class uiGenInput;
class uiIOObjSel;
class uiDataPointSetCrossPlotter;
class uiPrDenFunVarSel;
class BufferStringSet;
class ArrayNDProbDenFunc;
/*! \brief Dialog for Horizon Import */

mClass uiCreateDPSPDF : public uiDialog
{
public:
			uiCreateDPSPDF(uiParent*,uiDataPointSetCrossPlotter&,
				       const BufferStringSet&);
			~uiCreateDPSPDF();

protected:

    ObjectSet<uiPrDenFunVarSel>	probflds_;
    ObjectSet<uiButton>		addbuts_;
    ObjectSet<uiButton>		rmbuts_;
    int				nrdisp_;

    uiIOObjSel*			outputfld_;
    uiComboBox*			createfrmfld_;
    uiComboBox*			createoffld_;
    uiGenInput*			nrbinfld_;
    uiDataPointSetCrossPlotter&	plotter_;

    void 			fillPDF(ArrayNDProbDenFunc&);
    void			setColRange(CallBacker*);
    void			butPush(CallBacker*);
    void			handleDisp(CallBacker*);
    bool			acceptOK(CallBacker*);
};


#endif
