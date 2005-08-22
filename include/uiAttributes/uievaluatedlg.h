#ifndef uievaluatedlg_h
#define uievaluatedlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          March 2003
 RCS:           $Id: uievaluatedlg.h,v 1.1 2005-08-22 15:31:53 cvsnanne Exp $
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


class AttribParamGroup : public uiGroup
{
public:
                        AttribParamGroup(uiParent*,const Attrib::Desc*,
					 const EvalParam&);
    void                updatePars(Attrib::Desc&,int);
    void                updateDesc(Attrib::Desc&,int);
    const char*         getLabel()                      { return evallbl_; }

protected:

    void		createInputSpecs(const Attrib::ValParam*,
	    				 DataInpSpec*&,DataInpSpec*&);

    uiGenInput*         initfld;
    uiGenInput*         incrfld;
    BufferString	parlbl_;
    BufferString        evallbl_;

    BufferString	parstr1_;
    BufferString	parstr2_;
};


class uiEvaluateDlg : public uiDialog
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
    static const char*		evaluserref;

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
