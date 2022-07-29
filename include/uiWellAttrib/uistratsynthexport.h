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
#include "stratsynth.h"

class uiCheckBox;
class uiGroup;
class uiGenInput;
class uiSeisSel;
class uiIOObjSel;
class uiSeis2DLineNameSel;
class uiStratSynthOutSel;

namespace Geometry { class RandomLine; }
namespace PosInfo { class Line2DData; }
namespace StratSynth { class Level; }

mExpClass(uiWellAttrib) uiStratSynthExport : public uiDialog
{ mODTextTranslationClass(uiStratSynthExport)
public:
    enum GeomSel	{ StraightLine, Polygon, RandomLine, Existing };

			uiStratSynthExport(uiParent*,
					   const StratSynth::DataMgr&);
			~uiStratSynthExport();


protected:

    uiGenInput*		crnewfld_;
    uiSeisSel*		linesetsel_;
    uiGenInput*		newlinenmfld_;
    uiSeis2DLineNameSel* existlinenmsel_;
    uiGroup*		geomgrp_;
    uiGenInput*		geomsel_;
    uiGenInput*		coord0fld_;
    uiGenInput*		coord1fld_;
    uiIOObjSel*		picksetsel_;
    uiIOObjSel*		randlinesel_	= nullptr;
    uiStratSynthOutSel* poststcksel_	= nullptr;
    uiStratSynthOutSel*	horsel_;
    uiStratSynthOutSel* prestcksel_	= nullptr;
    uiGenInput*		prefxfld_;
    uiGenInput*		postfxfld_;
    uiCheckBox*		repludfsfld_	= nullptr;

    const StratSynth::DataMgr*	datamgr_;
    TypeSet<SynthID>	selids_;
    BufferStringSet	sellvls_;

    GeomSel		selType() const;
    void		addPrePostFix(BufferString&) const;
    void		fillGeomGroup();
    void		getSelections();
    void		getLevels(ObjectSet<StratSynth::Level>&) const;
    void		removeNonSelected();
    bool		createHor2Ds();
    bool		getGeometry(const char* linenm);
    bool		createAndWrite2DGeometry(const TypeSet<Coord>&,
						 Pos::GeomID) const;
    void		getCornerPoints(Coord& start,Coord& stop);

    void		crNewChg(CallBacker*);
    void		geomSel(CallBacker*);

    bool		acceptOK(CallBacker*) override;

};

