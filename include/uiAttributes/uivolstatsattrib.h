#ifndef uivolstatsattrib_h
#define uivolstatsattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: uivolstatsattrib.h,v 1.4 2006-09-11 07:04:12 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

namespace Attrib { class Desc; };
class uiGenInput;
class uiAttrSel;
class uiSteeringSel;
class uiStepOutSel;


/*! \brief VolumeStatistics Attribute description editor */

class uiVolumeStatisticsAttrib : public uiAttrDescEd
{
public:
    static void		initClass();
			uiVolumeStatisticsAttrib(uiParent*);

    const char*		getAttribName() const;
    void		getEvalParams(TypeSet<EvalParam>& params) const;

protected:
    static uiAttrDescEd* createInstance(uiParent*);

    uiAttrSel*		inpfld;
    uiSteeringSel*	steerfld;
    uiGenInput*		gatefld;
    uiGenInput*		shapefld;
    uiStepOutSel*	stepoutfld;
    uiGenInput*		outpfld;

    virtual void	set2D(bool);

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);
    bool		setOutput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
    bool		getOutput(Attrib::Desc&);
};


#endif
