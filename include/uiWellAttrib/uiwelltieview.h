#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellattribmod.h"
#include "uigroup.h"
#include "uiflatviewer.h"
#include "welltiedata.h"
#include "uistring.h"

class SeisTrc;
class SeisTrcBuf;
class SeisTrcBufDataPack;
class uiFunctionDisplay;
class uiGroup;
class uiLabel;
class uiLineItem;
class uiPolyLineItem;
class uiTextItem;
class uiWellDisplayControl;
class uiWellLogDisplay;

namespace Well
{
    class Data;
    class Marker;
}

namespace WellTie
{

class Setup;

mExpClass(uiWellAttrib) uiTieView : public CallBacker
{ mODTextTranslationClass(uiTieView);
public:
				uiTieView(uiParent*,uiFlatViewer*,const Data&);
				~uiTieView();

    void			fullRedraw();
    void			drawUserPicks();
    void			redrawViewer();
    void			redrawViewerAuxDatas();
    void			redrawLogsAuxDatas();

    void			setNrTrcs(int);
    int				nrTrcs() const		{ return nrtrcs_; }

    void			setSEGPositivePolarity(bool);

    void			enableCtrlNotifiers(bool);

    ObjectSet<uiWellLogDisplay>& logDisps() { return logsdisp_; }
    uiGroup* displayGroup()      { return logdispgrp_; }

    Notifier<uiTieView>		infoMsgChanged;

protected:

    uiFlatViewer*		vwr_;
    uiParent*			parent_;
    ObjectSet<uiWellLogDisplay> logsdisp_;
    uiWellDisplayControl*	wellcontrol_;

    const DispParams&		params_;
    const Data&			data_;
    const StepInterval<float>	zrange_;
    const TypeSet<Marker>&	seispickset_;
    const TypeSet<Marker>&	synthpickset_;
    SeisTrcBuf&			trcbuf_;
    SeisTrcBufDataPack*		seisdp_;
    int				nrtrcs_;
    bool			segpospolarity_ = true;

    ObjectSet<FlatView::AuxData> userpickauxdatas_;
    ObjectSet<FlatView::AuxData> wellmarkerauxdatas_;
    ObjectSet<FlatView::AuxData> horauxdatas_;
    ObjectSet<uiTextItem>	hortxtnms_;
    ObjectSet<uiTextItem>	mrktxtnms_;
    uiPolyLineItem*		checkshotitm_;
    uiGroup*            logdispgrp_;

    uiLineItem*			linelog1_;
    uiLineItem*			linelog2_;
    uiLineItem*			lineseis_;

    void		drawLog(const char*,bool,int,bool);
    void		drawTraces();
    void		drawUserPicks(const TypeSet<Marker>&,bool);
    void		drawMarker(FlatView::AuxData*,bool,float);
    void		drawViewerWellMarkers();
    void		drawLogDispWellMarkers();
    void		initFlatViewer();
    void		initLogViewers();
    void		initWellControl();
    void		loadHorizons();
    void		drawHorizons();
    void		setLogsRanges(Interval<float>);
    void		setLogsParams();
    void		setUdfTrc(SeisTrc&) const;
    void		setDataPack();
    void		setInfoMsg(CallBacker*);
    void		zoomChg(CallBacker*);
    void		mouseMoveCB(CallBacker*);
};



mExpClass(uiWellAttrib) uiCrossCorrView : uiGroup
{ mODTextTranslationClass(uiCrossCorrView);
public:

			uiCrossCorrView(uiParent*,const Data&);

    void		set(const Data::CorrelData&);
    void		draw();

protected:

    uiLabel*		lbl_;
    uiFunctionDisplay*	disp_;
    TypeSet<float>	vals_;
    float		lag_;
    float		coeff_;
    const Data&		data_;
};

} // namespace WellTie
