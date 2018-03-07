#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2001
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uibatchprocdlg.h"
#include "attribdescid.h"
#include "dbkey.h"
#include "bufstringset.h"

class IOObj;
class NLAModel;

class uiAttrSel;
class uiGenInput;
class uiIOObjSel;
class uiMultiAttribSel;
class uiSeisSel;
class uiSeisTransfer;
class uiBatchJobDispatcherSel;


/*! \brief Dialog for creating volume output */

mExpClass(uiAttributes) uiAttrVolOut : public uiBatchProcDlg
{ mODTextTranslationClass(uiAttrVolOut);
public:
			uiAttrVolOut(uiParent*,const Attrib::DescSet&,
				     bool multioutput,
				     const NLAModel*,const DBKey& nlaid);
			~uiAttrVolOut();
    void		setInput(const Attrib::DescID&);

    const IOPar&	subSelPar() const		{ return subselpar_; }

    static const char*  sKeyMaxCrlRg();
    static const char*  sKeyMaxInlRg();
    void		updateAttributes(const Attrib::DescSet& descst,
					 const NLAModel* nlamodel,
					 const DBKey& nlaid);

    mExpClass(uiAttributes) Selection
    {
    public:
			Selection()
			    : outputnr_(-1)	{}
	Attrib::DescID	attrid_;
	DBKey		dbky_;
	int		outputnr_;
    };

    const Selection&	getSelection() const	{ return sel_; }

protected:

    Selection		sel_;
    IOPar&		subselpar_;
    Attrib::DescSet*	ads_;
    DBKey		nlaid_;
    const NLAModel*	nlamodel_;

    uiAttrSel*		todofld_;
    uiMultiAttribSel*	attrselfld_;
    uiSeisTransfer*	transffld_;
    uiSeisSel*		objfld_;
    uiIOObjSel*		datastorefld_;
    uiGenInput*		offsetfld_;

    TypeSet<int>	seloutputs_;
    BufferStringSet	seloutnms_;

    void		getJobName(BufferString& jobnm) const;
    bool		prepareProcessing();
    bool		fillPar(IOPar&);
    Attrib::DescSet*	getFromToDoFld(TypeSet<Attrib::DescID>&,int&);
    void		attrSel(CallBacker*);
    void		psSelCB(CallBacker*);
    void		outSelCB(CallBacker*);
    void		addNLA(Attrib::DescID&);

};
