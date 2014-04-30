
#ifndef uiGLCM_attrib_h
#define uiGLCM_attrib_h
/*+
 * (C) JOANNEUM RESEARCH; http://www.joanneum.at
 * AUTHOR   : Christoph Eichkitz
 * DATE     : November 2013
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
class CubeSampling;
class LineKey;


class uiGLCM_attrib : public uiAttrDescEd
{ mODTextTranslationClass(uiGLCM_attrib);
public:

			uiGLCM_attrib( uiParent*, bool );
    void		getEvalParams(TypeSet<EvalParam>&) const;

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
    bool		readInputCube(SeisTrcBuf&, const CubeSampling&,
				      int, const LineKey&) const;
    void		determineMinMax( const SeisTrcBuf&);

    bool		setParameters( const Attrib::Desc& );
    bool		setInput( const Attrib::Desc& );

    bool		getParameters( Attrib::Desc&);
    bool		getInput( Attrib::Desc&);

    void		steerTypeSel( CallBacker* );

			mDeclReqAttribUIFns
};

#endif

