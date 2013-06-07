#ifndef uicreatedpspdf_h
#define uicreatedpspdf_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          Jan 2010
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uidialog.h"

class ArrayNDProbDenFunc;
class BufferStringSet;
class DataPointSet;
class ProbDenFunc;

class uiComboBox;
class uiGenInput;
class uiIOObjSel;
class uiDataPointSetCrossPlotter;
class uiPrDenFunVarSel;

mClass uiCreateDPSPDF : public uiDialog
{
public:
			uiCreateDPSPDF(uiParent*,
				       uiDataPointSetCrossPlotter&,
				       const BufferStringSet&);
			// Implementation removed, Do not use TODO remove
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
    const uiDataPointSetCrossPlotter* plotter_;

    void 			fillPDF(ArrayNDProbDenFunc&);
    void			setColRange(CallBacker*);
    void			butPush(CallBacker*);
    void			handleDisp(CallBacker*);
    bool			acceptOK(CallBacker*);

    ProbDenFunc*		pdf_;
    bool			restrictedmode_;
    const DataPointSet&		dps_;

    void			createDefaultUI();
    bool			createPDF();
    void			viewPDF();
    float 			getVal(int rid,int cid) const;

public:
				uiCreateDPSPDF(uiParent*,
					const uiDataPointSetCrossPlotter*);
    				uiCreateDPSPDF(uiParent*,const DataPointSet&,
					bool restricted=false);
    const ProbDenFunc*		probDensFunc() const	{ return pdf_; }
    void			setPrefDefNames(const BufferStringSet&);
};


#endif
