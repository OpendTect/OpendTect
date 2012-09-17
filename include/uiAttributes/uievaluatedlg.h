#ifndef uievaluatedlg_h
#define uievaluatedlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          March 2003
 RCS:           $Id: uievaluatedlg.h,v 1.5 2009/07/22 16:01:20 cvsbert Exp $
________________________________________________________________________

-*/


#include "uidialog.h"
#include "uigroup.h"
#include "bufstringset.h"
#include "attribdescid.h"

namespace Attrib
{
    class Desc;
    class DescSet;
    class SelSpec;
    class ValParam;
}

class uiAttrDescEd;
class uiCheckBox;
class DataInpSpec;
class EvalParam;
class uiGenInput;
class uiLabel;
class uiLabeledSpinBox;
class uiPushButton;
class uiSliderExtra;
class IOPar;


mClass AttribParamGroup : public uiGroup
{
public:
				AttribParamGroup(uiParent*,const uiAttrDescEd&,
						 const EvalParam&);
    void			updatePars(Attrib::Desc&,int);
    void			updateDesc(Attrib::Desc&,int);
    const char*			getLabel()		{ return evallbl_; }

protected:

    void			createInputSpecs(const Attrib::ValParam*,
						 DataInpSpec*&,DataInpSpec*&);

    uiGenInput*			initfld;
    uiGenInput*			incrfld;
    BufferString		parlbl_;
    BufferString		evallbl_;

    BufferString		parstr1_;
    BufferString		parstr2_;
    int				pgidx_;
    bool			evaloutput_;
    const uiAttrDescEd&		desced_;
};


mClass uiEvaluateDlg : public uiDialog
{
public:
				uiEvaluateDlg(uiParent*,uiAttrDescEd&,
					      bool enabstore=true);
				~uiEvaluateDlg();

    Attrib::Desc*		getAttribDesc() const	{ return seldesc_; }
    void			getEvalSpecs(TypeSet<Attrib::SelSpec>&) const;
    Attrib::DescSet*		getEvalSet() const	{ return attrset_; }
    bool			storeSlices() const;
    bool			evaluationPossible() const { return haspars_; }

    Notifier<uiEvaluateDlg>	calccb;
    CNotifier<uiEvaluateDlg,int> showslicecb;

protected:

    uiGenInput*			evalfld;
    uiPushButton*		calcbut;
    uiSliderExtra*		sliderfld;
    uiLabeledSpinBox*		nrstepsfld;
    uiLabel*			displaylbl;
    uiCheckBox*			storefld;

    void			variableSel(CallBacker*);
    void			calcPush(CallBacker*);
    void			sliderMove(CallBacker*);
    void			doFinalise(CallBacker*);

    bool			acceptOK(CallBacker*);

    Attrib::Desc*		seldesc_;
    Attrib::DescSet*		attrset_;
    uiAttrDescEd&		desced_;
    Attrib::DescID		srcid_;

    IOPar&			initpar_;
    BufferStringSet		lbls_;
    ObjectSet<AttribParamGroup>	grps_;

    TypeSet<Attrib::SelSpec>	specs_;
    bool			enabstore_;
    bool			haspars_;
};


#endif
