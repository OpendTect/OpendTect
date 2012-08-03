#ifndef uiscalingattrib_h
#define uiscalingattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          December 2004
 RCS:           $Id: uiscalingattrib.h,v 1.16 2012-08-03 13:00:49 cvskris Exp $
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uiattrdesced.h"
#include "attribdescid.h"

namespace Attrib { class Desc; class EngineMan; class Processor; }
class uiParent;
class uiAttrSel;
class uiGenInput;
class uiPushButton;
class uiTable;


/*! \brief Scaling Attribute description editor */

mClass(uiAttributes) uiScalingAttrib : public uiAttrDescEd
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

