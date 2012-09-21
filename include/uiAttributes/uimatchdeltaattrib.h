#ifndef uimatchdeltaattrib_h
#define uimatchdeltaattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Aug 2006
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiattrdesced.h"

class uiAttrSel;
class uiGenInput;


/*! \brief MatchDelta Attribute description editor */

class uiMatchDeltaAttrib : public uiAttrDescEd
{
public:

			uiMatchDeltaAttrib(uiParent*,bool);

    void		getEvalParams(TypeSet<EvalParam>&) const;

protected:

    uiAttrSel*		refcubefld_;
    uiAttrSel*		mtchcubefld_;
    uiGenInput*		maxshiftfld_;

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);

    			mDeclReqAttribUIFns
};


#endif
