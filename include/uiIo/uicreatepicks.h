#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        R. K. Singh
 Date:          Aug 2007
________________________________________________________________________

-*/

#include "uiiocommon.h"
#include "uinewpickset.h"
#include "trckeysampling.h"
#include "bufstringset.h"

class uiComboBox;
class uiPosSubSel;
class uiLabeledComboBox;
class uiListBox;
class uiPosProvider;
class uiPosFilterSetSel;
class DataPointSet;

/*! \brief Pars for generating random locations */

mExpClass(uiIo) RandLocGenPars
{ mODTextTranslationClass(RandLocGenPars);
public:

			RandLocGenPars()
			    : nr_(1), needhor_(false)
			    , horidx_(-1), horidx2_(-1)	{}

    int			nr_;
    bool		needhor_;
    TrcKeySampling	hs_;
    Interval<float>	zrg_;
    int			horidx_;
    int			horidx2_;
    BufferStringSet	linenms_;
};


/*!\brief creates a Pick::Set with generated positions. */

mExpClass(uiIo) uiGenPosPicksDlg : public uiNewPickSetDlg
{ mODTextTranslationClass(uiGenPosPicksDlg);
public:

			uiGenPosPicksDlg(uiParent*);
			~uiGenPosPicksDlg();

protected:

    uiPosProvider*	posprovfld_;
    uiGenInput*		maxnrpickfld_;
    uiPosFilterSetSel*	posfiltfld_;
    uiGenInput*		posfiltmodefld_;

    virtual bool	fillData(Pick::Set&);
};


/*!\brief creates a Pick::Set with random 2D positions.

  Note that it will only make the RandLocGenPars, it will call you to fill the
  locations (i.e. setting fillLocs is mandatory).

*/

mExpClass(uiIo) uiGenRandPicks2DDlg : public uiNewPickSetDlg
{ mODTextTranslationClass(uiGenRandPicks2DDlg);
public:

			uiGenRandPicks2DDlg(uiParent*,const BufferStringSet&,
					    const BufferStringSet&);

    const RandLocGenPars& randPars() const	{ return randpars_; }

    Notifier<uiGenRandPicks2DDlg>   fillLocs;

protected:

    RandLocGenPars	randpars_;
    const BufferStringSet& hornms_;

    uiGenInput*		nrfld_;
    uiGenInput*		geomfld_;
    uiLabeledComboBox*	horselfld_;
    uiComboBox*		horsel2fld_;
    uiListBox*		linenmfld_;
    uiGenInput*		zfld_;

    BufferStringSet	linenms_;

    virtual bool	fillData(Pick::Set&);
    void		mkRandPars();

    void		geomSel(CallBacker*);
    void		hor1Sel(CallBacker*);
    void		hor2Sel(CallBacker*);
    void		horSel(uiComboBox*,uiComboBox*);

};
