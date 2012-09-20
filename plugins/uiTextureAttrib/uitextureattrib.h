#ifndef uitextureattrib_h
#define uitextureattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        P.F.M. de Groot
 Date:          September 2012
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiattrdesced.h"

namespace Attrib { class Desc; }
class uiAttrSel;
class uiGenInput;
class uiLabel;
class uiSteeringSel;
class uiStepOutSel;


class uiTextureAttrib : public uiAttrDescEd
{
public:

			uiTextureAttrib(uiParent*,bool);

	void		getEvalParams(TypeSet<EvalParam>&) const;

protected:

    uiAttrSel*		inpfld_;
    uiGenInput*		actionfld_;
    uiLabel*		label_;
    uiSteeringSel*  	steerfld_;
    uiStepOutSel*   	stepoutfld_;
    uiGenInput*		gatefld_;
    uiGenInput*		glcmsizefld_;
    uiGenInput*		scalingtypefld_;
    uiGenInput*		globalmeanfld_;
    uiGenInput*		globalstdevfld_;

    void		actionSel(CallBacker*);
    void		steerTypeSel(CallBacker*);
    void		scalingSel(CallBacker*);
    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);
    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
    			mDeclReqAttribUIFns

};

#endif
