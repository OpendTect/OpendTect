#ifndef uiscalingattrib_h
#define uiscalingattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          December 2004
 RCS:           $Id: uiscalingattrib.h,v 1.15 2011/03/17 11:24:07 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"
#include "attribdescid.h"

namespace Attrib { class Desc; class EngineMan; class Processor; }
class uiParent;
class uiAttrSel;
class uiGenInput;
class uiPushButton;
class uiTable;


/*! \brief Scaling Attribute description editor */

mClass uiScalingAttrib : public uiAttrDescEd
{
public:

			uiScalingAttrib(uiParent*,bool);

protected:

    uiParent*		parent_;
    uiAttrSel*		inpfld;
    uiGenInput*		typefld;
    uiGenInput*		nfld;
    uiGenInput*		statsfld;
    uiTable*		table;
    uiGenInput*         windowfld;
    uiGenInput*         lowenergymute;
    uiGenInput*         sqrgfld;
    uiGenInput*         squrgfld;
    uiPushButton*	analysebut_;

    TypeSet<float>	zvals_;
    TypeSet<float>	scalefactors_;

    void		typeSel(CallBacker*);
    void		statsSel(CallBacker*);
    void		analyseCB(CallBacker*);

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);

    void		getEvalParams(TypeSet<EvalParam>&) const;

    bool		areUIParsOK();

    			mDeclReqAttribUIFns
};

#endif
