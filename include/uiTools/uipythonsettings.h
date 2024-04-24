/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"

#include "commanddefs.h"

class uiFileInput;
class uiGenInput;
class uiPathSel;
class uiToolBarCommandEditor;


mExpClass(uiTools) uiPythonSettings : public uiDialog
{
mODTextTranslationClass(uiPythonSettings)
public:
			uiPythonSettings(uiParent*, const char*);
    virtual		~uiPythonSettings();

    static CommandDefs	getPythonIDECommands();

private:
    bool		isOK(bool noerr=false) const;
    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);
    bool		commitSetts(const IOPar&);
    IOPar&		curSetts() const;
    void		setCustomEnvironmentNames();
    void		testPythonModules();
    bool		getPythonEnvBinPath(BufferString&) const;
    void		updateIDEfld();

    void		initDlg(CallBacker*);
    void		sourceChgCB(CallBacker*);
    void		customEnvChgCB(CallBacker*);
    void		internalLocChgCB(CallBacker*);
    void		parChgCB(CallBacker*);

    void		testCB(CallBacker*);
    void		promptCB(CallBacker*);
    void		safetycheckCB(CallBacker*);
    void		cloneCB(CallBacker*);
    void		cloneFinishedCB(CallBacker*);
    bool		rejectOK(CallBacker*) override;
    bool		acceptOK(CallBacker*) override;

    uiGenInput*		pythonsrcfld_;
    uiFileInput*	internalloc_ = nullptr;
    uiFileInput*	customloc_;
    uiGenInput*		customenvnmfld_;
    uiPathSel*		custompathfld_;
    uiToolBarCommandEditor* pyidefld_;
    uiToolBarCommandEditor* pytermfld_;
    uiButton*		clonebut_;

    IOPar		initialsetts_;
    bool		needrestore_ = false;
};


mExpClass(uiTools) uiSettingsMgr : public CallBacker
{ mODTextTranslationClass(uiSettingsMgr);
public:
				uiSettingsMgr();
				~uiSettingsMgr();

    void			loadToolBarCmds(uiMainWin&);
    void			updateUserCmdToolBar();
    uiRetVal			openTerminal(bool withfallback=true,
					const char* cmd=nullptr,
					const BufferStringSet* args=nullptr,
					const char* workingdir=nullptr);
    uiToolBar*			getToolBar()		{ return usercmdtb_; }
    const BufferStringSet*	programArgs(int) const;

    Notifier<uiSettingsMgr>	terminalRequested;
    Notifier<uiSettingsMgr>	toolbarUpdated;

private:
			mOD_DisableCopy(uiSettingsMgr);

    void		keyPressedCB(CallBacker*);
    void		updateUserCmdToolBarCB(CallBacker*);
    void		doTerminalCmdCB(CallBacker*);
    void		doToolBarCmdCB(CallBacker*);
    void		doPythonSettingsCB(CallBacker*);

    BufferStringSet	commands_;
    ObjectSet<const BufferStringSet>	progargs_;
    BufferStringSet	prognms_;
    TypeSet<int>	toolbarids_;
    int			termcmdidx_ = -1;
    int			idecmdidx_ = -1;

    uiMenu*		usercmdmnu_ = nullptr;
    uiToolBar*		usercmdtb_ = nullptr;
};

mGlobal(uiTools) uiSettingsMgr& uiSettsMgr();
