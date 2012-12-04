#ifndef uitextureattrib_h
#define uitextureattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        P.F.M. de Groot
 Date:          September 2012
 RCS:           $Id: uitextureattrib.h 27530 2012-11-19 09:49:13Z kristofer.tingdahl@dgbes.com $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

namespace Attrib { class Desc; }
class uiAttrSel;
class uiGenInput;
class uiLabel;
class uiSteeringSel;
class uiStepOutSel;
class uiPushButton;
class SeisTrcBuf;
class CubeSampling;
class LineKey;


class uiTextureAttrib : public uiAttrDescEd
{
public:

    uiTextureAttrib(uiParent*,bool);
    void		getEvalParams(TypeSet<EvalParam>&) const;

protected:

    uiAttrSel*		inpfld_;
    uiGenInput*		actionfld_;
    uiSteeringSel*	steerfld_;
    uiStepOutSel*	stepoutfld_;
    uiGenInput*		gatefld_;
    uiGenInput*		glcmsizefld_;
    uiGenInput*		globalminfld_;
    uiGenInput*		globalmaxfld_;
    uiPushButton*	analysebut_;

    void		actionSel(CallBacker*);
    void		steerTypeSel(CallBacker*);
    void		scalingSel(CallBacker*);
    void		analyseCB(CallBacker*); 
    void		readSampAttrib(CubeSampling&,int,LineKey&);
    void		setMinMaxVal(const SeisTrcBuf&);
    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);
    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
				mDeclReqAttribUIFns
};

#endif
