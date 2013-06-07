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

#include "uigroup.h"
#include "bufstringset.h"
#include "survinfo.h"
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

mClass uiWellExtractParams : public uiGroup
{
public:
    mClass Setup
    {
	public:
				Setup()
				    : withzstep_(false)
				    , withzintime_(false)
				    , withzvalsel_(true)
				    , withsampling_(false)
				    , txtofmainfld_("Extract between")	  
				    , withextractintime_(false)
				    , singlelog_(false)
				    {}

	mDefSetupMemb(BufferString,txtofmainfld) 
	mDefSetupMemb(bool,withzstep) 
	mDefSetupMemb(bool,withzintime) 
	mDefSetupMemb(bool,withzvalsel) 
	mDefSetupMemb(bool,withsampling) 
	mDefSetupMemb(bool,withextractintime) 
	mDefSetupMemb(bool,singlelog) 
	mDefSetupMemb(BufferString,prefpropnm) 
    };

			uiWellExtractParams(uiParent*,const Setup&);

    void		clear();

    void		addMarkers(const Well::MarkerSet&);
    void		addMarkers(const BufferStringSet&);

    const Well::ExtractParams& params() { return params_; }
    void		setRange(Interval<float>,bool istime);

protected:

    BufferStringSet	markernms_;
    Well::ExtractParams	params_;	

    uiGenInput*		abovefld_;
    uiGenInput*		belowfld_;
    uiGenInput*		stepfld_;
    uiCheckBox*		zistimefld_;

    ObjectSet<uiGenInput> zselectionflds_;
    ObjectSet<uiLabel> 	zlabelflds_;
    uiGenInput*         zchoicefld_;

    uiGroup*		attach_;
    uiGenInput*		sampfld_;

    int			selidx_;

    void		putToScreen();
    void		getFromScreen(CallBacker*);
    void		extrInTimeCB(CallBacker*);

public:
    void                setRangeSel( const Well::ExtractParams& sel )
			{ params_ = sel;  putToScreen(); }

    const Well::ExtractParams& zRangeSel() { return params_; } //Legacy,4.3 only
    void		setTopMarker(const char* nm,float off)
			{ params_.setTopMarker( nm,off ); putToScreen(); }
    void		setBotMarker(const char* nm,float off)
			{ params_.setBotMarker( nm,off ); putToScreen(); }

protected:
    void		updateDisplayFlds();
    bool		singlelog_;
    BufferString	prefpropnm_;

public:
    void		setMarkers(const Well::MarkerSet& mrks) 
    			{ addMarkers( mrks ); }
    void		setMarkers(const BufferStringSet& mrks)
    			{ addMarkers( mrks ); }
};

typedef uiWellExtractParams uiWellZRangeSelector;

mClass uiMultiWellLogSel : public uiWellExtractParams
{
public:
			uiMultiWellLogSel(uiParent*,const Setup&);
			~uiMultiWellLogSel();

    void		getSelLogNames(BufferStringSet&) const;
    void		getSelWellNames(BufferStringSet&) const;
    void		getSelWellIDs(BufferStringSet&) const;

    void		update(); //call this when data changed

    void		setSingleLogSel(bool yn);

protected :

    ObjectSet<IOObj>	wellobjs_;
    TypeSet<MultiID>	wellids_;

    uiListBox*		wellsfld_;
    uiListBox*		logsfld_;
    uiLabeledListBox*	welllslblfld_;

    void		onFinalise( CallBacker* );
};

#endif
