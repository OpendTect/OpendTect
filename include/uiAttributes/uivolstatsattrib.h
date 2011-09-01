#ifndef uivolstatsattrib_h
#define uivolstatsattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: uivolstatsattrib.h,v 1.12 2011-09-01 15:09:38 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

namespace Attrib { class Desc; };
class StringListInpSpec;
class uiGenInput;
class uiAttrSel;
class uiCheckBox;
class uiLabeledSpinBox;
class uiSteeringSel;
class uiStepOutSel;


/*! \brief VolumeStatistics Attribute description editor */


mClass uiVolumeStatisticsAttribBase : public uiAttrDescEd
{
public:

    void		getEvalParams(TypeSet<EvalParam>& params) const;

protected:
			uiVolumeStatisticsAttribBase(uiParent*,bool,
						const StringListInpSpec& shapes,
						const char* helpid);
    uiAttrSel*		inpfld_;
    uiSteeringSel*	steerfld_;
    uiGenInput*		gatefld_;
    uiGenInput*		shapefld_;
    uiStepOutSel*	stepoutfld_;
    uiLabeledSpinBox*	nrtrcsfld_;
    uiGenInput*		outpfld_;

    void		stepoutChg(CallBacker*);
    virtual void	shapeChg(CallBacker*) {};

    virtual bool	setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);
    bool		setOutput(const Attrib::Desc&);

    virtual bool	getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
    bool		getOutput(Attrib::Desc&);
};


mClass uiVolumeStatisticsAttrib : public uiVolumeStatisticsAttribBase
{
public:
			uiVolumeStatisticsAttrib(uiParent*,bool);
protected:
    uiLabeledSpinBox*	optstackstepfld_;
    uiCheckBox*		edgeeffectfld_;
    uiGenInput*		stackdirfld_;

    void		stackstepChg(CallBacker*);
    void		shapeChg(CallBacker*);

    bool		setParameters(const Attrib::Desc&);
    bool		getParameters(Attrib::Desc&);

    			mDeclReqAttribUIFns
};


#endif
