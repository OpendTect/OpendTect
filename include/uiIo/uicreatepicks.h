#ifndef uicreatepicks_h
#define uicreatepicks_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        R. K. Singh
 Date:          Aug 2007
 RCS:           $Id: uicreatepicks.h,v 1.1 2007-08-13 04:33:10 cvsraman Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "binidselimpl.h"
class Color;
class IOObj;
class uiGroup;
class uiGenInput;
class uiComboBox;
class uiColorInput;
class uiBinIDSubSel;
class BufferStringSet;
class uiLabeledComboBox;

/*! \brief Dialog for creating (a) pick set(s) */

class RandLocGenPars
{
public:

			RandLocGenPars()
			    : nr(1), iscube(true)
			    , horidx(-1), horidx2(-1)	{}


    int			nr;
    bool		iscube;
    BinIDRange		bidrg;
    Interval<float>	zrg;
    int			horidx;
    int			horidx2;

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

    virtual void	randHorSel(CallBacker*)			=0;
    virtual void	mkRandPars()				=0;

};


class uiCreatePicks3D : public uiCreatePicks 
{
public:
			uiCreatePicks3D(uiParent*,const BufferStringSet&);

protected:

    uiBinIDSubSel*	randvolsubselfld_;
    uiBinIDSubSel*	randhorsubselfld_;

    void		randHorSel(CallBacker*);
    bool		acceptOK(CallBacker*);
    void		mkRandPars();

};


#endif
