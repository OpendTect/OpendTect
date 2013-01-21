#ifndef uicreatepicks_h
#define uicreatepicks_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        R. K. Singh
 Date:          Aug 2007
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"
#include "cubesampling.h"
#include "bufstringset.h"

class Color;
class IOObj;
class uiGroup;
class uiComboBox;
class uiColorInput;
class uiPosSubSel;
class uiGenInput;
class uiLabeledComboBox;
class uiLabeledListBox;
class uiSeis2DSubSel;
class uiPosProvider;
class uiPosFilterSetSel;
class DataPointSet;
namespace Pick { class Set; }

/*! \brief Dialog for creating (a) pick set(s) */

mExpClass(uiIo) RandLocGenPars
{
public:

			RandLocGenPars()
			    : nr_(1), needhor_(false)
			    , horidx_(-1), horidx2_(-1)	{}

    int			nr_;
    bool		needhor_;
    HorSampling		hs_;
    Interval<float>	zrg_;
    int			horidx_;
    int			horidx2_;
    int			lsetidx_;
    BufferStringSet	linenms_;
};


mExpClass(uiIo) uiCreatePicks : public uiDialog 
{
public:
			uiCreatePicks(uiParent*,bool aspolygon=false);
			~uiCreatePicks() {}

    virtual Pick::Set*	getPickSet() const;	//!< Set is yours

protected:

    uiGenInput*		nmfld_;
    uiColorInput*       colsel_;
    BufferString	name_;

    bool		aspolygon_;

    virtual bool	acceptOK(CallBacker*);
};


mExpClass(uiIo) uiGenPosPicks : public uiCreatePicks
{
public:
    			uiGenPosPicks(uiParent*);
			~uiGenPosPicks();

    virtual Pick::Set*	getPickSet() const;

protected:

    uiPosProvider*	posprovfld_;
    uiPosFilterSetSel*	posfiltfld_;
    DataPointSet*	dps_;

    bool		acceptOK(CallBacker*);
};


mExpClass(uiIo) uiGenRandPicks2D : public uiCreatePicks
{
public:

    			uiGenRandPicks2D(uiParent*,const BufferStringSet&,
					const BufferStringSet&,
					const TypeSet<BufferStringSet>&);

    const RandLocGenPars& randPars() const      { return randpars_; }

protected:

    RandLocGenPars              randpars_;
    const BufferStringSet&      hornms_;

    uiGenInput*		nrfld_;
    uiGenInput*		geomfld_;
    uiLabeledComboBox*	horselfld_;
    uiComboBox*		horsel2fld_;
    uiGenInput*		linesetfld_;
    uiLabeledListBox*	linenmfld_;
    uiGenInput*		zfld_;

    TypeSet<BufferStringSet>	linenms_;

    void		lineSetSel(CallBacker*);
    bool                acceptOK(CallBacker*);
    void                mkRandPars();

    void                geomSel(CallBacker*);
    void                hor1Sel(CallBacker*);
    void                hor2Sel(CallBacker*);
    void		horSel(uiComboBox*,uiComboBox*);

};


#endif

