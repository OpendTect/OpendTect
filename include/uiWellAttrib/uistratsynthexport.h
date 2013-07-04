#ifndef uistratsynthexport_h
#define uistratsynthexport_h
/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki / Bert
 Date:          July 2013
 RCS:           $Id$
 _______________________________________________________________________

      -*/


#include "uiwellattribmod.h"
#include "uidialog.h"
class StratSynth;
class SyntheticData;
class StratSynthLevel;
class uiGroup;
class uiGenInput;
class uiSeisSel;
class uiIOObjSel;
class uiSeis2DLineNameSel;
class uiStratSynthOutSel;


mExpClass(uiWellAttrib) uiStratSynthExport : public uiDialog
{
public:
    			uiStratSynthExport(uiParent*,const StratSynth&);
			~uiStratSynthExport();


protected:

    uiGenInput*		crnewfld_;
    uiSeisSel*		linesetsel_;
    uiSeis2DLineNameSel* newlinenmsel_;
    uiSeis2DLineNameSel* existlinenmsel_;
    uiGroup*		geomgrp_;
    uiGenInput*		geomsel_;
    uiGenInput*		coord0fld_;
    uiGenInput*		coord1fld_;
    uiIOObjSel*		picksetsel_;
    uiIOObjSel*		randlinesel_;
    uiStratSynthOutSel*	poststcksel_;
    uiStratSynthOutSel*	horsel_;
    uiStratSynthOutSel*	prestcksel_;
    uiGenInput*		prefxfld_;
    uiGenInput*		postfxfld_;

    const StratSynth&	ss_;
    ObjectSet<const SyntheticData> postsds_;
    ObjectSet<const SyntheticData> presds_;
    ObjectSet<StratSynthLevel> sslvls_;

    BufferString	getWinTitle(const StratSynth&) const;
    void		fillGeomGroup();
    void		getExpObjs();

    void		crNewChg(CallBacker*);
    void		lsSel(CallBacker*);
    void		geomSel(CallBacker*);

    bool		acceptOK(CallBacker*);

};

#endif

