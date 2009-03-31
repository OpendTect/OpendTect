#ifndef uieventattrib_h
#define uieventattrib_h

/*+
 ________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          February 2005
 RCS:           $Id: uieventattrib.h,v 1.7 2009-03-31 10:01:39 cvshelene Exp $
 ________________________________________________________________________

-*/

#include "uiattrdesced.h"

namespace Attrib { class Desc; }

class uiAttrSel;
class uiGenInput;
class uiLabel;

/*! \brief Event Attributes description editor */

mClass uiEventAttrib : public uiAttrDescEd
{
public:

                        uiEventAttrib(uiParent*,bool);

protected:

    uiAttrSel*          inpfld;
    uiGenInput*         issinglefld;
    uiGenInput*         tonextfld;
    uiGenInput*		outpfld;
    uiGenInput*		evtypefld;
    uiGenInput*		gatefld;
    uiLabel*		tonextlblfld;

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);
    bool		setOutput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
    bool		getOutput(Attrib::Desc&);

    void		isSingleSel(CallBacker*);
    void                isGateSel(CallBacker*);

    			mDeclReqAttribUIFns
};


#endif
