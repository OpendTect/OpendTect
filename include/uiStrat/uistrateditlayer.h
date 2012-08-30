#ifndef uistrateditlayer_h
#define uistrateditlayer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          August 2012
 RCS:           $Id: uistrateditlayer.h,v 1.1 2012-08-30 13:11:22 cvsbert Exp $
________________________________________________________________________

-*/

#include "uistratmod.h"
#include "uidialog.h"
class BufferStringSet;
class UnitOfMeasure;
class uiGenInput;
class uiPropertyValFld;
namespace Strat { class Layer; class LayerSequence; }

/*!\brief Displays and optionally edits a Strat::Layer instance */

mClass(uiStrat) uiStratEditLayer : public uiDialog
{
public:
			uiStratEditLayer(uiParent*,Strat::Layer&,
					 const Strat::LayerSequence&,
					 bool editable=true);

    void		getUnits(ObjectSet<const UnitOfMeasure>&) const;

protected:

    bool			editable_;
    uiGenInput*			lithfld_;
    uiGenInput*			topfld_;
    ObjectSet<uiPropertyValFld>	valflds_;

    bool			acceptOK(CallBacker*);

};


#endif
