#ifndef uieventattrib_h
#define uieventattrib_h

/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Payraudeau
 Date:          February 2005
 RCS:           $Id$
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
    uiGenInput*		outampfld;
    uiLabel*		tonextlblfld;

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);
    bool		setOutput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
    bool		getOutput(Attrib::Desc&);

    void		isSingleSel(CallBacker*);
    void                isGateSel(CallBacker*);
    void                outAmpSel(CallBacker*);

    			mDeclReqAttribUIFns
};


#endif
