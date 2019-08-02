#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          February 2004
________________________________________________________________________

-*/

#include "uiwellattribmod.h"
#include "uidialog.h"
#include "binnedvalueset.h"

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

    Well::Data&			wd_;
    const Attrib::DescSet&	attrset_;
    const NLAModel*		nlamodel_;

    uiAttrSel*			attribfld;
    uiGenInput*			rangefld;
    uiGenInput*			lognmfld;

    void			setDefaultRange();
    void			selDone(CallBacker*);
    virtual bool		acceptOK();

    bool			inputsOK();
    void			getPositions(BinnedValueSet&,
					     TypeSet<BinnedValueSet::Pos>&,
					     TypeSet<float>& depths);
    bool			extractData(BinnedValueSet&);
    bool			createLog(const BinnedValueSet&,
					  const TypeSet<BinnedValueSet::Pos>&,
					  const TypeSet<float>& depths);

    int				sellogidx_;
};
