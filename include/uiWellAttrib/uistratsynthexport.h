#pragma once
/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki / Bert
 Date:          July 2013
 _______________________________________________________________________

      -*/


#include "uiwellattribmod.h"
#include "uidialog.h"
#include "stratsynthdatamgr.h"

class StratSynthLevel;
class uiCheckBox;
class uiGroup;
class uiGenInput;
class uiSeisSel;
class uiIOObjSel;
class uiPickSetIOObjSel;
class uiSeis2DLineNameSel;
class uiStratSynthOutSel;

namespace Geometry { class RandomLine; }
namespace PosInfo { class Line2DData; }
namespace StratSynth { class DataMgr; }


mExpClass(uiWellAttrib) uiStratSynthExport : public uiDialog
{ mODTextTranslationClass(uiStratSynthExport)
public:

    typedef StratSynth::DataMgr		DataMgr;

    enum GeomSel	{ StraightLine, Polygon, RandomLine, Existing };

			uiStratSynthExport(uiParent*,const DataMgr&);
			~uiStratSynthExport();

protected:

    typedef DataMgr::SynthID		SynthID;
    typedef DataMgr::SynthIDSet		SynthIDSet;
    typedef PosInfo::Line2DData		Line2DData;

    uiGenInput*		crnewfld_;
    uiSeisSel*		linesetsel_;
    uiGenInput*		newlinenmfld_;
    uiSeis2DLineNameSel* existlinenmsel_;
    uiGroup*		geomgrp_;
    uiGenInput*		geomsel_;
    uiGenInput*		coord0fld_;
    uiGenInput*		coord1fld_;
    uiPickSetIOObjSel*	picksetsel_;
    uiIOObjSel*		randlinesel_;
    uiStratSynthOutSel*	poststcksel_;
    uiStratSynthOutSel*	horsel_;
    uiStratSynthOutSel*	prestcksel_;
    uiGenInput*		prefxfld_;
    uiGenInput*		postfxfld_;
    uiCheckBox*		repludfsfld_;

    const DataMgr&	datamgr_;
    SynthIDSet		selids_;
    BufferStringSet	sellvls_;

    GeomSel		selType() const;
    void		fillGeomGroup();
    void		getSelections();
    bool		createHor2Ds(const char*,const char*);
    Pos::GeomID		getGeometry(Line2DData&);
    void		create2DGeometry(const TypeSet<Coord>&,
					 Line2DData&);

    void		crNewChg(CallBacker*);
    void		geomSel(CallBacker*);

    bool		acceptOK();

};
