#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"

#include "datapointset.h"

class ArrayNDProbDenFunc;
class BufferStringSet;
class ProbDenFunc;

class uiComboBox;
class uiDataPointSetCrossPlotter;
class uiGenInput;
class uiIOObjSel;
class uiPrDenFunVarSel;

/*!
\brief Dialog for creating Probability Density Function of DataPointSet.
*/

mExpClass(uiIo) uiCreateDPSPDF : public uiDialog
{ mODTextTranslationClass(uiCreateDPSPDF);
public:
			uiCreateDPSPDF(uiParent*,
				       const uiDataPointSetCrossPlotter*,
				       bool requireunits);
			uiCreateDPSPDF(uiParent*,const DataPointSet&,
				       bool requireunits,
				       bool restricted=false);
			~uiCreateDPSPDF();

    const ProbDenFunc*	probDensFunc() const			{ return pdf_; }
    void		setPrefDefNames(const BufferStringSet&);
    void		setPrefNrDimensions(int);

protected:

    ObjectSet<uiPrDenFunVarSel>	probflds_;
    ObjectSet<uiButton>		addbuts_;
    ObjectSet<uiButton>		rmbuts_;
    ProbDenFunc*		pdf_ = nullptr;
    int				nrdisp_ = 1;
    bool			restrictedmode_;
    bool			requireunits_ = false;
    int				prefnrdims_ = 4;

    uiIOObjSel*			outputfld_;
    uiComboBox*			createfrmfld_ = nullptr;
    uiComboBox*			createoffld_;
    uiGenInput*			nrbinfld_;
    const uiDataPointSetCrossPlotter* plotter_ = nullptr;
    ConstRefMan<DataPointSet>	dps_;

    void			createDefaultUI();
    bool			createPDF();
    bool			viewPDF();

    float			getVal(int rid,int cid) const;
    Interval<float>		getRange(DataPointSet::ColID) const;
    void			fillPDF(ArrayNDProbDenFunc&);
    void			setColRange(CallBacker*);
    void			initDlg(CallBacker*);
    void			butPush(CallBacker*);
    void			handleDisp(CallBacker*);
    bool			acceptOK(CallBacker*) override;
};
