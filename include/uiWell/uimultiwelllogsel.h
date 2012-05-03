#ifndef uimultiwelllogsel_h
#define uimultiwelllogsel_h
/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Jan 2011
RCS:           $Id: uimultiwelllogsel.h,v 1.13 2012-05-03 07:30:08 cvsbruno Exp $
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
    Well::ZRangeSelector* params_;	

    void		putToScreen();
    void		getFromScreen(CallBacker*);
    void		updateDisplayFlds();
};


mClass uiWellExtractParams : public uiWellZRangeSelector
{
public:
    mClass Setup : public uiWellZRangeSelector::Setup
    {
	public:
				Setup()
				    : withzstep_(false)
				    , withsampling_(false)
				    , withextractintime_(SI().zIsTime())
				    {}

	mDefSetupMemb(bool,withzstep) 
	mDefSetupMemb(bool,withsampling) 
	mDefSetupMemb(bool,withextractintime) 
    };

			uiWellExtractParams(uiParent*,const Setup&);

    Well::ExtractParams& params() 
    			{ return static_cast<Well::ExtractParams&>(*params_); }

protected:


    uiGenInput*		stepfld_;
    uiCheckBox*		zistimefld_;
    uiGenInput*		sampfld_;

    virtual void	putToScreen();
    virtual void	getFromScreen(CallBacker*);
    void 		extrInTimeCB(CallBacker*);
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
