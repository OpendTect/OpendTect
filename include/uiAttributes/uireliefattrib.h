#ifndef uireliefattrib_h
#define uireliefattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		July 2016
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uiattrdesced.h"

namespace Attrib { class Desc; }

class uiAttrSel;
class uiGenInput;

/*!
\brief Pseudo %Relief Attribute description editor
*/


mExpClass(uiAttributes) uiReliefAttrib : public uiAttrDescEd
{ mODTextTranslationClass(uiReliefAttrib)
public:
			uiReliefAttrib(uiParent*,bool);

protected:

    uiAttrSel*		inpfld_;
    uiGenInput*		gatefld_;

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);
    bool		setOutput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    uiRetVal		getInput(Attrib::Desc&);
    bool		getOutput(Attrib::Desc&);

			mDeclReqAttribUIFns
};

#endif
