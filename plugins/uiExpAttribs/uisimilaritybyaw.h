#ifndef uiSimilaritybyaw_h
#define uiSimilaritybyaw_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Haibin Di
 Date:          August 2013
 RCS:           $Id$ 
 ________________________________________________________________________
-*/

#include "uiexpattribsmod.h"
#include "uiattrdesced.h"


namespace Attrib { class Desc; }

class uiAttrSel;
class uiGenInput;
class uiSteeringSel;
class uiStepOutSel;


mExpClass(uiExpAttribs) uiSimilaritybyAW : public uiAttrDescEd
{

public:
    			uiSimilaritybyAW(uiParent*,bool);

protected:

			mDeclReqAttribUIFns

    void		choiceSel(CallBacker*);
    void		steerTypeSel(CallBacker*);
 
    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);
    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);

    uiAttrSel*		inputfld_;
    uiSteeringSel*	steerfld_;
    uiGenInput*		reftimegatefld_;
    uiGenInput*		searchrangefld_;
    uiStepOutSel*	stepoutfld_;
    uiGenInput*		attributefld_;
};

#endif
