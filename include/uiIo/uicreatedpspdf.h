#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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

/*!
\brief Dialog for creating Probability Density Function of DataPointSet.
*/

mExpClass(uiIo) uiCreateDPSPDF : public uiDialog
{ mODTextTranslationClass(uiCreateDPSPDF);
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
    ProbDenFunc*		pdf_ = nullptr;
    int				nrdisp_ = 1;
    bool			restrictedmode_;

    uiIOObjSel*			outputfld_;
    uiComboBox*			createfrmfld_ = nullptr;
    uiComboBox*			createoffld_;
    uiGenInput*			nrbinfld_;
    const uiDataPointSetCrossPlotter* plotter_ = nullptr;
    const DataPointSet&		dps_;

    void			createDefaultUI();
    bool			createPDF();
    void			viewPDF();

    float			getVal(int rid,int cid) const;
    void			fillPDF(ArrayNDProbDenFunc&);
    void			setColRange(CallBacker*);
    void			butPush(CallBacker*);
    void			handleDisp(CallBacker*);
    bool			acceptOK(CallBacker*) override;
};
