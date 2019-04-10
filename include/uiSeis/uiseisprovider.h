#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          March 2019
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "seisgeomtypeprovider.h"
#include "dbkey.h"
#include "uigroup.h"
#include "uistring.h"
class SurveyDiskLocation;
namespace Seis { class Provider; class SelData; class ProviderInfo; }
namespace ZDomain { class Info; }
class uiButton;
class uiCheckBox;
class uiComboBox;
class uiLabel;
class uiListBox;
class uiSeisSelData;
class uiSeisProviderData;
class uiSeisProvGTProvider;


mExpClass(uiSeis) uiSeisProvider : public uiGroup
{ mODTextTranslationClass(uiSeisProvider);
public:

    mUseType( Seis,			GeomType );
    mUseType( Seis,			Provider );
    mUseType( Seis,			SelData );
    typedef int				idx_type;
    typedef int				comp_idx_type;
    typedef TypeSet<comp_idx_type>	comp_idx_set;
    enum Style				{ Compact, SingleLine, PickOne };
    enum SubSelPol			{ NoSubSel, OnlyRanges, AllSubsels };
    enum CompSelPol			{ OneComp, SomeComps, AllComps };
    enum PSPol				{ NoPS, OnlyPS };

    mExpClass(uiSeis) Setup
    {
    public:

	mUseType( Seis,		SteerPol );

				Setup();
				Setup(GeomType);
				Setup(GeomType,GeomType);
				Setup(GeomType,GeomType,GeomType);
				Setup(PSPol);
				Setup(const Setup&);
	virtual			~Setup();
	Setup&			operator =(const Setup&);

	mDefSetupMember(Style,		style,		SingleLine)
	mDefSetupMember(SubSelPol,	subselpol,	AllSubsels)
	mDefSetupMember(SteerPol,	steerpol,	Seis::NoSteering)
	mDefSetupMember(CompSelPol,	compselpol,	OneComp)
	mDefSetupMember(bool,		optional,	false)
	mDefSetupMemb(uiString,		seltxt)		// auto from GeomType
	const ZDomain::Info*		zdominf_	= 0;

	TypeSet<GeomType>	gts_;
	SurveyDiskLocation&	sdl_;

    };

			uiSeisProvider(uiParent*,const Setup&);
			~uiSeisProvider();
    const Setup&	setup() const			{ return setup_; }

    void		set(const DBKey&,const comp_idx_set* comps=0);
    void		setSelData(const SelData*);

    bool		isOK(bool showerr=true) const;
    Provider*		get(bool showerr=true) const;
			//!< ready to use, includes SelData
			//!< will emit error message when returning null

	    // if you do not want to create a Provider, but still want to know:
    GeomType		geomType() const;
    DBKey		key() const;
    comp_idx_set	components() const;
    SelData*		getSelData() const;
    uiSeisProvGTProvider& getGTProv()			{ return gtprov_; }

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    Notifier<uiSeisProvider> selectionChanged;
    Notifier<uiSeisProvider> geomTypeChanged;

protected:

    const Setup		setup_;
    uiSeisProviderData&	data_;
    uiSeisProvGTProvider& gtprov_;
    BufferString	usrsetkey_;
    SelData*		fixedseldata_			= nullptr;

    uiGroup*		maingrp_;
    uiCheckBox*		optcheck_			= nullptr;
    uiLabel*		lbl_				= nullptr;
    uiComboBox*		geomtypebox_			= nullptr;
    uiComboBox*		itmbox_				= nullptr;
    uiListBox*		itmlist_			= nullptr;
    uiComboBox*		compbox_			= nullptr;
    uiListBox*		complist_			= nullptr;
    uiButton*		selbut_				= nullptr;
    uiSeisSelData*	seldatafld_			= nullptr;

    void		attachFlds();
    void		fillUi();
    void		updUi();
    void		updCompsUi();
    void		setCurrent(idx_type,idx_type);
    void		setComponents(const comp_idx_set&);

    bool		isActive() const;
    idx_type		curGTIdx() const;
    idx_type		curObjIdx() const;
    Seis::ProviderInfo*	curInfo() const;

    void		initGrp(CallBacker*);
    void		gtChgCB(CallBacker*);
    void		selChgCB(CallBacker*);
    void		optChgCB(CallBacker*);
    void		selInpCB(CallBacker*);

};



mExpClass(uiSeis) uiSeisProvGTProvider : public Seis::GeomTypeProvider
{
public:

    mUseType( Seis,	GeomType );

			uiSeisProvGTProvider( const uiSeisProvider& uip )
			    : uiprov_(uip)
			{ mAttachCB( uiprov_.geomTypeChanged,
				     uiSeisProvGTProvider::gtChgCB ); }

    GeomType		geomType() const override
			{ return uiprov_.geomType(); }
    TypeSet<GeomType>	availableTypes() const override
			{ return uiprov_.setup().gts_; }

    const uiSeisProvider& uiprov_;

    void		gtChgCB( CallBacker* ) { geomTypeChanged.trigger(); }

};
