#ifndef uiprestackattrib_h
#define uiprestackattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        B.Bril & H.Huck
 Date:          Jan 2008
 RCS:           $Id: uiprestackattrib.h,v 1.1 2008-01-14 15:59:44 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

namespace Attrib { class Desc; };

class CtxtIOObj;
class uiGenInput;
class uiSeisSel;

/*! \brief PreStack Attribute ui */

class uiPreStackAttrib : public uiAttrDescEd
{
public:

			uiPreStackAttrib(uiParent*,bool);

protected:

    uiSeisSel*		inpfld_;
    uiGenInput*         typefld_;

    CtxtIOObj*		ctxt_;		

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);
    bool                setOutput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
    bool                getOutput(Attrib::Desc&);

    			mDeclReqAttribUIFns
};

#endif
