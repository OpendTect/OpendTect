#ifndef uivolstatsattrib_h
#define uivolstatsattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: uivolstatsattrib.h,v 1.7 2006-12-20 11:23:00 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

namespace Attrib { class Desc; };
class uiGenInput;
class uiAttrSel;
class uiLabeledSpinBox;
class uiSteeringSel;
class uiStepOutSel;


/*! \brief VolumeStatistics Attribute description editor */

class uiVolumeStatisticsAttrib : public uiAttrDescEd
{
public:

			uiVolumeStatisticsAttrib(uiParent*,bool);

    void		getEvalParams(TypeSet<EvalParam>& params) const;

protected:

    uiAttrSel*		inpfld;
    uiSteeringSel*	steerfld;
    uiGenInput*		gatefld;
    uiGenInput*		shapefld;
    uiStepOutSel*	stepoutfld;
    uiLabeledSpinBox*	nrtrcsfld;
    uiGenInput*		outpfld;

    void		stepoutChg(CallBacker*);

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);
    bool		setOutput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
    bool		getOutput(Attrib::Desc&);

    			mDeclReqAttribUIFns
};


#endif
