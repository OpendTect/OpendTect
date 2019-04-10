#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          March 2019
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uicompoundparsel.h"
#include "uidialog.h"
#include "dbkey.h"
#include "seistype.h"
class GeomIDSet;
class LineSubSelSet;
class LineSubSel;
class SeisIOObjInfo;
namespace Seis { class RangeSelData; class SelData; }
class uiComboBox;
class uiListBox;
class uiSingleLineSubSel;
class uiPolyPosProvGroup;
class uiSelNrRange;
class uiSelZRange;
class uiTablePosProvGroup;


mExpClass(uiSeis) uiSeisSelData : public uiCompoundParSel
{ mODTextTranslationClass(uiSeisSelData)
public:

    mUseType( Pos,	GeomID );
    mUseType( Seis,	GeomType );
    mUseType( Seis,	SelData );

    enum Opt		{ OnlyRange, AllSubsels };

			uiSeisSelData(uiParent*,Opt opt=AllSubsels);
			~uiSeisSelData();

    void		set(const SelData*);
    void		setInput(const DBKey&);

    SelData*		get() const;

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    Notifier<uiSeisSelData> typeChanged;

protected:

    const bool	onlyrange_;
    DBKey	dbky_;
    SelData*	seldata_		= nullptr;

    void	updUi();
    void	initGrp(CallBacker*);
    void	doDlg(CallBacker*);

    uiString	getSummary() const override;

};


class uiSeisSelDataDlg : public uiDialog
{ mODTextTranslationClass(uiSeisSelDataDlg)
public:

    mUseType( Pos,	GeomID );
    mUseType( Seis,	SelData);

			uiSeisSelDataDlg(uiParent*,const DBKey&,
					 const SelData* sd=0,
					 bool onlyrange=false);
			uiSeisSelDataDlg(uiParent*,bool is2d,
					 const SelData* sd=0,
					 bool onlyrange=false);
			~uiSeisSelDataDlg();

    bool		is2D() const	{ return lsss_; }
    SelData*		getSelData();

protected:

    LineSubSelSet*	lsss_		= nullptr;
    SeisIOObjInfo*	ioobjinf_	= nullptr;
    GeomIDSet*		geomids_	= nullptr;
    int			prevlidx_	= -1;

    uiComboBox*		typfld_		= nullptr;
    uiSelNrRange*	inlsel_		= nullptr;
    uiSelNrRange*	crlsel_		= nullptr;
    uiSelNrRange*	trcnrsel_	= nullptr;
    uiSelZRange*	zsel_		= nullptr;
    uiListBox*		linesel_	= nullptr;
    uiPolyPosProvGroup*	polysel_	= nullptr;
    uiTablePosProvGroup* tablesel_	= nullptr;

    void		initWin(CallBacker*);
    void		lineChgCB(CallBacker*);
    void		lineChosenCB(CallBacker*);
    void		updUi(CallBacker* cb=nullptr);
    void		updLineUi(int);
    void		setLSS(int);

    SelData*		get2DSelData();
    SelData*		get3DSelData();
    bool		acceptOK();

private:

    void		make2D();
    void		init(const SelData*,bool);
    void		create2DFlds(const SelData*,uiGroup*);
    void		create3DFlds(const SelData*,bool,uiGroup*);

};
