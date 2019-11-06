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
#include "uidialog.h"

#include "bufstringset.h"
#include "faulttrace.h"
#include "trckeysampling.h"
#include "posinfo2dsurv.h"
#include "emposid.h"

class CtxtIOObj;
class IODirEntryList;
class IOObj;

class uiCheckBox;
class uiColorInput;
class uiFaultOptSel;
class uiIOObjSel;
class uiListBox;
class uiPosSubSel;
class uiStratLevelSel;
class uiPushButton;

namespace EM { class Surface; class SurfaceIODataSelection; }


/*! \brief Base group for Surface input and output */

mExpClass(uiEarthModel) uiIOSurface : public uiGroup
{ mODTextTranslationClass(uiIOSurface)
public:
			~uiIOSurface();

    const IOObj*	selIOObj() const;
    void		getSelection(EM::SurfaceIODataSelection&) const;
    void		getSelAttributes( BufferStringSet&) const;
    void		setInput(const MultiID&) const;
    void		setSelAttributes(const BufferStringSet&) const;

    virtual bool	processInput()		{ return true; }

    Notifier<uiIOSurface> attrSelChange;
    bool		haveAttrSel() const;
    uiIOObjSel*		getObjSel()		{ return objfld_; }
    uiPosSubSel*	getPosSubSel()		{ return rgfld_; }

protected:
			uiIOSurface(uiParent*,bool forread,
				    const char* type);

    bool		fillFields(const MultiID&,bool showerrmsg=true);
    void		fillFields(const EM::ObjectID&);
    void		fillSectionFld(const BufferStringSet&);
    void		fillAttribFld(const BufferStringSet&);
    void		fillRangeFld(const TrcKeySampling&);

    void		mkAttribFld(bool);
    void		mkSectionFld(bool);
    void		mkRangeFld(bool multiss=false);
    void		mkObjFld(const uiString&);

    void		objSel(CallBacker*);
    void		attrSel(CallBacker*);
    virtual void	ioDataSelChg(CallBacker*)			{}

    uiListBox*		sectionfld_;
    uiListBox*		attribfld_;
    uiPosSubSel*	rgfld_;
    uiIOObjSel*		objfld_;

    CtxtIOObj*		ctio_;
    bool		forread_;

    virtual void	inpChanged()		{}
};


mExpClass(uiEarthModel) uiSurfaceWrite : public uiIOSurface
{ mODTextTranslationClass(uiSurfaceWrite)
public:

    mExpClass(uiEarthModel) Setup
    {
    public:
			Setup( const char* surftyp, const uiString& type_name )
			    : typ_(surftyp)
			    , typname_( type_name )
			    , withsubsel_(false)
			    , withcolorfld_(false)
			    , withstratfld_(false)
			    , withdisplayfld_(false)
			    , displaytext_(tr("Replace in tree"))
			{}

	mDefSetupMemb(BufferString,typ)
	mDefSetupMemb(uiString,typname)
	mDefSetupMemb(bool,withsubsel)
	mDefSetupMemb(bool,withcolorfld)
	mDefSetupMemb(bool,withstratfld)
	mDefSetupMemb(bool,withdisplayfld)
	mDefSetupMemb(uiString,displaytext)
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

    uiCheckBox*		getDisplayFld()		{ return displayfld_; }

protected:
    void		stratLvlChg(CallBacker*);
    void		ioDataSelChg(CallBacker*);

    uiCheckBox*		displayfld_;
    uiColorInput*       colbut_;
    uiStratLevelSel*    stratlvlfld_;
    TrcKeySampling		surfrange_;
};


mExpClass(uiEarthModel) uiSurfaceRead : public uiIOSurface
{ mODTextTranslationClass(uiSurfaceRead)
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


mExpClass(uiEarthModel) uiHorizonParSel : public uiCompoundParSel
{ mODTextTranslationClass(uiHorizonParSel)
public:
				uiHorizonParSel(uiParent*,bool is2d,
						bool withclear=false);
				~uiHorizonParSel();

    void			setSelected(const TypeSet<MultiID>&);
    const TypeSet<MultiID>&	getSelected() const;

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

    BufferString		getSummary() const;

protected:

    void			clearPush(CallBacker*);
    void			doDlg(CallBacker*);

    bool			is2d_;
    TypeSet<MultiID>		selids_;
};


mExpClass(uiEarthModel) uiFaultParSel : public uiCompoundParSel
{ mODTextTranslationClass(uiFaultParSel)
public:
				uiFaultParSel(uiParent*,bool is2d,
					      bool use_act_option=false);
				~uiFaultParSel();

				/*Set my own options on selected, optional*/
    void			setActOptions(const BufferStringSet&,
					      int defaultoptidx=0);
    const TypeSet<int>&		getSelectedOptIndies() const { return optids_; }

    void			setSelectedFaults(const TypeSet<MultiID>&,
					const TypeSet<FaultTrace::Act>* =0);
    BufferString		getSummary() const;
    const TypeSet<MultiID>&	selFaultIDs() const { return selfaultids_; }

    void			setEmpty();
    void			setGeomIDs(const TypeSet<Pos::GeomID>&);
				/*<for FaultStickSet picked from 2D lines.*/

    void			hideClearButton(bool yn=true);
    void			updateOnSelChg(bool isfltset=false);

    Notifier<uiFaultParSel>	selChange;

protected:

    friend class		uiFaultOptSel;
    void			clearPush(CallBacker*);
    void			doDlg(CallBacker*);
    void			updateOnSelChgCB(CallBacker*);
    bool			is2d_;
    BufferStringSet		selfaultnms_;
    TypeSet<MultiID>		selfaultids_;
    TypeSet<Pos::GeomID>	geomids_;

    bool			useoptions_;
    BufferStringSet		optnms_;
    TypeSet<int>		optids_;
    int				defaultoptidx_;
    uiPushButton*		clearbut_;
};

#endif
