#ifndef uispecdecompattrib_h
#define uispecdecompattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2003
 RCS:           $Id: uispecdecompattrib.h,v 1.9 2009-07-22 16:01:20 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

namespace Attrib { class Desc; };

class uiGenInput;
class uiImagAttrSel;
class uiLabeledSpinBox;

/*! \brief Spectral Decomposition Attribute description editor */

mClass uiSpecDecompAttrib : public uiAttrDescEd
{
public:

			uiSpecDecompAttrib(uiParent*,bool);

    void		getEvalParams(TypeSet<EvalParam>&) const;
    int			getOutputIdx(float) const;
    float		getOutputValue(int) const;

protected:

    uiImagAttrSel*	inpfld_;
    uiGenInput*		typefld_;
    uiGenInput*         gatefld_;
    uiLabeledSpinBox*	outpfld_;
    uiLabeledSpinBox*	stepfld_;
    uiGenInput*		waveletfld_;

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);
    bool		setOutput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
    bool		getOutput(Attrib::Desc&);

    void		inputSel(CallBacker*);
    void		typeSel(CallBacker*);
    void		stepChg(CallBacker*);

    void		checkOutValSnapped() const;

    float		nyqfreq_;
    int			nrsamples_; //!< Nr of samples in selected data
    float		ds_; //!< Sample spacing of selected data

    			mDeclReqAttribUIFns
};

#endif
