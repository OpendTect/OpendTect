#ifndef uicrossattrevaluatedlg_h
#define uicrossattrevaluatedlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Y. Liu
 Date:          March 2012
 RCS:           $Id: uicrossattrevaluatedlg.h,v 1.8 2012-08-03 13:00:48 cvskris Exp $
________________________________________________________________________

-*/


#include "uiattributesmod.h"
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

mClass(uiAttributes) uiCrossAttrEvaluateDlg : public uiDialog
{
public:
				uiCrossAttrEvaluateDlg(uiParent*,
					uiAttribDescSetEd&,bool enabstore=true);
				~uiCrossAttrEvaluateDlg();

    Attrib::Desc*		getAttribDesc() const   { return seldesc_; }
    void			getEvalSpecs(TypeSet<Attrib::SelSpec>&) const;
    Attrib::DescSet*		getEvalSet() const	{ return &attrset_; }
    bool			storeSlices() const;
    bool			evaluationPossible() const { return haspars_; }
    const TypeSet<Attrib::DescID>& evaluateChildIds() const 
    				{ return seldeschildids_; }
    BufferString		acceptedDefStr() const;

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
    void			getSelDescIDs(
	    				TypeSet<TypeSet<Attrib::DescID> >&, 
					TypeSet<TypeSet<int> >&);

    bool			acceptOK(CallBacker*);

    Attrib::Desc*		seldesc_; 
    Attrib::DescID		srcid_;
    Attrib::DescSet&		attrset_;
    TypeSet<BufferStringSet>	userattnms_;//per parameter

    IOPar&			initpar_;
    ObjectSet<AttribParamGroup>	grps_;

    TypeSet<Attrib::DescID>	srcspecids_;
    TypeSet<Attrib::DescID>	seldeschildids_;
    BufferStringSet		lbls_; //size is nr of steps
    TypeSet<Attrib::SelSpec> 	specs_;//size is nr of steps

    BufferStringSet		defstr_;    
    bool			enabstore_;
    bool			haspars_;
};


#endif

