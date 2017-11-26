#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        P.F.M. de Groot
 Date:          September 2012
________________________________________________________________________

-*/

#include "uiattrdesced.h"

namespace Attrib { class Desc; }
class uiAttrSel;
class uiGenInput;
class uiSteeringSel;
class uiStepOutSel;
class SeisTrcBuf;
class TrcKeyZSampling;

mClass(uiTextureAttrib) uiTextureAttrib : public uiAttrDescEd
{ mODTextTranslationClass(uiTextureAttrib);
public:
                        uiTextureAttrib(uiParent*,bool);
    void		getEvalParams(TypeSet<EvalParam>&) const;

protected:

    uiAttrSel*		inpfld_;
    uiGenInput*		gatefld_;
    uiSteeringSel*	steerfld_;
    uiStepOutSel*	stepoutfld_;
    uiGenInput*		actionfld_;
    uiGenInput*		glcmsizefld_;
    uiGenInput*		globalminfld_;
    uiGenInput*		globalmaxfld_;

    void		analyseCB(CallBacker*);
    bool		readInpAttrib(SeisTrcBuf&,const TrcKeyZSampling&,
				      int) const;
    void		calcAndSetMinMaxVal(const SeisTrcBuf&);

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);
    bool		setOutput(const Attrib::Desc&);
    bool		getParameters(Attrib::Desc&);
    uiRetVal		getInput(Attrib::Desc&);
    bool		getOutput(Attrib::Desc&);
				mDeclReqAttribUIFns
};
