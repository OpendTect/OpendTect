#ifndef uiprestackattrib_h
#define uiprestackattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        B.Bril & H.Huck
 Date:          Jan 2008
 RCS:           $Id: uiprestackattrib.h,v 1.11 2011-06-27 08:41:16 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"
#include "datapack.h"

class CtxtIOObj;
namespace Attrib { class Desc; };

class uiLabel;
class uiGenInput;
class uiSeisSel;
namespace PreStack { class uiProcSel; }

/*! \brief PreStack Attribute ui */

mClass uiPreStackAttrib : public uiAttrDescEd
{
public:

			uiPreStackAttrib(uiParent*,bool);
			~uiPreStackAttrib();

    void                getEvalParams(TypeSet<EvalParam>&) const;

    void        	setDataPackInp(const TypeSet<DataPack::FullID>&);

protected:

    uiSeisSel*				seisinpfld_;
    uiAttrSel*				datapackinpfld_;
    uiGenInput*				dopreprocessfld_;
    PreStack::uiProcSel*		preprocsel_;
    uiGenInput*				offsrgfld_;
    uiGenInput*				calctypefld_;
    uiGenInput*				stattypefld_;
    uiGenInput*				lsqtypefld_;
    uiGenInput*				offsaxtypefld_;
    uiGenInput*				valaxtypefld_;
    uiGenInput*				useazimfld_;
    uiLabel*				xlbl_;

    CtxtIOObj&				ctio_;

    bool		usedatapackasinput_;

    bool		setParameters(const Attrib::Desc&);
    bool		getParameters(Attrib::Desc&);

    void		calcTypSel(CallBacker*);
    void		doPreProcSel(CallBacker*);

    bool        	setInput(const Desc&);

    			mDeclReqAttribUIFns
};

#endif
