#ifndef uicrossattrevaluatedlg_h
#define uicrossattrevaluatedlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Y. Liu
 Date:          March 2012
 RCS:           $Id: uicrossattrevaluatedlg.h,v 1.3 2012-03-21 16:25:29 cvsyuancheng Exp $
________________________________________________________________________

-*/


#include "uidialog.h"
#include "bufstringset.h"
#include "attribdescset.h"

class AttribParamGroup;
class IOPar;
class uiAttribDescSetEd;
class uiCheckBox;
class uiLabel;
class uiLabeledSpinBox;
class uiListBox;
class uiPushButton;
class uiSliderExtra;

mClass uiCrossAttrEvaluateDlg : public uiDialog
{
public:
				uiCrossAttrEvaluateDlg(uiParent*,
					uiAttribDescSetEd&,bool enabstore=true);
				~uiCrossAttrEvaluateDlg();

    Attrib::Desc*		getAttribDesc() const {return attrset_.desc(0);}
    void			getEvalSpecs(TypeSet<Attrib::SelSpec>&) const;
    Attrib::DescSet*		getEvalSet() const	{ return &attrset_; }
    bool			storeSlices() const;
    bool			evaluationPossible() const { return haspars_; }

    Notifier<uiCrossAttrEvaluateDlg>		calccb;
    CNotifier<uiCrossAttrEvaluateDlg,int>	showslicecb;

protected:

    uiListBox*			paramsfld_;
    uiListBox*			attrnmsfld_;

    uiPushButton*		calcbut;
    uiSliderExtra*		sliderfld;
    uiLabeledSpinBox*		nrstepsfld;
    uiLabel*			displaylbl;
    uiCheckBox*			storefld;

    void			parameterSel(CallBacker*);
    void			calcPush(CallBacker*);
    void			sliderMove(CallBacker*);
    void			doFinalise(CallBacker*);

    bool			acceptOK(CallBacker*);

    Attrib::DescSet&		attrset_;
    TypeSet<BufferStringSet>	userattnms_;//per parameter

    IOPar&			initpar_;
    BufferStringSet		lbls_;
    ObjectSet<AttribParamGroup>	grps_;

    TypeSet<Attrib::SelSpec>	specs_;
    bool			enabstore_;
    bool			haspars_;
};


#endif
