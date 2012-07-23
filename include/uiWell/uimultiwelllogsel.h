#ifndef uimultiwelllogsel_h
#define uimultiwelllogsel_h
/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Jan 2011
RCS:           $Id: uimultiwelllogsel.h,v 1.15 2012-07-23 09:32:25 cvssatyaki Exp $
________________________________________________________________________

-*/

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

mClass uiWellZRangeSelector : public uiGroup
{
public:
    mClass Setup
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

    void		addMarkers(const Well::MarkerSet&);
    void		addMarkers(const BufferStringSet&);

    void		setTopMarker(const char* mrk,float shift)
			{ params_->setTopMarker(mrk,shift); putToScreen(); }
    void		setBotMarker(const char* mrk,float shift)
			{ params_->setBotMarker(mrk,shift); putToScreen(); }
    void		setRangeSel( const Well::ZRangeSelector& sel )
			{ *params_ = sel;  putToScreen(); }
    void		setRange(Interval<float> rg, bool istime);

    Well::ZRangeSelector& zRangeSel() { return *params_; }

protected:

    BufferStringSet	markernms_;
    uiGenInput*		abovefld_;
    uiGenInput*		belowfld_;

    ObjectSet<uiGenInput> zselectionflds_;
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


mClass uiWellExtractParams : public uiWellZRangeSelector
{
public:
    mClass Setup : public uiWellZRangeSelector::Setup
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
    };

			uiWellExtractParams(uiParent*,const Setup&);

    Well::ExtractParams& params() 
    			{ return static_cast<Well::ExtractParams&>(*params_); }

protected:

    bool		dostep_;
    bool		singlelog_;

    uiGenInput*		depthstepfld_;
    uiGenInput*		timestepfld_;
    uiCheckBox*		zistimefld_;
    uiGenInput*		sampfld_;

    virtual void	updateDisplayFlds();
    virtual void	putToScreen();
    virtual void	getFromScreen(CallBacker*);
    virtual void	onFinalise(CallBacker*);
};



mClass uiMultiWellLogSel : public uiWellExtractParams
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
