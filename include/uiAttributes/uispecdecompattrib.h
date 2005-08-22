#ifndef uispecdecompattrib_h
#define uispecdecompattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          January 2003
 RCS:           $Id: uispecdecompattrib.h,v 1.2 2005-08-22 15:33:53 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

namespace Attrib { class Desc; };

class uiGenInput;
class uiImagAttrSel;
class uiLabeledSpinBox;

/*! \brief Spectral Decomposition Attribute description editor */

class uiSpecDecompAttrib : public uiAttrDescEd
{
public:

			uiSpecDecompAttrib(uiParent*);

    void		getEvalParams(TypeSet<EvalParam>&) const;
    int			getOutputIdx(float) const;
    float		getOutputValue(int) const;

protected:

    uiImagAttrSel*	inpfld;
    uiGenInput*		typefld;
    uiGenInput*         gatefld;
    uiLabeledSpinBox*	outpfld;
    uiLabeledSpinBox*	stepfld;
    uiGenInput*		waveletfld;

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);
    bool		setOutput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
    bool		getOutput(Attrib::Desc&);

    void		inputSel(CallBacker*);
    void		typeSel(CallBacker*);
    void		stepChg(CallBacker*);

    float		nyqfreq;
    int			nrsamples; //!< Nr of samples in selected data
    float		ds; //!< Sample spacing of selected data
};

#endif
