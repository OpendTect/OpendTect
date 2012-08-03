#ifndef uiiosurface_h
#define uiiosurface_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          July 2003
 RCS:           $Id: uiiosurface.h,v 1.38 2012-08-03 13:00:57 cvskris Exp $
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "horsampling.h"
#include "uigroup.h"

class BufferStringSet;
class CtxtIOObj;
class IODirEntryList;
class IOObj;
class MultiID;

class uiPosSubSel;
class uiColorInput;
class uiGenInput;
class uiIOObjSel;
class uiLabeledListBox;
class uiCheckBox;
class uiStratLevelSel;


namespace EM { class Surface; class SurfaceIODataSelection; };


/*! \brief Base group for Surface input and output */

mClass(uiEarthModel) uiIOSurface : public uiGroup
{
public:
			~uiIOSurface();

    IOObj*		selIOObj() const;
    void		getSelection(EM::SurfaceIODataSelection&) const;
    void		getSelAttributes( BufferStringSet&) const;
    void		setInput(const MultiID&) const;
    void		setSelAttributes(const BufferStringSet&) const;

    virtual bool	processInput()		{ return true; };

    Notifier<uiIOSurface> attrSelChange;
    bool		haveAttrSel() const;
    uiIOObjSel*		getObjSel()		{ return objfld_; }
    uiPosSubSel*	getPosSubSel()		{ return rgfld_; }

protected:
			uiIOSurface(uiParent*,bool forread,
				    const char* type);

    bool		fillFields(const MultiID&,bool showerrmsg=true);
    void		fillSectionFld(const BufferStringSet&);
    void		fillAttribFld(const BufferStringSet&);
    void		fillRangeFld(const HorSampling&);

    void		mkAttribFld();
    void		mkSectionFld(bool);
    void		mkRangeFld(bool multiss=false);
    void		mkObjFld(const char*);

    void		objSel(CallBacker*);
    void		attrSel(CallBacker*);
    virtual void	ioDataSelChg(CallBacker*)			{};

    uiLabeledListBox*	sectionfld_;
    uiLabeledListBox*	attribfld_;
    uiPosSubSel*	rgfld_;
    uiIOObjSel*		objfld_;

    CtxtIOObj*		ctio_;
    bool		forread_;

    virtual void	inpChanged()		{}
};


mClass(uiEarthModel) uiSurfaceWrite : public uiIOSurface
{
public:

    mClass(uiEarthModel) Setup
    {
    public:
			Setup( const char* surftyp )
			    : typ_(surftyp)
			    , withsubsel_(false)
			    , withcolorfld_(false)
			    , withstratfld_(false)
			    , withdisplayfld_(false)
			    , displaytext_("Replace in tree")
			{}

	mDefSetupMemb(BufferString,typ)
	mDefSetupMemb(bool,withsubsel)
	mDefSetupMemb(bool,withcolorfld)
	mDefSetupMemb(bool,withstratfld)
	mDefSetupMemb(bool,withdisplayfld)
	mDefSetupMemb(BufferString,displaytext)
    };

			uiSurfaceWrite(uiParent*,const EM::Surface&,
				       const uiSurfaceWrite::Setup& setup);
			uiSurfaceWrite(uiParent*,
				       const uiSurfaceWrite::Setup& setup);

    virtual bool	processInput();
    int			getStratLevelID() const;
    void		setColor(const Color&);
    Color		getColor() const;
    bool		replaceInTree()	const;

protected:
    void		stratLvlChg(CallBacker*);
    void 		ioDataSelChg(CallBacker*);

    uiCheckBox*		displayfld_;
    uiColorInput*       colbut_;
    uiStratLevelSel*    stratlvlfld_;
    HorSampling		surfrange_;
};


mClass(uiEarthModel) uiSurfaceRead : public uiIOSurface
{
public:
    mClass(uiEarthModel) Setup
    {
    public:
			Setup( const char* surftyp )
			    : typ_(surftyp)
			    , withattribfld_(true)
			    , withsectionfld_(true)
			    , withsubsel_(false)
			    , multisubsel_(false)
			    , multiattribsel_(true)
			{}

	mDefSetupMemb(BufferString,typ)
	mDefSetupMemb(bool,withattribfld)
	mDefSetupMemb(bool,withsectionfld)
	mDefSetupMemb(bool,withsubsel)
	mDefSetupMemb(bool,multisubsel)
	mDefSetupMemb(bool,multiattribsel)
    };

    			uiSurfaceRead(uiParent*,const Setup&);

    virtual bool	processInput();
    void		setIOObj(const MultiID&);

    Notifier<uiIOSurface> inpChange;

protected:

    void		inpChanged()	{ inpChange.trigger(); }

};


#endif

