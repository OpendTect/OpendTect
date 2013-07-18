#ifndef uiodinstpkgmgr_h
#define uiodinstpkgmgr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:		May 2010
 RCS:           $Id: uiodinstpkgmgr.h 8009 2013-06-20 06:11:26Z kristofer.tingdahl@dgbes.com $
________________________________________________________________________

-*/

#include "uiodinstmgrmod.h"
#include "uiodinstdlg.h"
#include "bufstringset.h"
#include "odinstplf.h"
class Timer;
class uiLabel;
class uiButton;
class uiListBox;
class uiComboBox;
class uiCheckBox;
class uiGenInput;
class uiTextBrowser;

namespace ODInst
{ class AppData; class PkgGroupSet; class PkgProps; class PkgSelMgr; 
  class InstallHandler; class HtmlComposer; class RelData;}


mDefClass(uiODInstMgr) uiODInstPkgMgr : public uiODInstDlg
{
public:

    			uiODInstPkgMgr(uiParent*,ODInst::AppData&,
				   ODInst::DLHandler&,
				   const ODInst::PkgGroupSet&,
				   const ODInst::Platform&,
				   const ODInst::RelData&,int choice,
				   bool isoffln);
			~uiODInstPkgMgr();

    void		setAutoUpdate( bool yn )	{ autoupdate_ = yn; }

protected:

    enum		PackageType{ GPL, Academic, Commercial, Custom };

    ODInst::AppData&	appdata_;
    const ODInst::RelData&	reldata_;
    const ODInst::PkgGroupSet&	pkggrps_;
    ODInst::HtmlComposer& htmlcomp_;
    ODInst::PkgSelMgr*	pkgselmgr_;
    bool		isnewinst_;
    ODInst::Platform	platform_;
    bool		autoupdate_;
    bool		ishtmlon_;

    uiComboBox*		pkglblfld_;
    uiListBox*		pkgsfld_;
    uiTextBrowser*	descfld_;
    uiGenInput*		creatorfld_;
    uiGenInput*		instverfld_;
    uiGenInput*		availverfld_;
    uiButton*		creatorurlbut_;
    uiLabel*		actionlbl_;
    uiButton*		filelistbut_;
    uiButton*		switchviewbut_;
    uiCheckBox*		reinstallfld_;

    uiGroup*		classicgrp_;
    uiGroup*		htmlgrp_;

    uiLabel*		mkTitleStuff();
    void		mkClassicGrp();
    void		mkHTMLGrp();

    void		initWin(CallBacker*);
    void		switchViewCB(CallBacker*);
    void		creatorWebPush(CallBacker*)	{ showURL(true); }
    void		pkgWebPush(CallBacker*)		{ showURL(false); }
    void		pkgSelChg(CallBacker*);
    void		grpSelChg(CallBacker*);
    void		filelistCB( CallBacker* );
    void		itmChck(CallBacker*);
    bool		acceptOK(CallBacker*);
    bool		rejectOK(CallBacker*);
    void		closeWinCB(CallBacker*);
    void		exportDlListCB(CallBacker*);
    void		rollbackCB(CallBacker*);
    void		statusMsgCB(CallBacker*);
    void		reinstallCB(CallBacker*);
    void		viewLog() const;
    virtual void	addPreFinaliseStuff();


    const ODInst::PkgProps& getPkg(const char*) const;
    const ODInst::PkgProps& curPkg() const;
    void		showURL(bool);
    bool		removeEntireInstallation();
    bool		doInstallation();
    void		makeFileList(const char*filenm,BufferString&) const;
    void		updActionDisp();
    void		updatePkgSelFld();
    BufferString	genHTML( const ODInst::PkgProps& ) const;
    void		cleanUp() const;
    void		getFilteredPackageList(
	                    ObjectSet<const ODInst::PkgProps>& res ) const;
    bool		checkIfODRunning() const;
    BufferStringSet	getPkgLabels() const;
    void		generateMainSelHtml();
    void		urlClickCB(CallBacker*);
    void		linkHighlightedCB(CallBacker*);
    void		generatePackageTableCell( const ODInst::PkgProps& pkg,
	    		    BufferString&, BufferString&, BufferString& ) const;
    void		generateActionHtml( const ODInst::PkgProps& pp,
					    BufferString& html ) const;
    void		getInstalledMsg( BufferString& msg );
    void		getDtectData();

    bool		iscustom_;
    bool		isgpl_;
    bool		isupdt_;
    Timer*		closewintimer_;
    const int		packagechoice_;
    BufferString	oldbasedirnm_;
    BufferString	filelist_;
    ODInst::InstallHandler*  insthandler_;

    const ODInst::PkgProps*	curpkg_;


    uiTextBrowser*	mainselfld_;
    bool		isviewingdetail_;
    bool		isoonline_;
};


#endif

