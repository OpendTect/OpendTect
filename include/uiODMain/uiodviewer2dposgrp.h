#pragma once

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
#include "dbman.h"
#include "dbkey.h"
#include "survgeom.h"
#include "survinfo.h"
#include "trckeyzsampling.h"

class Probe;
class IOObj;
class uiAttrSel;
class uiLabeledComboBox;
class uiIOObjSel;
class uiODMain;
class uiODApplMgr;
class uiPushButton;
class uiSeis2DSubSel;
class uiSliceSel;

namespace Geometry { class RandomLine; }

mStruct(uiODMain) Viewer2DPosDataSel
{
    enum PosType	{InLine=0, CrossLine=1, Line2D=2, ZSlice=3, RdmLine=4 };
			mDeclareEnumUtils(PosType);

			Viewer2DPosDataSel()
			    : tkzs_(false)
			{ clean(); }
    virtual		~Viewer2DPosDataSel() {}
			Viewer2DPosDataSel(const Viewer2DPosDataSel& sd)
			{
			    postype_	    = sd.postype_;
			    selspec_	    = sd.selspec_;
			    tkzs_	    = sd.tkzs_;
			    rdmlineid_	    = sd.rdmlineid_;
			    rdmlinedbkey_ = sd.rdmlinedbkey_;
			    geomid_	    = sd.geomid_;
			    selectdata_	    = sd.selectdata_;
			}

    virtual void	clean()
			{
			    postype_ = DBM().isBad() || SI().has3D()
				     ? Viewer2DPosDataSel::InLine
				     : Viewer2DPosDataSel::Line2D;
			    selspec_ = Attrib::SelSpec();
			    tkzs_.init( !DBM().isBad() );
			    rdmlineid_= -1;
			    rdmlinedbkey_ = DBKey::getInvalid();
			    rdmlineid_ = mUdf(int);
			    geomid_ = Pos::GeomID();
			    selectdata_	= true;
			}

    Probe*		createNewProbe();
    void		fillFromProbe(const Probe&);
    PosType		postype_;
    Attrib::SelSpec	selspec_;
    TrcKeyZSampling	tkzs_;
    Pos::GeomID		geomid_;
    DBKey		rdmlinedbkey_;
    int			rdmlineid_;
    bool		selectdata_;

    static const char*	sKeyRdmLineDBKey(){ return "Random Line DBKey"; }
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
};
