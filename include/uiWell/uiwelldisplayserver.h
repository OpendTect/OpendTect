#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellmod.h"

#include "uigroup.h"
#include "uistring.h"
#include "welldisp.h"

class uiParent;
class uiWellLogToolWinGrp;
class MnemonicSelection;
class WellLogToolData;
namespace Well
{
    class Data;
    class Log;
    class LogSet;
    class SubSelData;
}

mExpClass(uiWell) uiLogDisplayGrp : public uiGroup
{
public:
			uiLogDisplayGrp(uiParent*,const char* nm);
    virtual		~uiLogDisplayGrp();

    virtual void	addLogSelection(const ObjectSet<Well::SubSelData>&) {}
    virtual void	addLog(const Well::Log&)		{}
    virtual void	addLogSet(const Well::LogSet&)		{}
    virtual void	update()				{}
    virtual void	setDisplayProps(const Well::DisplayProperties::Log&,
					bool forname=true) {}
};


mExpClass(uiWell) uiWellDisplayServer
{
mODTextTranslationClass(uiWellDisplayServer)
public:
				uiWellDisplayServer();
    virtual			~uiWellDisplayServer();

    virtual uiMainWin*		createMultiWellDisplay(uiParent*,
						const DBKeySet&,
						const BufferStringSet&) =0;
    virtual uiWellLogToolWinGrp* createWellLogToolGrp(uiParent*,
				const ObjectSet<WellLogToolData>&) =0;

    virtual uiMainWin*		createLogViewWin(uiParent*,
					const ObjectSet<Well::Data>&,
					const BufferStringSet& lognms,
					const BufferStringSet& markernms,
					const DBKeySet& sel_ids,
					const BufferStringSet& sel_lognms,
					const BufferStringSet& sel_markernms)
				{ return nullptr; }
    virtual uiMainWin*		createLogViewWin(uiParent*,
					const ObjectSet<Well::Data>&,
					const MnemonicSelection&,
					const BufferStringSet& markernms,
					const DBKeySet& sel_ids,
					const MnemonicSelection& sel_mns,
					const BufferStringSet& sel_markernms)
				{ return nullptr; }
    virtual uiMainWin*		createLogViewWinCB(uiParent*,
					const ObjectSet<Well::Data>&,
					const BufferStringSet& lognms,
					const BufferStringSet& markernms,
					const DBKeySet& sel_ids,
					const BufferStringSet& sel_lognms,
					const BufferStringSet& sel_markernms)
				{ return nullptr; }
    virtual uiMainWin*		createLogViewWinCB(uiParent*,
					const ObjectSet<Well::Data>&,
					const MnemonicSelection&,
					const BufferStringSet& markernms,
					const DBKeySet& sel_ids,
					const MnemonicSelection& sel_mns,
					const BufferStringSet& sel_markernms)
				{ return nullptr; }

    virtual uiLogDisplayGrp*	createLogDisplayGrp(uiParent*)
				{ return nullptr; }


    virtual bool		isBasic() const { return false; }
};


mExpClass(uiWell) uiODWellDisplayServer : public uiWellDisplayServer
{
mODTextTranslationClass(uiODWellDisplayServer)
public:
			uiODWellDisplayServer();
			~uiODWellDisplayServer();

    uiMainWin*		createMultiWellDisplay(uiParent*,const DBKeySet&,
					const BufferStringSet&) override;
    uiWellLogToolWinGrp* createWellLogToolGrp(uiParent*,
				const ObjectSet<WellLogToolData>&) override;
    bool		isBasic() const override	{ return true; }
};


mGlobal(uiWell) uiWellDisplayServer&
			GetWellDisplayServer(bool set=false,
					     uiWellDisplayServer* wds=nullptr);
