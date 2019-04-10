#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          March 2019
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "seistype.h"
#include "dbkey.h"
#include "uigroup.h"
#include "uistring.h"
class SurveyDiskLocation;
namespace Seis { class Storer; class SelData; class GeomTypeProvider; }
namespace ZDomain { class Info; }
class uiButton;
class uiCheckBox;
class uiComboBox;
class uiLabel;
class uiListBox;
class uiScaler;
class uiSeisSel;
class uiPosSubSel;


mExpClass(uiSeis) uiSeisStorer : public uiGroup
{ mODTextTranslationClass(uiSeisStorer);
public:

    mUseType( Seis,	Storer );
    mUseType( Seis,	GeomType );
    mUseType( Seis,	GeomTypeProvider );

    mExpClass(uiSeis) Setup
    {
    public:

				Setup()		{}
	virtual			~Setup()	{}

	mDefSetupMember(bool,	optional,	false)
	mDefSetupMember(bool,	allowscale,	true)
	mDefSetupMemb(BufferString, allowtransls) // empty => all
	mDefSetupMemb(uiString,	seltxt)		// empty => auto

    };

			uiSeisStorer(uiParent*,GeomType,
				     const Setup& su=Setup());
			uiSeisStorer(uiParent*,const GeomTypeProvider&,
				     const Setup& su=Setup());
			~uiSeisStorer();

    void		set(const DBKey&);

    bool		isOK(bool showerr=true) const;
    Storer*		get(bool showerr=true) const;

    DBKey		key() const;

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    Notifier<uiSeisStorer> selectionChanged;

protected:

    const Setup		setup_;
    const GeomTypeProvider& gtprov_;
    bool		gtprovmine_		= false;

    ObjectSet<uiSeisSel> seissels_;
    uiScaler*		scalefld_		= nullptr;

    int			curSelIdx() const;
    void		createFlds();
    void		updUi();

    void		initGrp(CallBacker*);
    void		selChgCB(CallBacker*);
    void		optChgCB(CallBacker*);

};
