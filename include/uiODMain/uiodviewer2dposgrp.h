#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"

#include "uigroup.h"
#include "uislicesel.h"
#include "uistring.h"

#include "attribsel.h"
#include "integerid.h"
#include "multiid.h"
#include "survgeom.h"
#include "survinfo.h"
#include "trckeyzsampling.h"

class IOObj;
class uiAttrSel;
class uiIOObjSel;
class uiLabeledComboBox;
class uiODApplMgr;
class uiODMain;
class uiPushButton;
class uiSeis2DSubSel;
class uiSliceSel;

namespace Geometry { class RandomLine; }

mStruct(uiODMain) Viewer2DPosDataSel
{
    enum PosType	{InLine=0, CrossLine=1, Line2D=2, ZSlice=3, RdmLine=4 };
			mDeclareEnumUtils(PosType);

			Viewer2DPosDataSel() { clean(); }
    virtual		~Viewer2DPosDataSel() {}
			Viewer2DPosDataSel(const Viewer2DPosDataSel& sd)
			{
			    postype_	    = sd.postype_;
			    selspec_	    = sd.selspec_;
			    tkzs_	    = sd.tkzs_;
			    rdmlineid_	    = sd.rdmlineid_;
			    rdmlinemultiid_ = sd.rdmlinemultiid_;
			    geomid_	    = sd.geomid_;
			    selectdata_	    = sd.selectdata_;
			}

    virtual void	clean()
			{
			    postype_ = SI().has3D() ? Viewer2DPosDataSel::InLine
						: Viewer2DPosDataSel::Line2D;
			    selspec_ = Attrib::SelSpec();
			    tkzs_ = TrcKeyZSampling(true);
			    rdmlinemultiid_ = MultiID::udf();
			    rdmlineid_.setUdf();
			    geomid_ = Survey::GeometryManager::cUndefGeomID();
			    selectdata_	= true;
			}

    PosType		postype_;
    Attrib::SelSpec	selspec_;
    TrcKeyZSampling	tkzs_;
    Pos::GeomID		geomid_;
    MultiID		rdmlinemultiid_;
    RandomLineID	rdmlineid_;
    bool		selectdata_;

    static const char*	sKeyRdmLineMultiID(){ return "Random Line MultiID"; }
    static const char*  sKeyRdmLineID()	    { return "Random Line ID"; }
    static const char*	sKeySelectData()    { return "Select Data"; }

    virtual void	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);
};


mExpClass(uiODMain) uiODViewer2DPosGrp : public uiGroup
{ mODTextTranslationClass(uiODViewer2DPosGrp)
public:

				uiODViewer2DPosGrp(uiParent*,
						   Viewer2DPosDataSel*,
						   bool onlyvertical,
						   bool withpostype=false);
				// Viewer2DPosDataSel objects becomes mine

				~uiODViewer2DPosGrp();

    bool			is2D() const;
    Viewer2DPosDataSel::PosType selPosType() const
				{ return posdatasel_->postype_; }
    void			showDataSelField(bool yn);
    void			setApplSceneMgr(uiODMain&);
    virtual void		fillPar(IOPar&) const;
    virtual void		usePar(const IOPar&);
    Viewer2DPosDataSel&		posDataSel()		{ return *posdatasel_; }
    const Viewer2DPosDataSel&	posDataSel() const	{ return *posdatasel_; }
    virtual bool		commitSel( bool emiterror );

    Notifier<uiODViewer2DPosGrp> inpSelected;

protected:

    bool		onlyvertical_;
    Viewer2DPosDataSel* posdatasel_;

    uiODApplMgr*	applmgr_;

    uiLabeledComboBox*	postypefld_;
    uiAttrSel*		inp2dfld_;
    uiAttrSel*		inp3dfld_;
    uiIOObjSel*		rdmlinefld_;
    uiSeis2DSubSel*	subsel2dfld_;
    uiPushButton*	gen2dlinebut_;
    uiPushButton*	genrdmlinebut_;
    ObjectSet<uiSliceSel> sliceselflds_;
    uiGroup*		topgrp_;
    uiGroup*		botgrp_;

    IOObj*		get2DObj();
    void		init(bool);
    void		updateFlds();
    void		updateDataSelFld();
    void		updatePosFlds();
    void		updateTrcKeySampFld();
    void		createSliceSel(uiSliceSel::Type);

    void		gen2DLine(CallBacker*);
    void		genRdmLine(CallBacker*);
    void		rdmLineDlgClosed(CallBacker*);
    void		inpSel(CallBacker*);
    void		attr2DSelected(CallBacker*);

    void		getSelAttrSamp(TrcKeyZSampling&);
};
