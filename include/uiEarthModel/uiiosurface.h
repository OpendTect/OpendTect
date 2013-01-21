#ifndef uiiosurface_h
#define uiiosurface_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          July 2003
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uicompoundparsel.h"
#include "uigroup.h"

#include "bufstringset.h"
#include "horsampling.h"
#include "surv2dgeom.h"

class CtxtIOObj;
class IODirEntryList;
class IOObj;
class MultiID;

class uiCheckBox;
class uiColorInput;
class uiGenInput;
class uiIOObjSel;
class uiLabeledListBox;
class uiPosSubSel;
class uiStratLevelSel;

namespace EM { class Surface; class SurfaceIODataSelection; };


/*! \brief Base group for Surface input and output */

mExpClass(uiEarthModel) uiIOSurface : public uiGroup
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


mExpClass(uiEarthModel) uiSurfaceWrite : public uiIOSurface
{
public:

    mExpClass(uiEarthModel) Setup
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


mExpClass(uiEarthModel) uiSurfaceRead : public uiIOSurface
{
public:
    mExpClass(uiEarthModel) Setup
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


mExpClass(uiEarthModel) uiFaultParSel : public uiCompoundParSel
{
public:
				uiFaultParSel(uiParent*,bool);

    void			setSelectedFaults(const TypeSet<MultiID>&);
    BufferString		getSummary() const;
    const TypeSet<MultiID>&	selFaultIDs() const { return selfaultids_; }
					
    void			set2DGeomIds(const TypeSet<PosInfo::GeomID>&);
    				/*<for FaultStickSet picked from 2D lines.*/

    Notifier<uiFaultParSel>	selChange;

protected:

    void			clearPush(CallBacker*);
    void			doDlg(CallBacker*);

    bool			is2d_;
    BufferStringSet		selfaultnms_;
    TypeSet<MultiID>		selfaultids_;
    TypeSet<PosInfo::GeomID>	geomids_;
};

#endif
