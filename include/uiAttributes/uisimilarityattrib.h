#ifndef uisimilarityattrib_h
#define uisimilarityattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2005
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uiattrdesced.h"
#include "uisteeringsel.h"

class uiAttrSel;
class uiGenInput;
class uiStepOutSel;


/*! \brief Similarity Attribute description editor */

mClass(uiAttributes) uiSimilarityAttrib : public uiAttrDescEd
{
public:

			uiSimilarityAttrib(uiParent*,bool);

    void		getEvalParams(TypeSet<EvalParam>&) const;

protected:

    uiAttrSel*		inpfld_;
    uiGenInput*		gatefld_;
    uiGenInput*		extfld_;
    uiStepOutSel*	pos0fld_;
    uiStepOutSel*	pos1fld_;
    uiStepOutSel*	stepoutfld_;
    uiGenInput*		outpstatsfld_;
    uiGenInput*		maxdipfld_;
    uiGenInput*		deltadipfld_;
    uiGenInput*		outpdipfld_;
    uiGenInput*		dooutpstatsfld_;

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);
    bool		setOutput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
    bool		getOutput(Attrib::Desc&);

    void		extSel(CallBacker*);
    void		outSel(CallBacker*);
    void		steerTypeSel(CallBacker*);

    			mDeclReqAttribUIFns

    mClass(uiAttributes) uiSimiSteeringSel : public uiSteeringSel
    {                                                                           
	public:                                                                     
			uiSimiSteeringSel(uiParent*,const Attrib::DescSet*,
					  bool is2d);
	   
	    bool	willSteer() const;	
	    bool	wantBrowseDip() const;
	    int		browseDipIdxInList() const;
				                                             
	    Notifier<uiSimiSteeringSel>	typeSelected;

	protected:                                                                  
	    void	typeSel(CallBacker*);
    };

    uiSimiSteeringSel*	steerfld_;
};


#endif

