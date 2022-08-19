#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattrdesced.h"
#include "uisteeringsel.h"

namespace Attrib { class Desc; }
class uiAttrSel;
class uiGenInput;
class uiStepOutSel;
class uiSteeringSel;
class uiLabel;
class uiPushButton;
class SeisTrcBuf;
class TrcKeyZSampling;
class LineKey;


class uiGLCM_attrib : public uiAttrDescEd
{ mODTextTranslationClass(uiGLCM_attrib);
public:

			uiGLCM_attrib( uiParent*, bool );
    void		getEvalParams(TypeSet<EvalParam>&) const override;

protected:

    uiAttrSel*		inpfld_;
    uiGenInput* 	attributefld_;
    uiGenInput* 	numbergreyfld_;
    uiGenInput* 	limitfld_;
    uiGenInput* 	directfld_;
    uiStepOutSel*	stepoutfld_;
    uiGenInput* 	samprangefld_;
    uiGenInput* 	outputfld_;
    uiSteeringSel*	steerfld_;

    void		GLCMattributeSel(CallBacker*);
    void		GLCMdirectionSel(CallBacker*);

    void		analyseData(CallBacker*);
    bool		readInputCube(SeisTrcBuf&, const TrcKeyZSampling&,
				      int, const LineKey&) const;
    void		determineMinMax( const SeisTrcBuf&);

    bool		setParameters( const Attrib::Desc& ) override;
    bool		setInput( const Attrib::Desc& ) override;

    bool		getParameters( Attrib::Desc&) override;
    bool		getInput( Attrib::Desc&) override;

    void		steerTypeSel( CallBacker* );

			mDeclReqAttribUIFns
};
