#ifndef uicreatepicks_h
#define uicreatepicks_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        R. K. Singh
 Date:          Aug 2007
 RCS:           $Id: uicreatepicks.h,v 1.6 2008-12-02 13:58:33 cvsbert Exp $
________________________________________________________________________

-*/

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

class RandLocGenPars
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


class uiCreatePicks : public uiDialog 
{
public:
			uiCreatePicks(uiParent*);
			~uiCreatePicks() {}

    virtual Pick::Set*	getPickSet() const;	//!< Set is yours

protected:

    uiGenInput*		nmfld_;
    uiColorInput*       colsel_;
    BufferString	name_;

    virtual bool	acceptOK(CallBacker*);
};


class uiGenPosPicks : public uiCreatePicks
{
public:
    			uiGenPosPicks(uiParent*,bool is2d); //!<TODO: 2D support
			~uiGenPosPicks();

    virtual Pick::Set*	getPickSet() const;

protected:

    uiPosProvider*	posprovfld_;
    uiPosFilterSetSel*	posfiltfld_;
    DataPointSet*	dps_;

    bool		acceptOK(CallBacker*);
};


class uiGenRandPicks : public uiCreatePicks
{
public:
    const RandLocGenPars& randPars() const      { return randpars_; }
    virtual void        geomSel(CallBacker*)                 =0;

protected:
    			uiGenRandPicks(uiParent*,const BufferStringSet&);

    RandLocGenPars              randpars_;
    const BufferStringSet&      hornms_;

    uiGenInput*		nrfld_;
    uiGenInput*		geomfld_;
    uiLabeledComboBox*	horselfld_;
    uiComboBox*		horsel2fld_;

    void                hor1Sel(CallBacker*);
    void                hor2Sel(CallBacker*);
    void		horSel(uiComboBox*,uiComboBox*);

};


class uiGenRandPicks3D : public uiGenRandPicks 
{
public:
			uiGenRandPicks3D(uiParent*,const BufferStringSet&);

    void                geomSel(CallBacker*);

protected:

    uiPosSubSel*	volsubselfld_;
    uiPosSubSel*	horsubselfld_;

    bool		acceptOK(CallBacker*);
    void		mkRandPars();

};


class uiGenRandPicks2D : public uiGenRandPicks 
{
public:
    			uiGenRandPicks2D(uiParent*,const BufferStringSet&,
					const BufferStringSet&,
					const TypeSet<BufferStringSet>&);

    void                geomSel(CallBacker*);

protected:
    
    uiGenInput*		linesetfld_;
    uiLabeledListBox*	linenmfld_;
    uiGenInput*		zfld_;

    TypeSet<BufferStringSet>	linenms_;

    void		lineSetSel(CallBacker*);
    bool                acceptOK(CallBacker*);
    void                mkRandPars();
};

#endif
