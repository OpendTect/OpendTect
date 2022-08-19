#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellattribmod.h"
#include "uidialog.h"
#include "binidvalset.h"

class NLAModel;
class uiAttrSel;
class uiGenInput;

namespace Attrib { class DescSet; }
namespace Well { class Data; };

/*! \brief Dialog for marker specifications */

mExpClass(uiWellAttrib) uiWellAttribSel : public uiDialog
{
public:
				uiWellAttribSel(uiParent*,Well::Data&,
						const Attrib::DescSet&,
						const NLAModel* mdl=0);

    int				selectedLogIdx() const	{ return sellogidx_; }

protected:

    RefMan<Well::Data>		wd_;
    const Attrib::DescSet&	attrset_;
    const NLAModel*		nlamodel_;

    uiAttrSel*			attribfld;
    uiGenInput*			rangefld;
    uiGenInput*			lognmfld;

    void			setDefaultRange();
    void			selDone(CallBacker*);
    bool			acceptOK(CallBacker*) override;

    bool			inputsOK();
    void			getPositions(BinIDValueSet&,
	    				     TypeSet<BinIDValueSet::Pos>&,
					     TypeSet<float>& depths);
    bool			extractData(BinIDValueSet&);
    bool			createLog(const BinIDValueSet&,
	    				  const TypeSet<BinIDValueSet::Pos>&,
					  const TypeSet<float>& depths);

    int				sellogidx_;
};
