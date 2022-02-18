#pragma once
/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Jan 2011
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

    void		setMarkers(const BufferStringSet&);
    void		setMarkers(const Well::MarkerSet&);

    void		setTopMarker(const char* mrk,float shift)
			{ params_->setTopMarker(mrk,shift); putToScreen(); }
    void		setBotMarker(const char* mrk,float shift)
			{ params_->setBotMarker(mrk,shift); putToScreen(); }
    void		setRangeSel( const Well::ZRangeSelector& sel )
			{ *params_ = sel;  putToScreen(); }
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
				    : withzstep_(false)
				    , defmeterstep_(1)
				    , withsampling_(false)
				    , singlelog_(false)
				    , withextractintime_(SI().zIsTime())
				    { }

	mDefSetupMemb(bool,withzstep)
	mDefSetupMemb(bool,withsampling)
	mDefSetupMemb(bool,withextractintime)
	mDefSetupMemb(float,defmeterstep)
	mDefSetupMemb(bool,singlelog)
	mDefSetupMemb(BufferString,prefpropnm)
    };

			uiWellExtractParams(uiParent*,const Setup&);

    Well::ExtractParams& params()
			{ return static_cast<Well::ExtractParams&>(*params_); }
     void		setWellExtractParams(Well::ExtractParams param)
			{
			    params() = param;
			    putToScreen();
			}


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



mExpClass(uiWell) uiMultiWellLogSel : public uiWellExtractParams
{ mODTextTranslationClass(uiMultiWellLogSel);
public:
			uiMultiWellLogSel(uiParent*,const Setup&,
					  const BufferStringSet* wellnms=0,
					  const BufferStringSet* lognms=0);
			uiMultiWellLogSel(uiParent*,const Setup&,
					const MultiID& singlewid);
			~uiMultiWellLogSel();

    void		selectOnlyWritableWells();

    void		getSelLogNames(BufferStringSet&) const;
    void		getSelWellNames(BufferStringSet&) const;
    void		getSelWellIDs(TypeSet<MultiID>&) const;

    void		setSelLogNames(const BufferStringSet&);
    void		setSelWellNames(const BufferStringSet&);
    void		setSelWellIDs(const TypeSet<MultiID>&);

    void		update(); //call this when data changed

    mDeprecated("Use setSelWellIDs(const TypeSet<MultiID>&)")
    void		setSelWellIDs(const BufferStringSet&);
    mDeprecated("Use getSelWellIDs(TypeSet<MultiID>&)")
    void		getSelWellIDs(BufferStringSet&) const;

protected:

    ObjectSet<IOObj>	wellobjs_;

    const MultiID*	singlewid_;

    uiListBox*		wellsfld_;
    uiListBox*		logsfld_;
    uiListBoxChoiceIO*	wellschoiceio_;

    void		init();
    void		onFinalise(CallBacker*);

    void		readWellChoiceDone(CallBacker*);
    void		writeWellChoiceReq(CallBacker*);
    void		updateLogsFldCB(CallBacker*);
};

