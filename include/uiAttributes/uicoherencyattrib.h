#ifndef uicoherencyattrib_h
#define uicoherencyattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: uicoherencyattrib.h,v 1.2 2006-09-11 13:14:10 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

namespace Attrib { class Desc; };

class uiAttrSel;
class uiImagAttrSel;
class uiGenInput;
class uiStepOutSel;


/*! \brief Coherency attribute description editor */

class uiCoherencyAttrib : public uiAttrDescEd
{
public:
    static void		initClass();
			uiCoherencyAttrib(uiParent*);

    const char*		getAttribName() const;

protected:
    static uiAttrDescEd* createInstance(uiParent*);

    uiImagAttrSel*	inpfld;
    uiGenInput*		is1fld;
    uiGenInput*		tgfld;
    uiGenInput*		maxdipfld;
    uiGenInput*		deltadipfld;
    uiStepOutSel*	stepoutfld;

    void		is1Sel(CallBacker*);

    bool                setParameters(const Attrib::Desc&);
    bool                setInput(const Attrib::Desc&);

    bool                getParameters(Attrib::Desc&);
    bool                getInput(Attrib::Desc&);
    virtual void	set2D(bool);
};


#endif
