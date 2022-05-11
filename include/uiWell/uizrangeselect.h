#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		May 2022
________________________________________________________________________

-*/
#include "uiwellmod.h"
#include "uigroup.h"
#include "wellextractdata.h"

/*! Field that takes care of getting a time, depth or marker range. */

namespace Well { class ZRangeSelector; }
class uiGenInput;
class uiWellMarkerSel;
class uiZRangeInput;


mExpClass(uiWell) uiZRangeSelect : public uiGroup
{ mODTextTranslationClass(uiZRangeSelect)
public:
				uiZRangeSelect(uiParent*,
					const uiString& lbl=tr("Show Between"),
					bool zintwt=false);
				~uiZRangeSelect();

    void			setMarkers(const BufferStringSet&);
    void			reset();

    Well::ZRangeSelector	zRangeSel();
    void			setZinTime(bool yn=false);
    void			setSensitive(bool choice_yn, bool markersel_yn,
					     bool offset_yn, bool zrange_yn);
    void			setSelectedMarkers(const char*, const char*);
    bool			inMarkerMode() const;

protected:
    uiGenInput*			zchoicefld_;
    uiWellMarkerSel*		markersel_;
    uiZRangeInput*		markeroffsetfld_;
    uiZRangeInput*		zrangefld_;
    bool			zintwt_;

    void			updateDisplayCB(CallBacker*);
    void			updateLabels();

    BufferStringSet		getZChoiceStrings() const;

};
