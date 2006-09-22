#ifndef uihorizonattrib_h
#define uihorizonattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          September 2006
 RCS:           $Id: uihorizonattrib.h,v 1.1 2006-09-22 15:14:43 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

namespace Attrib { class Desc; };

class CtxtIOObj;
class uiAttrSel;
class uiGenInput;
class uiIOObjSel;


/*! \brief Coherency attribute description editor */

class uiHorizonAttrib : public uiAttrDescEd
{
public:
    static void		initClass();
			uiHorizonAttrib(uiParent*);
			~uiHorizonAttrib();

    const char*		getAttribName() const;

protected:
    static uiAttrDescEd* createInstance(uiParent*);

    uiAttrSel*		inpfld;
    uiIOObjSel*		horfld;
    uiGenInput*		outputfld;

    CtxtIOObj&		horctio_;

    bool                setParameters(const Attrib::Desc&);
    bool                setInput(const Attrib::Desc&);

    bool                getParameters(Attrib::Desc&);
    bool                getInput(Attrib::Desc&);
    virtual void	set2D(bool);
};


#endif
