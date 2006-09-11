#ifndef uimathattrib_h
#define uimathattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          October 2001
 RCS:           $Id: uimathattrib.h,v 1.3 2006-09-11 07:04:12 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

namespace Attrib { class Desc; };
class uiAttrSel;
class uiGenInput;
class uiPushButton;

/*! \brief Math Attribute description editor */

class uiMathAttrib : public uiAttrDescEd
{
public:
    static void		initClass();
			uiMathAttrib(uiParent*);

    const char*		getAttribName() const;

protected:
    static uiAttrDescEd* createInstance(uiParent*);

    uiGenInput*         inpfld;
    uiPushButton*	parsebut;
    ObjectSet<uiAttrSel> attribflds;

    int			nrvariables;
    void 		parsePush(CallBacker*);

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
};

#endif
