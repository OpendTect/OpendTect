#ifndef uigrubfilterattrib_h
#define uigrubfilterattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          March 2011
 RCS:           $Id: uigrubbsfilterattrib.h,v 1.1 2011-03-17 05:23:29 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

class uiAttrSel;
class uiGenInput;
class uiStepOutSel;
class uiSteeringSel;


/*! \brief Semblance Attribute description editor */

mClass uiGrubbFilterAttrib : public uiAttrDescEd
{
public:

			uiGrubbFilterAttrib(uiParent*,bool);

    void		getEvalParams(TypeSet<EvalParam>&) const;

protected:

    uiAttrSel*		inpfld_;
    uiGenInput*		grubbvalfld_;
    uiGenInput*		gatefld_;
    uiGenInput*		replacetype_;
    uiStepOutSel*	stepoutfld_;

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);

    			mDeclReqAttribUIFns
};


#endif
