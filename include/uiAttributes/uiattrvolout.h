#ifndef uiattrvolout_h
#define uiattrvolout_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uidialog.h"
#include "multiid.h"
#include "bufstringset.h"

class IOObj;
class NLAModel;
namespace Attrib	{ class CurrentSel; class DescID; class DescSet; };
namespace Batch		{ class JobSpec; }

class uiAttrSel;
class uiGenInput;
class uiIOObjSel;
class uiMultiAttribSel;
class uiSeisSel;
class uiSeisTransfer;
class uiBatchJobDispatcherSel;


/*! \brief Dialog for creating volume output */

mExpClass(uiAttributes) uiAttrVolOut : public uiDialog
{ mODTextTranslationClass(uiAttrVolOut);
public:
			uiAttrVolOut(uiParent*,const Attrib::DescSet&,
				     bool multioutput,
				     const NLAModel*,const MultiID& nlaid);
			~uiAttrVolOut();

    const IOPar&	subSelPar() const		{ return subselpar_; }
    const Attrib::CurrentSel& outputSelection() const	{ return sel_; }

    static const char*  sKeyMaxCrlRg();
    static const char*  sKeyMaxInlRg();

protected:

    Attrib::CurrentSel&	sel_;
    IOPar&		subselpar_;
    Attrib::DescSet&	ads_;
    MultiID		nlaid_;
    const NLAModel*	nlamodel_;

    uiAttrSel*		todofld_;
    uiMultiAttribSel*	attrselfld_;
    uiSeisTransfer*	transffld_;
    uiSeisSel*		objfld_;
    uiIOObjSel*		datastorefld_;
    uiGenInput*		offsetfld_;
    uiBatchJobDispatcherSel* batchfld_;

    TypeSet<int>	seloutputs_;
    BufferStringSet	seloutnms_;


    Batch::JobSpec&	jobSpec();
    bool		prepareProcessing();
    bool		fillPar();
    Attrib::DescSet*	getFromToDoFld(TypeSet<Attrib::DescID>&,int&);
    bool		acceptOK(CallBacker*);
    void		attrSel(CallBacker*);
    void		psSelCB(CallBacker*);
    void		outSelCB(CallBacker*);
    void		addNLA(Attrib::DescID&);

};

#endif

