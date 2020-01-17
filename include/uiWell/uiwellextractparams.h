#pragma once
/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Prajjaval Singh
Date:          Oct 2016
________________________________________________________________________

-*/
#include "uiwellmod.h"
#include "uigroup.h"
#include "bufstringset.h"
#include "wellmarker.h"
#include "wellextractdata.h"
#include "uistring.h"

class IOObj;
class uiCheckBox;
class uiGenInput;
class uiLabel;
class uiListBox;
class uiListBoxChoiceIO;

namespace Well { class MarkerSet; }

/*! brief: UI facilities to extract well data with zrg and extraction methods!*/

mExpClass(uiWell) uiWellZRangeSelector : public uiGroup
{ mODTextTranslationClass(uiWellZRangeSelector);
public:
    mExpClass(uiWell) Setup
    {
	public:
				Setup()
				    : withzintime_(true)
				    , withzvalsel_(true)
				    , txtofmainfld_("Extract Between")
				    {}

	mDefSetupMemb(bool,withzintime)
	mDefSetupMemb(bool,withzvalsel)
	mDefSetupMemb(BufferString,txtofmainfld)
    };

			uiWellZRangeSelector(uiParent*,const Setup&);
    virtual		~uiWellZRangeSelector();

    virtual void	clear();

    void		fillWithAllMarkers();
    void		setMarkers(const Well::MarkerSet&);
    void		setMarkers(const BufferStringSet&,
				   const TypeSet<Color>&);

    void		setTopMarker(const char* mrk,float shift)
			{ params_->setTopMarker(mrk,shift); putToScreen(); }
    void		setBotMarker(const char* mrk,float shift)
			{ params_->setBotMarker(mrk,shift); putToScreen(); }
    void		setRangeSel(const Well::ZRangeSelector&);

    void		setRange(Interval<float> rg, bool istime);

    Well::ZRangeSelector& zRangeSel() { return *params_; }

protected:

    uiGenInput*		abovefld_;
    uiGenInput*		belowfld_;

    ObjectSet<uiGroup>	zselectionflds_;
    ObjectSet<uiLabel>	zlabelflds_;
    uiGenInput*         zchoicefld_;

    int			selidx_;
    float		ztimefac_;
    Well::ZRangeSelector* params_;

    virtual void	putToScreen();
    virtual void	getFromScreen(CallBacker*);
    virtual void	updateDisplayFlds();
    virtual void	onFinalise(CallBacker*);
};


mExpClass(uiWell) uiWellExtractParams : public uiWellZRangeSelector
{ mODTextTranslationClass(uiWellExtractParams);
public:
    mExpClass(uiWell) Setup : public uiWellZRangeSelector::Setup
    {
	public:
				Setup()
				    : withzrgselparams_(false)
				    , withzstep_(false)
				    , defmeterstep_(1)
				    , withsampling_(false)
				    , withextractintime_(SI().zIsTime())
				    {
					if ( SI().depthsInFeet() )
					    defmeterstep_ =
						0.5f*mFromFeetFactorF;
				    }

	mDefSetupMemb(bool,withzrgselparams)
	mDefSetupMemb(bool,withzstep)
	mDefSetupMemb(bool,withsampling)
	mDefSetupMemb(bool,withextractintime)
	mDefSetupMemb(float,defmeterstep)
	mDefSetupMemb(BufferString,prefpropnm)
    };

			uiWellExtractParams(uiParent*,const Setup&);

    Well::ExtractParams& params()
			{ return static_cast<Well::ExtractParams&>(*params_); }

protected:

    bool		dostep_;
    bool		singlelog_;
    BufferString	prefpropnm_;

    uiGenInput*		depthstepfld_;
    uiGenInput*		timestepfld_;
    uiCheckBox*		zistimefld_;
    uiGenInput*		sampfld_;

    virtual void	updateDisplayFlds();
    virtual void	putToScreen();
    virtual void	getFromScreen(CallBacker*);
    virtual void	onFinalise(CallBacker*);
};
