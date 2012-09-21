#ifndef uifreqfilterattrib_h
#define uifreqfilterattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          July 2001
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uiattrdesced.h"
#include "uiwindowfunctionsel.h"

namespace Attrib { class Desc; };

class uiFreqFilterSelFreq;
class uiImagAttrSel;
class uiGenInput;
class uiCheckBox;
class uiLabeledSpinBox;

/*! \brief ** Attribute description editor */

mClass(uiAttributes) uiFreqFilterAttrib : public uiAttrDescEd
{
public:

			uiFreqFilterAttrib(uiParent*,bool);

    void		getEvalParams(TypeSet<EvalParam>&) const;

protected:

    uiImagAttrSel*      inpfld;
    uiGenInput*         isfftfld;
    uiFreqFilterSelFreq* freqfld;
    uiLabeledSpinBox*	polesfld;
    uiCheckBox*		freqwinselfld;
    ObjectSet<uiWindowFunctionSel> winflds;
    uiWindowFunctionSel::Setup* viewsetup_;

    void		finaliseCB(CallBacker*);
    void		freqChanged(CallBacker*);
    void		freqWinSel(CallBacker*);
    void		updateTaperFreqs(CallBacker*);
    void		typeSel(CallBacker*);
    void		isfftSel(CallBacker*);

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);

    virtual bool        areUIParsOK();

    			mDeclReqAttribUIFns
};

#endif

