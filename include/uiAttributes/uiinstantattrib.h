#ifndef uiinstantattrib_h
#define uiinstantattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          July 2001
 RCS:           $Id: uiinstantattrib.h,v 1.9 2012/05/25 12:07:55 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

namespace Attrib { class Desc; };

class uiImagAttrSel;
class uiGenInput;
class uiLabeledSpinBox;

/*! \brief Instantaneous Attribute description editor */

mClass uiInstantaneousAttrib : public uiAttrDescEd
{
public:

			uiInstantaneousAttrib(uiParent*,bool);

protected:

    uiImagAttrSel*	inpfld;
    uiGenInput*		outpfld;
    uiLabeledSpinBox*	phaserotfld;

    static const char*	outstrs[];

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);
    bool		setOutput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
    bool		getOutput(Attrib::Desc&);

    void		outputSelCB(CallBacker*);

    			mDeclReqAttribUIFns

    void		getEvalParams( TypeSet<EvalParam>& params ) const;
};

#endif
