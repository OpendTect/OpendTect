#ifndef uimultiwelllogsel_h
#define uimultiwelllogsel_h
/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Jan 2011
RCS:           $Id$
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uigroup.h"
#include "bufstringset.h"
#include "wellmarker.h"
#include "wellextractdata.h"

class IOObj;
class MultiID;
class uiCheckBox;
class uiComboBox;
class uiGenInput;
class uiLabel;
class uiListBox;
class uiLabeledListBox;

namespace Well { class MarkerSet; } 

/*! brief: UI facilities to extract well data with zrg and extraction methods!*/

mExpClass(uiWell) uiWellZRangeSelector : public uiGroup
{
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
			~uiWellZRangeSelector();

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
    ObjectSet<uiLabel> 	zlabelflds_;
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
{
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
				    {}

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
{
public:
			uiMultiWellLogSel(uiParent*,const Setup&);
			uiMultiWellLogSel(uiParent*,const Setup&,
					const MultiID& singlewid);
			~uiMultiWellLogSel();

    void		getSelLogNames(BufferStringSet&) const;
    void		getSelWellNames(BufferStringSet&) const;
    void		getSelWellIDs(BufferStringSet&) const;

    void		update(); //call this when data changed

protected:

    ObjectSet<IOObj>	wellobjs_;

    const MultiID*	singlewid_;

    uiListBox*		wellsfld_;
    uiListBox*		logsfld_;
    uiLabeledListBox*	welllslblfld_;

    void		init();
    void		onFinalise( CallBacker* );
};

#endif

