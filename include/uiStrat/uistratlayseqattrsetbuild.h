#ifndef uistratlayseqattrsetbuild_h
#define uistratlayseqattrsetbuild_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2011
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uistratmod.h"
#include "uibuildlistfromlist.h"
#include "propertyref.h"
class CtxtIOObj;
namespace Strat { class RefTree; class LayerModel; class LaySeqAttribSet; }


/*!\brief allows user to define (or read) a set of layer sequence attributes */

mClass(uiStrat) uiStratLaySeqAttribSetBuild : public uiBuildListFromList
{
public:

    enum SetTypeSel	{ AllTypes, OnlyLocal, OnlyIntegrated };

    			uiStratLaySeqAttribSetBuild(uiParent*,
						const Strat::LayerModel&,
						SetTypeSel sts=AllTypes,
						Strat::LaySeqAttribSet* a=0);
    			~uiStratLaySeqAttribSetBuild();

    const Strat::LaySeqAttribSet& attribSet() const	{ return attrset_; }
    const PropertyRefSelection&	  propertyRefs() const	{ return props_; }

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

    virtual void	editReq(bool);
    virtual void	removeReq();
    virtual bool	ioReq(bool);
    virtual const char*	avFromDef(const char*) const;

};


#endif

