#ifndef uiprestackattrib_h
#define uiprestackattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        B.Bril & H.Huck
 Date:          Jan 2008
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uiattrdesced.h"
#include "datapack.h"
#include "stattype.h"

class CtxtIOObj;
namespace Attrib { class Desc; };

class uiCheckBox;
class uiGenInput;
class uiLabel;
class uiPreStackSel;
class uiVelSel;
namespace PreStack { class uiProcSel; class uiAngleCompGrp; 
		     class AngleCompParams; class AngleComputer; }

/*! \brief PreStack Attribute ui */

mExpClass(uiAttributes) uiPreStackAttrib : public uiAttrDescEd
{
public:

			uiPreStackAttrib(uiParent*,bool);
			~uiPreStackAttrib();

    void                getEvalParams(TypeSet<EvalParam>&) const;

    void        	setDataPackInp(const TypeSet<DataPack::FullID>&);

protected:

    uiPreStackSel*			prestackinpfld_;
    uiGenInput*				dopreprocessfld_;
    PreStack::uiProcSel*		preprocsel_;
    uiGenInput*				offsrgfld_;
    uiGenInput*				calctypefld_;
    uiGenInput*				stattypefld_;
    uiGenInput*				lsqtypefld_;
    uiGenInput*				offsaxtypefld_;
    uiGenInput*				valaxtypefld_;
    uiCheckBox*				useanglefld_;
    uiLabel*				offsrglbl_;

    PreStack::uiAngleCompGrp*		anglecompgrp_;
    PreStack::AngleCompParams&		params_;

    bool		usedatapackasinput_;

    bool		setParameters(const Attrib::Desc&);
    bool		setAngleParameters(const Attrib::Desc&);
    bool		getParameters(Attrib::Desc&);
    bool		getAngleParameters(Attrib::Desc&);

    Stats::Type		getStatEnumfromString(const char* stattypename);
    const char*		getStringfromStatEnum(Stats::Type enm);
    void		getStatTypeNames(BufferStringSet& stattypenames);

    void		calcTypSel(CallBacker*);
    void		angleTypSel(CallBacker*);
    void		doPreProcSel(CallBacker*);

    bool        	setInput(const Desc&);

    			mDeclReqAttribUIFns
};

#endif

