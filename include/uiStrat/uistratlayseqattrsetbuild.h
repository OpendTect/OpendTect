#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uistratmod.h"
#include "uibuildlistfromlist.h"
#include "propertyref.h"

class CtxtIOObj;
namespace Strat { class RefTree; class LayerModel; class LaySeqAttribSet; }


/*!\brief allows user to define (or read) a set of layer sequence attributes */

mExpClass(uiStrat) uiStratLaySeqAttribSetBuild : public uiBuildListFromList
{ mODTextTranslationClass(uiStratLaySeqAttribSetBuild);
public:

    enum SetTypeSel	{ AllTypes, OnlyLocal, OnlyIntegrated };

    			uiStratLaySeqAttribSetBuild(uiParent*,
						const Strat::LayerModel&,
						SetTypeSel sts=AllTypes,
						Strat::LaySeqAttribSet* a=0);
    			~uiStratLaySeqAttribSetBuild();

    const Strat::LaySeqAttribSet& attribSet() const	{ return attrset_; }
    const PropertyRefSelection&   propertyRefs() const	{ return props_; }

    bool			handleUnsaved();
    				//!< Only returns false on user cancel
    bool			haveChange() const	{ return anychg_; }

protected:

    Strat::LaySeqAttribSet&	attrset_;
    const bool			setismine_;
    const Strat::RefTree&	reftree_;
    PropertyRefSelection	props_;
    CtxtIOObj&			ctio_;
    const SetTypeSel		typesel_;
    bool			anychg_;

    void		editReq(bool) override;
    void		removeReq() override;
    bool		ioReq(bool) override;
    const char*		avFromDef(const char*) const override;

};
