#ifndef uicreatepicks_h
#define uicreatepicks_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        R. K. Singh
 Date:          Aug 2007
 RCS:           $Id: uicreatepicks.h,v 1.2 2007-08-21 05:35:26 cvsraman Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "binidselimpl.h"
#include "bufstringset.h"

class Color;
class IOObj;
class uiGroup;
class uiComboBox;
class uiColorInput;
class uiBinIDSubSel;
class uiGenInput;
class uiLabeledComboBox;
class uiLabeledListBox;
class uiSeis2DSubSel;

/*! \brief Dialog for creating (a) pick set(s) */

class RandLocGenPars
{
public:

			RandLocGenPars()
			    : nr_(1), iscube_(true)
			    , horidx_(-1), horidx2_(-1)	{}


    int			nr_;
    bool		iscube_;
    BinIDRange		bidrg_;
    Interval<float>	zrg_;
    int			horidx_;
    int			horidx2_;
    int			lsetidx_;
    BufferStringSet	linenms_;
};


class uiCreatePicks : public uiDialog 
{
public:
			~uiCreatePicks() {}

    const char*		getName() const;
    const Color&	getPickColor();
    bool		genRand() const;
    const RandLocGenPars& randPars() const	{ return randpars_; }
    IOObj*		storObj() const;
    			//!< If non-null, user wants to store the new set

    void		randSel(CallBacker*);
    virtual void	randHorSel(CallBacker*)			=0;

protected:
			uiCreatePicks(uiParent*,const BufferStringSet&);

    RandLocGenPars		randpars_;
    const BufferStringSet&	hornms_;

    uiGenInput*		nmfld_;
    uiGenInput*		randfld_;
    uiGenInput*		randnrfld_;
    uiGenInput*		randvolfld_;
    uiColorInput*	colsel_;
    uiLabeledComboBox*	randhorselfld_;
    uiComboBox*		randhorsel2fld_;
    uiGroup*		randgrp_;

    void                hor1Sel(CallBacker*);
    void                hor2Sel(CallBacker*);
    void		horSel(uiComboBox*,uiComboBox*);

    virtual void	mkRandPars()				=0;

};


class uiCreatePicks3D : public uiCreatePicks 
{
public:
			uiCreatePicks3D(uiParent*,const BufferStringSet&);

    void                randHorSel(CallBacker*);

protected:

    uiBinIDSubSel*	randvolsubselfld_;
    uiBinIDSubSel*	randhorsubselfld_;

    bool		acceptOK(CallBacker*);
    void		mkRandPars();

};


class uiCreatePicks2D : public uiCreatePicks
{
public:
    			uiCreatePicks2D(uiParent*,const BufferStringSet&,
					const BufferStringSet&,
					const TypeSet<BufferStringSet>&);

    void                randHorSel(CallBacker*);

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
