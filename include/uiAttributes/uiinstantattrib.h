#ifndef uiinstantattrib_h
#define uiinstantattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          July 2001
 RCS:           $Id: uiinstantattrib.h,v 1.10 2012-08-03 13:00:49 cvskris Exp $
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uiattrdesced.h"

namespace Attrib { class Desc; };

class uiImagAttrSel;
class uiGenInput;
class uiLabeledSpinBox;

/*! \brief Instantaneous Attribute description editor */

mClass(uiAttributes) uiInstantaneousAttrib : public uiAttrDescEd
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
    void		getEvalParams( TypeSet<EvalParam>& params ) const;

    			mDeclReqAttribUIFns
};

#endif

