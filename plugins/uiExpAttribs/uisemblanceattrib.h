#ifndef uisemblanceattrib_h
#define uisemblanceattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          January 2008
 RCS:           $Id: uisemblanceattrib.h,v 1.1 2008-10-17 05:42:10 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

namespace Attrib { class Desc; }
class uiAttrSel;
class uiGenInput;


class uiSemblanceAttrib : public uiAttrDescEd
{
public:
			uiSemblanceAttrib(uiParent*,bool);
    			mDeclReqAttribUIFns

protected:

    uiAttrSel*		inpfld_;
   
    uiGenInput*		sz3dfld_;
    uiGenInput*		sz2dfld_;

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
};

#endif
