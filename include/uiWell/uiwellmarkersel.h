#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellmod.h"

#include "uidialog.h"
#include "uigroup.h"
#include "uistring.h"

#include "bufstringset.h"


class uiComboBox;
class uiIOObjSelGrp;
class uiListBox;
namespace Well { class Marker; class MarkerSet; }


/*!\brief Select one or two markers (i.e. a range) */

mExpClass(uiWell) uiWellMarkerSel : public uiGroup
{ mODTextTranslationClass(uiWellMarkerSel)
public:

    mExpClass(uiWell) Setup
    { mODTextTranslationClass(Setup)
    public:
			Setup(bool single,const uiString& =uiString::empty());
				// Pass an empty string to get no label
			mDeprecated("Use uiString")
			Setup( bool single, const char* txt )
			  : Setup(single,::toUiString(txt)) {}
	virtual		~Setup()	{}

	mDefSetupMemb(bool,single);	//!< false => two levels (a zone)
	mDefSetupMemb(bool,allowsame);	//!< [true]
	mDefSetupMemb(bool,withudf);	//!< [true] udf or 'open' zones allowed
	mDefSetupMemb(bool,unordered);	//!< [false] true if your markers are
					//!< not ordered top to bottom
	mDefSetupMemb(bool,middef);	//!< [false] set center markers(s) def
	mDefSetupMemb(uiString,seltxt);
    };

			uiWellMarkerSel(uiParent*,const Setup&);
			~uiWellMarkerSel();

    void		setMarkers(const Well::MarkerSet&);
    void		setMarkers(const BufferStringSet&);
    void		setMarkerColors(const TypeSet<OD::Color>&);

    void		setInput(const Well::Marker&,bool top=true);
    void		setInput(const char*,bool top=true);

    const char*		getText(bool top=true) const;
    BufferString	getMarkerName(bool top=true) const;

    enum		MarkerSelTyp { Undef, Start, End, Marker };
			mDeclareEnumUtils(MarkerSelTyp);

    MarkerSelTyp	getMkType(bool top=true) const;
    bool		isValidMarker(bool top=true) const;
    void		reset(); //!Sets start to first marker, stop to last

    uiRetVal		isOK() const;
    void		fillPar(IOPar&,bool replacestartend=false) const;
    void		usePar(const IOPar&);

    uiComboBox*		getFld( bool top )
			{ return top ? topfld_ : botfld_; }

    Notifier<uiWellMarkerSel> mrkSelDone;
				//!< only useful if setup.withudf
protected:

    const Setup		setup_;
    uiComboBox*		topfld_;
    uiComboBox*		botfld_ = nullptr;

    void		setMarkers(uiComboBox&,const BufferStringSet&);
    void		mrkSel(CallBacker*);

public:

    mDeprecated("Use MarkerSelTyp")
    int			getType(bool top=true) const;
				//!< -1=udf/before-first, 0=marker, 1=after-last

    mDeprecated("Use MarkerSelTyp")
    static const char*	sKeyUdfLvl();
    mDeprecated("Use MarkerSelTyp")
    static const char*	sKeyDataStart();
    mDeprecated("Use MarkerSelTyp")
    static const char*	sKeyDataEnd();

};


/*!\brief Select a list of markers from the well database */

mExpClass(uiWell) uiWellMarkersDlg : public uiDialog
{ mODTextTranslationClass(uiWellMarkersDlg);
public:

    mExpClass(uiWell) Setup
    {
    public:
			Setup( OD::ChoiceMode markerscm=OD::ChooseAtLeastOne,
			       bool withwllfilt=true,
			       OD::ChoiceMode wellscm=OD::ChooseAtLeastOne )
			  : markerschoicemode_(markerscm)
			  , withwellfilter_(withwllfilt)
			  , wellschoicemode_(wellscm)	{}
	virtual		~Setup()	{}

	mDefSetupMemb(OD::ChoiceMode,markerschoicemode);
	mDefSetupMemb(bool,withwellfilter);
	mDefSetupMemb(OD::ChoiceMode,wellschoicemode);

    };

			uiWellMarkersDlg(uiParent*,
					 const uiWellMarkersDlg::Setup&);
			~uiWellMarkersDlg();

			//Available after dlg.go():
    void		getNames(BufferStringSet&);
    void		getWellNames(BufferStringSet&);
    void		getWellIDs(TypeSet<MultiID>&);

    uiIOObjSelGrp*	wellSelGrp()		{ return wellselgrp_; }

private:

    uiIOObjSelGrp*	wellselgrp_;
    uiListBox*		markersselgrp_;

    void		fltrMarkerNamesCB(CallBacker*);
};
