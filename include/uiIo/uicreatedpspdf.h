#ifndef uicreatedpspdf_h
#define uicreatedpspdf_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          Jan 2010
 RCS:           $Id: uicreatedpspdf.h,v 1.1 2010-02-16 06:13:26 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiComboBox;
class uiGenInput;
class uiIOObjSel;
class uiDataPointSetCrossPlotter;
class SampledProbDenFunc2D;
/*! \brief Dialog for Horizon Import */

mClass uiCreateDPSPDF : public uiDialog
{
public:
			uiCreateDPSPDF(uiParent*,uiDataPointSetCrossPlotter&);
			~uiCreateDPSPDF();

protected:

    uiIOObjSel*			outputfld_;
    uiComboBox*			createfrmfld_;
    uiComboBox*			createoffld_;
    uiGenInput*			nrbinfld_;
    uiDataPointSetCrossPlotter&	plotter_;

    void 			fillPDF(SampledProbDenFunc2D&);
    bool			acceptOK(CallBacker*);
};


#endif
