#ifndef uimultiwelllogsel_h
#define uimultiwelllogsel_h
/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Jan 2011
RCS:           $Id: uimultiwelllogsel.h,v 1.8 2012-03-29 07:15:25 cvsbruno Exp $
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

mClass uiWellExtractParams : public uiGroup
{
public:
    mClass Setup
    {
	public:
				Setup()
				    : withzstep_(false)
				    , withzintime_(true)
				    , withzvalsel_(true)
				    , withsampling_(false)
				    , withextractintime_(SI().zIsTime())
				    {}

	mDefSetupMemb(bool,withzstep) 
	mDefSetupMemb(bool,withzintime) 
	mDefSetupMemb(bool,withzvalsel) 
	mDefSetupMemb(bool,withsampling) 
	mDefSetupMemb(bool,withextractintime) 
    };

			uiWellExtractParams(uiParent*,const Setup&);

    void		clear();

    void		addMarkers(const Well::MarkerSet&);
    void		addMarkers(const BufferStringSet&);

    const Well::ExtractParams& params() { return params_; }

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
    void 		extrInTimeCB(CallBacker*);
};



mClass uiMultiWellLogSel : public uiWellExtractParams
{
public:
			uiMultiWellLogSel(uiParent*,const Setup&);
			~uiMultiWellLogSel();

    void		getSelLogNames(BufferStringSet&) const;
    void		getSelWellNames(BufferStringSet&) const;
    void		getSelWellIDs(BufferStringSet&) const;

    void		update(); //call this when data changed

protected :

    ObjectSet<IOObj>	wellobjs_;
    TypeSet<MultiID>	wellids_;

    uiListBox*		wellsfld_;
    uiListBox*		logsfld_;
    uiLabeledListBox*	welllslblfld_;

    void		onFinalise( CallBacker* );
};

#endif
