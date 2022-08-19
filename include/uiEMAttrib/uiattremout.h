#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiemattribmod.h"
#include "attribdescid.h"
#include "uibatchprocdlg.h"
#include "multiid.h"

class NLAModel;
class uiAttrSel;
class uiBatchJobDispatcherSel;

namespace Attrib { class DescSet; }

/*! \brief
Brief Base class Earth Model Output Batch dialog.
Used for calculating attributes in relation with surfaces
*/


mExpClass(uiEMAttrib) uiAttrEMOut : public uiBatchProcDlg
{ mODTextTranslationClass(uiAttrEMOut)
public:
			uiAttrEMOut(uiParent*,const Attrib::DescSet&,
				    const NLAModel*,const MultiID&,const char*);
			~uiAttrEMOut();

    void		updateAttributes(const Attrib::DescSet& descset,
					 const NLAModel* nlamodel,
					 const MultiID& nlaid);
    void		getDescNames(BufferStringSet&) const;

protected:

    virtual void	attribSel(CallBacker*)		=0;
    bool		prepareProcessing() override;
    bool		fillPar(IOPar&) override;
    bool		addNLA(Attrib::DescID&);
    void		fillOutPar(IOPar&,const char* outtyp,
				   const char* idlbl,const MultiID& outid);
    Attrib::DescSet*	getTargetDescSet(TypeSet<Attrib::DescID>&,
					 const char* outputnm);

    Attrib::DescSet*	ads_;
    MultiID		nlaid_;
    Attrib::DescID	nladescid_;
    const NLAModel*	nlamodel_;
    TypeSet<Attrib::DescID> outdescids_;
    BufferStringSet outdescnms_;

    uiAttrSel*		attrfld_;
    uiBatchJobDispatcherSel* batchfld_;
};
