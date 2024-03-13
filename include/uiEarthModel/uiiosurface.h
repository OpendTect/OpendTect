#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"

#include "uicompoundparsel.h"
#include "uidialog.h"
#include "uigroup.h"
#include "uiioobjsel.h"
#include "uisurfaceman.h"

#include "bufstringset.h"
#include "emmanager.h"
#include "emposid.h"
#include "faulttrace.h"
#include "posinfo2dsurv.h"
#include "stratlevel.h"
#include "trckeysampling.h"


class CtxtIOObj;
class IODirEntryList;
class IOObj;

class uiCheckBox;
class uiColorInput;
class uiFaultOptSel;
class uiGenInput;
class uiIOSelect;
class uiListBox;
class uiPosSubSel;
class uiPushButton;
class uiStratLevelSel;

namespace EM { class Surface; class SurfaceIODataSelection; }
namespace ZDomain { class Info; }


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
			uiIOSurface(uiParent*,bool forread,const char* type,
						    const ZDomain::Info*);

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

    uiListBox*		sectionfld_	= nullptr;
    uiListBox*		attribfld_	= nullptr;
    uiPosSubSel*	rgfld_		= nullptr;
    uiIOObjSel*		objfld_		= nullptr;

    CtxtIOObj*		ctio_	= nullptr;
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
			~Setup()
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
				       const Setup&,
				       const ZDomain::Info* =nullptr);
			uiSurfaceWrite(uiParent*,const Setup&,
				       const ZDomain::Info* =nullptr);
			~uiSurfaceWrite();

    bool		processInput() override;
    Strat::LevelID	getStratLevelID() const;
    void		setColor(const OD::Color&);
    OD::Color		getColor() const;
    bool		replaceInTree()	const;

    uiCheckBox*		getDisplayFld()		{ return displayfld_; }

protected:
    void		stratLvlChg(CallBacker*);
    void		ioDataSelChg(CallBacker*) override;

    uiCheckBox*		displayfld_	= nullptr;
    uiColorInput*       colbut_		= nullptr;
    uiStratLevelSel*    stratlvlfld_	= nullptr;
    TrcKeySampling	surfrange_;
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
			~Setup()
			{}

	mDefSetupMemb(BufferString,typ)
	mDefSetupMemb(bool,withattribfld)
	mDefSetupMemb(bool,withsectionfld)
	mDefSetupMemb(bool,withsubsel)
	mDefSetupMemb(bool,multisubsel)
	mDefSetupMemb(bool,multiattribsel)
    };

			uiSurfaceRead(uiParent*,const Setup&,
			    const ZDomain::Info* =nullptr);
			~uiSurfaceRead();

    bool		processInput() override;
    void		setIOObj(const MultiID&);

    Notifier<uiIOSurface> inpChange;

protected:

    void		inpChanged() override	{ inpChange.trigger(); }

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

    BufferString		getSummary() const override;

protected:

    void			clearPush(CallBacker*);
    void			doDlg(CallBacker*);

    bool			is2d_;
    TypeSet<MultiID>		selids_;
};


mExpClass(uiEarthModel) uiFaultParSel : public uiCompoundParSel
{ mODTextTranslationClass(uiFaultParSel)
public:
				uiFaultParSel(uiParent*,
					      EM::ObjectType,
					      bool use_act_option=false);
				~uiFaultParSel();

    EM::ObjectType		type() const;

				/*Set my own options on selected, optional*/
    void			setActOptions(const BufferStringSet&,
					      int defaultoptidx=0);
    const TypeSet<int>&		getSelectedOptIndies() const { return optids_; }

    void			setSelectedFaults(const TypeSet<MultiID>&,
				    const TypeSet<FaultTrace::Act>* =nullptr);
    BufferString		getSummary() const override;
    const TypeSet<MultiID>&	selFaultIDs() const { return selfaultids_; }

    void			setEmpty();
    void			setGeomIDs(const TypeSet<Pos::GeomID>&);
				/*<for FaultStickSet picked from 2D lines.*/

    void			hideClearButton(bool yn=true);

    Notifier<uiFaultParSel>	selChange;

protected:

    friend class		uiFaultOptSel;
    void			clearPush(CallBacker*);
    void			doDlg(CallBacker*);
    EM::ObjectType		objtype_;
    BufferStringSet		selfaultnms_;
    TypeSet<MultiID>		selfaultids_;
    TypeSet<Pos::GeomID>	geomids_;

    bool			useoptions_;
    BufferStringSet		optnms_;
    TypeSet<int>		optids_;
    int				defaultoptidx_;
    uiPushButton*		clearbut_;
};


mExpClass(uiEarthModel) uiAuxDataGrp : public uiGroup
{ mODTextTranslationClass(uiAuxDataGrp)
public:
				uiAuxDataGrp(uiParent*,bool forread);
				~uiAuxDataGrp();

    void			setKey(const MultiID&);
    void			setDataName(const char*);
    const char*			getDataName() const;

protected:
    void			selChg(CallBacker*);

    uiListBox*			listfld_;
    uiGenInput*			inpfld_;
};


mExpClass(uiEarthModel) uiAuxDataSel : public uiGroup
{ mODTextTranslationClass(uiAuxDataSel)
public:
				uiAuxDataSel(uiParent*,const char* type,
					     bool withobjsel,bool forread);
				~uiAuxDataSel();

    void			setKey(const MultiID&);
    void			setDataName(const char*);
    MultiID			getKey() const;
    const char*			getDataName() const;

protected:
    uiIOObjSel*			objfld_;
    uiIOSelect*			auxdatafld_;

    void			finalizeCB(CallBacker*);
    void			objSelCB(CallBacker*);
    void			auxSelCB(CallBacker*);
    BufferString		objtype_;
    MultiID			key_;
    BufferString		seldatanm_;
    bool			forread_;
};


mExpClass(uiEarthModel) uiBodySel : public uiIOObjSel
{
mODTextTranslationClass(uiBodySel)
public:
			uiBodySel(uiParent*,bool forread,
				     const uiIOObjSel::Setup&);
			uiBodySel(uiParent*,bool forread);
			~uiBodySel();
};


mExpClass(uiEarthModel) uiHorizonSel : public uiIOObjSel
{
mODTextTranslationClass(uiHorizonSel)
public:
			uiHorizonSel(uiParent*,bool is2d,
				     const ZDomain::Info*,bool isforread,
				     const uiIOObjSel::Setup& ={});
			uiHorizonSel(uiParent*,bool is2d,bool isforread,
				     const uiIOObjSel::Setup& ={});
			~uiHorizonSel();
protected:
    const uiString	getLabelText(const ZDomain::Info&,bool forread) const;
};


mExpClass(uiEarthModel) uiHorizon3DSel : public uiHorizonSel
{ mODTextTranslationClass(uiHorizon3DSel)
public:
			uiHorizon3DSel(uiParent*,
				       const ZDomain::Info*,bool isforread,
				       const uiIOObjSel::Setup& ={});
			uiHorizon3DSel(uiParent*,bool isforread,
				       const uiIOObjSel::Setup& ={});
			~uiHorizon3DSel();
};


mExpClass(uiEarthModel) uiFaultSel : public uiIOObjSel
{
mODTextTranslationClass(uiFaultSel)
public:
			uiFaultSel(uiParent*,EM::ObjectType,
				   const ZDomain::Info*,bool isforread,
				   const uiIOObjSel::Setup& ={});
			uiFaultSel(uiParent*,EM::ObjectType,
				   bool isforread,
				   const uiIOObjSel::Setup& ={});
			~uiFaultSel();
};
