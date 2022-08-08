#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          August 2012
________________________________________________________________________

-*/

#include "uistratmod.h"
#include "uidialog.h"
class UnitOfMeasure;
class uiGenInput;
class uiStratLayerContent;
class uiPropertyValFld;
namespace Strat { class Layer; class LayerSequence; }

/*!\brief Displays and optionally edits a Strat::Layer instance */

mExpClass(uiStrat) uiStratEditLayer : public uiDialog
{ mODTextTranslationClass(uiStratEditLayer);
public:
			uiStratEditLayer(uiParent*,Strat::Layer&,
					 const Strat::LayerSequence&,
					 bool editable=true);
			~uiStratEditLayer();

    bool		isChanged() const	{ return chgd_; }

protected:

    const Strat::Layer&	lay_;
    Strat::Layer&	worklay_;
    uiGenInput*		lithfld_;
    uiGenInput*		topfld_;
    ObjectSet<uiPropertyValFld> valflds_;
    uiStratLayerContent* contfld_;
    const bool		editable_;
    bool		chgd_;

    bool		getFromScreen(bool);
    void		valChg(CallBacker*);
    bool		acceptOK(CallBacker*) override;

};


