#ifndef uistratlayermodel_h
#define uistratlayermodel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
 RCS:           $Id: uistratlayermodel.h,v 1.19 2012-01-11 10:56:25 cvsbert Exp $
________________________________________________________________________

-*/

#include "uimainwin.h"
class CtxtIOObj;
class ElasticPropSelection;
class uiGenInput;
class uiSpinBox;
class uiStratSynthDisp;
class uiStratLayerModelDisp;
class uiLayerSequenceGenDesc;
class uiStratGenDescTools;
class uiStratLayModEditTools;
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
    uiStratGenDescTools*	gentools_;
    uiStratLayModEditTools*	modtools_;

    Strat::LayerSequenceGenDesc& desc_;
    Strat::LayerModel&		modl_;
    CtxtIOObj&			descctio_;
    ElasticPropSelection*	elpropsel_;

    void			dispEachChg(CallBacker*);
    void			levelChg(CallBacker*);
    void			zoomChg(CallBacker*);
    void			wvltChg(CallBacker*);
    void			modSelChg(CallBacker*);
    void			genModels(CallBacker*);
    void			xPlotReq(CallBacker*);

    void			setWinTitle();
    void			setModelProps();
    void			setElasticProps();
    void			selElasticPropsCB(CallBacker*);
    bool			selElasticProps(ElasticPropSelection&);
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
