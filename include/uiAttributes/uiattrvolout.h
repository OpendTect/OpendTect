#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uibatchprocdlg.h"
#include "multiid.h"
#include "bufstringset.h"

class IOObj;
class NLAModel;
namespace Attrib	{ class CurrentSel; class DescID; class DescSet; }
namespace Batch		{ class JobSpec; }

class uiAttrSel;
class uiGenInput;
class uiIOObjSel;
class uiMultiAttribSel;
class uiSeisSel;
class uiSeisTransfer;
class uiBatchJobDispatcherSel;


/*! \brief Dialog for creating volume output */

mExpClass(uiAttributes) uiAttrVolOut : public uiBatchProcDlg
{ mODTextTranslationClass(uiAttrVolOut)
public:
			uiAttrVolOut(uiParent*,const Attrib::DescSet&,
				     bool multioutput,
				     const NLAModel*,const MultiID& nlaid);
			~uiAttrVolOut();
    void		setInput(const Attrib::DescID&);

    const IOPar&	subSelPar() const		{ return subselpar_; }
    const Attrib::CurrentSel& outputSelection() const	{ return sel_; }

    static const char*  sKeyMaxCrlRg();
    static const char*  sKeyMaxInlRg();
    Notifier<uiAttrVolOut>	needSaveNLA;
    void		updateAttributes(const Attrib::DescSet& descst,
					 const NLAModel* nlamodel,
					 const MultiID& nlaid);

protected:

    Attrib::CurrentSel&	sel_;
    IOPar&		subselpar_;
    Attrib::DescSet*	ads_;
    MultiID		nlaid_;
    const NLAModel*	nlamodel_;

    uiAttrSel*		todofld_;
    uiMultiAttribSel*	attrselfld_;
    uiSeisTransfer*	transffld_;
    uiSeisSel*		objfld_;
    uiIOObjSel*		datastorefld_;
    uiGenInput*		offsetfld_;

    TypeSet<int>	seloutputs_;
    BufferStringSet	seloutnms_;


    void		getJobName(BufferString& jobnm) const override;
    bool		prepareProcessing() override;
    bool		fillPar(IOPar&) override;
    Attrib::DescSet*	getFromToDoFld(TypeSet<Attrib::DescID>&,int&);
    void		attrSel(CallBacker*);
    void		psSelCB(CallBacker*);
    void		outSelCB(CallBacker*);
    void		addNLA(Attrib::DescID&);

};
