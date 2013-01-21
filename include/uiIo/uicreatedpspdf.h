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

#include "uiiomod.h"
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
/*! \brief Dialog for Horizon Import */

mExpClass(uiIo) uiCreateDPSPDF : public uiDialog
{
public:
			uiCreateDPSPDF(uiParent*,
				       const uiDataPointSetCrossPlotter*);
			uiCreateDPSPDF(uiParent*,const DataPointSet&,
				       bool restricted=false);
			~uiCreateDPSPDF();
    const ProbDenFunc*	probDensFunc() const			{ return pdf_; }
    void		setPrefDefNames(const BufferStringSet&);

protected:

    ObjectSet<uiPrDenFunVarSel>	probflds_;
    ObjectSet<uiButton>		addbuts_;
    ObjectSet<uiButton>		rmbuts_;
    ProbDenFunc*		pdf_;
    int				nrdisp_;
    bool			restrictedmode_;

    uiIOObjSel*			outputfld_;
    uiComboBox*			createfrmfld_;
    uiComboBox*			createoffld_;
    uiGenInput*			nrbinfld_;
    const uiDataPointSetCrossPlotter* plotter_;
    const DataPointSet&		dps_;

    void			createDefaultUI();
    bool 			createPDF();
    void 			viewPDF();
    
    float			getVal(int rid,int cid) const;
    void 			fillPDF(ArrayNDProbDenFunc&);
    void			setColRange(CallBacker*);
    void			butPush(CallBacker*);
    void			handleDisp(CallBacker*);
    bool			acceptOK(CallBacker*);
};


#endif

