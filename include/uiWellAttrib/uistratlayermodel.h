#ifndef uistratlayermodel_h
#define uistratlayermodel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
 RCS:           $Id: uistratlayermodel.h,v 1.10 2011-07-11 07:28:08 cvsbert Exp $
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

    static const char*		sKeyModeler2Use();

protected:

    uiLayerSequenceGenDesc*	seqdisp_;
    uiStratLayerModelDisp*	moddisp_;
    uiStratSynthDisp*		synthdisp_;
    uiGenInput*			nrmodlsfld_;

    Strat::LayerSequenceGenDesc& desc_;
    Strat::LayerModel&		modl_;
    mutable CtxtIOObj&		descctio_;

    void			dispEachChg(CallBacker*);
    void			levelChg(CallBacker*);
    void			zoomChg(CallBacker*);
    void			wvltChg(CallBacker*);
    void			genModels(CallBacker*);
    void			xPlotReq(CallBacker*);

    void			openGenDescCB(CallBacker*) { openGenDesc(); }
    bool			openGenDesc();
    void			saveGenDescCB(CallBacker*) { saveGenDesc(); }
    bool			saveGenDesc() const;
    bool			saveGenDescIfNecessary() const;
    void			manPropsCB(CallBacker*);

    bool			closeOK();

public:

    static void			initClass();

};


#endif
