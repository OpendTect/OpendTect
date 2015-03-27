#ifndef uiodviewer2dposgrp_h
#define uiodviewer2dposgrp_h

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno/Satyaki
Date:	       Aug 2010
RCS:	       $Id: uiwellcorrstartdlg.h,v 1.1 2009-01-19 13:02:33 cvsbruno Exp
$
________________________________________________________________________

-*/


#include "uiodmainmod.h"
#include "uigroup.h"
#include "uislicesel.h"
#include "uistring.h"

#include "attribsel.h"
#include "multiid.h"
#include "survgeom.h"
#include "trckeyzsampling.h"

class IOObj;
class uiAttrSel;
class uiComboBox;
class uiIOObjSel;
class uiODMain;
class uiODApplMgr;
class uiODSceneMgr;
class uiPushButton;
class uiSeis2DSubSel;
class uiSliceSel;

namespace Geometry { class RandomLine; }

mStruct(uiODMain) Viewer2DPosDataSel
{
			Viewer2DPosDataSel() { clean(); };
			Viewer2DPosDataSel(const Viewer2DPosDataSel& sd)
			{
			    selspec_	= sd.selspec_;
			    tkzs_	= sd.tkzs_;
			    rdmlineid_	= sd.rdmlineid_;
			    geomid_	= sd.geomid_;
			    selectdata_ = sd.selectdata_;
			}

    void		clean()
			{
			    selspec_ = Attrib::SelSpec();
			    tkzs_ = TrcKeyZSampling(true);
			    rdmlineid_ = MultiID::udf();
			    geomid_ = Survey::GeometryManager::cUndefGeomID();
			    selectdata_ = true;
			}

    Attrib::SelSpec	selspec_;
    TrcKeyZSampling	tkzs_;
    Pos::GeomID		geomid_;
    MultiID		rdmlineid_;
    bool		selectdata_;

    static const char*	sKeyRdmLineID() { return "Random Line ID"; }
    static const char*	sKeySelectData(){ return "Select Data"; }

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);
};


mExpClass(uiODMain) uiODViewer2DPosGrp : public uiGroup
{ mODTextTranslationClass(uiODViewer2DPosGrp);
public:

			uiODViewer2DPosGrp(uiParent*,Viewer2DPosDataSel&,
					   bool wantz);
			~uiODViewer2DPosGrp();

    bool		is2D() const;
    void		showDataSelField(bool yn);
    void		setApplSceneMgr(uiODMain&);
    void		getRdmLineGeom(TypeSet<BinID>&,
				       StepInterval<float>* zrg=0);

    Notifier<uiODViewer2DPosGrp> inpSelected;

protected:

    bool		withz_;
    Viewer2DPosDataSel& posdatasel_;

    uiODApplMgr*	applmgr_;
    uiODSceneMgr*	scenemgr_;

    uiComboBox*		postypefld_;
    uiAttrSel*		inp2dfld_;
    uiAttrSel*		inp3dfld_;
    uiIOObjSel*		rdmlinefld_;
    uiSeis2DSubSel*	subsel2dfld_;
    uiPushButton*	gen2dlinebut_;
    uiPushButton*	genrdmlinebut_;
    ObjectSet<uiSliceSel> sliceselflds_;

    enum PosType	{RdmLine=0, InLine=1, CrossLine=2, Line2D=3, ZSlice=4};
			DeclareEnumUtils(PosType);
    PosType		tp_;

    IOObj*		get2DObj();
    void		getPosSubSel();
    void		createSliceSel(uiSliceSel::Type);
    bool		acceptOK();

    void		gen2DLine(CallBacker*);
    void		genRdmLine(CallBacker*);
    void		rdmLineDlgClosed(CallBacker*);
    void		inpSel(CallBacker*);
    void		preview2DLine(CallBacker*);
    void		attr2DSelected(CallBacker*);
};

#endif

