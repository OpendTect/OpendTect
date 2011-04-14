#ifndef uivolstatsattrib_h
#define uivolstatsattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: uivolstatsattrib.h,v 1.11 2011-04-14 22:06:49 cvskris Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

namespace Attrib { class Desc; };
class uiGenInput;
class uiAttrSel;
class uiCheckBox;
class uiLabeledSpinBox;
class uiSteeringSel;
class uiStepOutSel;


/*! \brief VolumeStatistics Attribute description editor */

mClass uiVolumeStatisticsAttrib : public uiAttrDescEd
{
public:

			uiVolumeStatisticsAttrib(uiParent*,bool);

    void		getEvalParams(TypeSet<EvalParam>& params) const;

protected:

    uiAttrSel*		inpfld_;
    uiSteeringSel*	steerfld_;
    uiGenInput*		gatefld_;
    uiCheckBox*		edgeeffectfld_;
    uiGenInput*		shapefld_;
    uiStepOutSel*	stepoutfld_;
    uiLabeledSpinBox*	optstackstepfld_;
    uiLabeledSpinBox*	nrtrcsfld_;
    uiGenInput*		outpfld_;
    uiGenInput*		stackdirfld_;

    void		stepoutChg(CallBacker*);
    void		shapeChg(CallBacker*);
    void		stackstepChg(CallBacker*);

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);
    bool		setOutput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
    bool		getOutput(Attrib::Desc&);

    			mDeclReqAttribUIFns
};


#endif
