#ifndef uiodinstmgr_h
#define uiodinstmgr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2011
 RCS:           $Id: uiodinstmgr.h 8009 2013-06-20 06:11:26Z kristofer.tingdahl@dgbes.com $
________________________________________________________________________

-*/

#include "uiodinstmgrmod.h"
#include "uiodinstdlg.h"
#include "odinstrel.h"
#include "odinstplf.h"

class uiCheckList;
class uiCheckBox;
class uiComboBox;
class uiFileInput;
class uiLabeledComboBox;
class uiPushButton;
class uiRadioButton;

class Timer;
class FilePath;
namespace ODInst { class AppData; class PkgGroupSet; class Version; }


mDefClass(uiODInstMgr) uiODInstMgr : public uiODInstDlg
{
public:

    			uiODInstMgr(uiParent*,ODInst::AppData&,
				    const char* sitesubdir,
				    const char* forced_site);
			~uiODInstMgr();

    void		setStartupMode( bool yn )	{ startupmode_ = yn; }
    void		setArgs(int,char**);
    bool		isOK() const;
    ODInst::RelType	relType() const;
    const char*		instDir() const;
    const ODInst::RelData&  relData() const;

protected:

    ODInst::AppData&	appdata_;
    ODInst::RelDataSet	reldata_;
    ODInst::PkgGroupSet* pkggrps_;
    ODInst::Platform	platform_;

    uiComboBox*		reltypefld_;
    uiFileInput*	basedirfld_;

    void		startAction(CallBacker*);
    void		startInternet(CallBacker*);
    bool		acceptOK(CallBacker*);
    bool		rejectOK(CallBacker*);
    void		networkSettingsCB(CallBacker*);
    void		onlineCB(CallBacker*);
    void		plfSelCB(CallBacker*);
    BufferString	InstMgrPkg() const;
    void		selectPlatform();

    void		initWin();
    bool		getPkgGroups(const ODInst::Version&);
    void		getImages();
    int			getPackageChoice() const;
    int			checkInstDir(FilePath&);
    bool		hasUpdateForInstMgr();
    BufferString	getBaseDirFromSettings() const;

    BufferString	forcedsite_;
    BufferString	subdir_;
    BufferString	args_;
    bool		startupmode_;
    Timer*		timer_;
    uiPushButton*	settingsbut_;
    uiRadioButton*	onlinebut_;
    uiRadioButton*	offlinebut_;
    uiLabeledComboBox*	plffld_;
    bool		isonlinemode_;
};


#endif

