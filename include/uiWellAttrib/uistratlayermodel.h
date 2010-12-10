#ifndef uistratlayermodel_h
#define uistratlayermodel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
 RCS:           $Id: uistratlayermodel.h,v 1.4 2010-12-10 14:32:33 cvsbert Exp $
________________________________________________________________________

-*/

#include "uimainwin.h"
class CtxtIOObj;
class uiGenInput;
class uiSpinBox;
class uiStratSynthDisp;
class uiStratLayerModelDisp;
class uiLayerSequenceGenDesc;
namespace Strat { class LayerModel; class LayerSequenceGenDesc; }


mClass uiStratLayerModel : public uiMainWin
{
public:

				uiStratLayerModel(uiParent*,
						  const char* seqdisptype=0);
				~uiStratLayerModel();

    void			go()		{ show(); }

protected:

    uiLayerSequenceGenDesc*	seqdisp_;
    uiStratLayerModelDisp*	moddisp_;
    uiStratSynthDisp*		synthdisp_;
    uiGenInput*			nrmodlsfld_;

    Strat::LayerSequenceGenDesc& desc_;
    Strat::LayerModel&		modl_;
    CtxtIOObj&			descctio_;
    CtxtIOObj&			modlctio_;

    void			dispEachChg(CallBacker*);
    void			levelChg(CallBacker*);
    void			openGenDesc(CallBacker*);
    void			saveGenDesc(CallBacker*);
    void			genModels(CallBacker*);

public:

    static void			initClass();

};


#endif
