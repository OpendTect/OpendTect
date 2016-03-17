#ifndef uicreatepicks_h
#define uicreatepicks_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        R. K. Singh
 Date:          Aug 2007
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"
#include "trckeyzsampling.h"
#include "bufstringset.h"

class uiComboBox;
class uiColorInput;
class uiPosSubSel;
class uiGenInput;
class uiLabeledComboBox;
class uiListBox;
class uiPosProvider;
class uiPosFilterSetSel;
class DataPointSet;
namespace Pick { class Set; }

/*! \brief Dialog for creating (a) pick set(s) */

mExpClass(uiIo) RandLocGenPars
{ mODTextTranslationClass(RandLocGenPars);
public:

			RandLocGenPars()
			    : nr_(1), needhor_(false)
			    , horidx_(-1), horidx2_(-1)	{}

    int			nr_;
    bool		needhor_;
    TrcKeySampling		hs_;
    Interval<float>	zrg_;
    int			horidx_;
    int			horidx2_;
    BufferStringSet	linenms_;
};


mExpClass(uiIo) uiCreatePicks : public uiDialog
{ mODTextTranslationClass(uiCreatePicks);
public:
			uiCreatePicks(uiParent*,bool aspolygon=false,
				      bool addstdfields=true);
			~uiCreatePicks() {}

    virtual Pick::Set*	getPickSet() const;	//!< Set is yours

protected:

    uiGenInput*		nmfld_;
    uiColorInput*	colsel_;
    BufferString	name_;

    bool		aspolygon_;

    virtual bool	acceptOK(CallBacker*);
    virtual void	addStdFields(uiObject* lastobj=0);
};


mExpClass(uiIo) uiGenPosPicks : public uiCreatePicks
{ mODTextTranslationClass(uiGenPosPicks);
public:
    			uiGenPosPicks(uiParent*);
			~uiGenPosPicks();

    virtual Pick::Set*	getPickSet() const;

protected:

    uiPosProvider*	posprovfld_;
    uiGenInput*		maxnrpickfld_;
    uiPosFilterSetSel*	posfiltfld_;
    DataPointSet*	dps_;

    bool		acceptOK(CallBacker*);
};


mExpClass(uiIo) uiGenRandPicks2D : public uiCreatePicks
{ mODTextTranslationClass(uiGenRandPicks2D);
public:

    			uiGenRandPicks2D(uiParent*,const BufferStringSet&,
					 const BufferStringSet&);

    const RandLocGenPars& randPars() const	{ return randpars_; }

protected:

    RandLocGenPars		randpars_;
    const BufferStringSet&	hornms_;

    uiGenInput*		nrfld_;
    uiGenInput*		geomfld_;
    uiLabeledComboBox*	horselfld_;
    uiComboBox*		horsel2fld_;
    uiListBox*		linenmfld_;
    uiGenInput*		zfld_;

    BufferStringSet	linenms_;

    bool		acceptOK(CallBacker*);
    void		mkRandPars();

    void		geomSel(CallBacker*);
    void		hor1Sel(CallBacker*);
    void		hor2Sel(CallBacker*);
    void		horSel(uiComboBox*,uiComboBox*);

};


#endif
