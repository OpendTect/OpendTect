#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
    bool		chgd_ = false;

    bool		getFromScreen(bool);
    void		initDlg(CallBacker*);
    void		valChg(CallBacker*);
    bool		acceptOK(CallBacker*) override;

};
