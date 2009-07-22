#ifndef uisemblanceattrib_h
#define uisemblanceattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2008
 RCS:           $Id: uisemblanceattrib.h,v 1.2 2009-07-22 16:01:28 cvsbert Exp $
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
