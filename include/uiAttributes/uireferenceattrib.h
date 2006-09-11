#ifndef uireferenceattrib_h
#define uireferenceattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          July 2005
 RCS:           $Id: uireferenceattrib.h,v 1.4 2006-09-11 07:04:12 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

namespace Attrib { class Desc; };

class uiImagAttrSel;
class uiGenInput;

/*! \brief Reference Attribute description editor */

class uiReferenceAttrib : public uiAttrDescEd
{
public:

    static void		initClass();
			uiReferenceAttrib(uiParent*);


    const char*		getAttribName() const;

protected:
    static uiAttrDescEd* createInstance(uiParent*);

    uiAttrSel*          inpfld;
    uiGenInput*		outpfld3d;
    uiGenInput*		outpfld2d;

    static const char*	outstrs3d[];
    static const char*	outstrs2d[];

    bool                is2d_;

    bool		setParameters(const Attrib::Desc&);
    bool                setInput(const Attrib::Desc&);
    bool		setOutput(const Attrib::Desc&);

    bool		getOutput(Attrib::Desc&);
    bool                getInput(Attrib::Desc&);
    bool                getParameters(Attrib::Desc&);

    virtual void        set2D(bool);
};

#endif
