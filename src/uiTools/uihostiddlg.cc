/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uihostiddlg.h"

#include "bufstring.h"
#include "file.h"
#include "filepath.h"
#include "generalinfo.h"
#include "oddirs.h"
#include "od_helpids.h"
#include "odplatform.h"
#include "systeminfo.h"

#include "uibutton.h"
#include "uiclipboard.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilocalhostgrp.h"
#include "uimsg.h"

#include <QTimeZone>


// uiInfoGrp

class uiInfoGroup : public uiDlgGroup
{mODTextTranslationClass(uiInfoGroup)
public:
		uiInfoGroup( uiParent* p, const uiString& caption )
		  : uiDlgGroup(p,caption)
		{}
		~uiInfoGroup()	{}

    virtual void	addInfo(BufferStringSet&) const  = 0;
};


// uiSystemInfoGrp

class uiSystemInfoGrp : public uiInfoGroup
{ mODTextTranslationClass( uiSystemInfoGrp )
public:
uiSystemInfoGrp( uiParent* p )
    : uiInfoGroup( p, tr("System Information") )
{
    hostidfld_ = new uiGenInput( this, tr("HostID(s)") );
    hostidfld_->setReadOnly();
    hostidfld_->setElemSzPol( uiObject::Wide );

    localhostgrp_ = new uiLocalHostGrp( this, tr("Computer/Host") );
    localhostgrp_->attach( alignedBelow, hostidfld_ );
    localhostgrp_->setHSzPol( uiObject::Wide );

    timezonefld_ = new uiGenInput( this, tr("Time Zone") );
    timezonefld_->setReadOnly();
    timezonefld_->attach( alignedBelow, localhostgrp_ );
    timezonefld_->setElemSzPol( uiObject::Wide );

    osfld_ = new uiGenInput( this, tr("Operating System") );
    osfld_->setReadOnly();
    osfld_->attach( alignedBelow, timezonefld_ );
    osfld_->setElemSzPol( uiObject::Wide );

    productnmfld_ = new uiGenInput( this, tr("OS Product name") );
    productnmfld_->setReadOnly();
    productnmfld_->attach( alignedBelow, osfld_ );
    productnmfld_->setElemSzPol( uiObject::Wide );

    usernmfld_ = new uiGenInput( this, tr("User name") );
    usernmfld_->setReadOnly();
    usernmfld_->attach( alignedBelow, productnmfld_ );
    usernmfld_->setElemSzPol( uiObject::Wide );

    if ( __islinux__ )
    {
	kernelverfld_ = new uiGenInput( this, tr("Kernel Version") );
	kernelverfld_->setReadOnly();
	kernelverfld_->attach( alignedBelow, usernmfld_ );
	kernelverfld_->setElemSzPol( uiObject::Wide );
	kernelverfld_->setText( System::kernelVersion() );
    }

    BufferStringSet hostids;
    BufferString errmsg;
    OD::getHostIDs( hostids, errmsg );
    if ( hostids.isEmpty() )
    {
	setCaption( uiString::emptyString() );
	uiString msg;
	msg.append( errmsg.str(), true );
	msg.append( tr("No HostID found.\n"
		    "Please contact support@dgbes.com"), true );
	new uiLabel( this, msg );
	return;
    }

    BufferString hostidstext = hostids.cat( " " );
    if ( hostids.size() > 1 )
	hostidstext.quote( '"' );

    const QTimeZone qloczone = QTimeZone::systemTimeZone();
    const QTimeZone::TimeType ttyp = QTimeZone::GenericTime;
    const QString qloczoneabbr = qloczone.displayName( ttyp,
						    QTimeZone::ShortName );
    BufferString zonestr( qloczoneabbr );
    if ( !__iswin__ )
    {
	const QString qloczoneoffs = qloczone.displayName( ttyp,
						    QTimeZone::OffsetName );
	zonestr.add( " (" ).add( qloczoneoffs ).add( ")" );
    }

    hostidfld_->setText( hostidstext );
    timezonefld_->setText( zonestr );
    osfld_->setText( OD::Platform().longName() );
    productnmfld_->setText( System::productName() );
    usernmfld_->setText( GetUserNm() );
}


~uiSystemInfoGrp()
{
}


private:

void addInfo( BufferStringSet& infos ) const override
{
    const BufferString idstr( "HostIDs: ", hostidfld_->text() );
    const BufferString namestr( "Host name: ", localhostgrp_->hostname() );
    const BufferString ipstr( "IP Address: ", localhostgrp_->address() );
    const BufferString timestr( "Time Zone: ", timezonefld_->text() );
    const BufferString osstr( "Operating System: ", osfld_->text() );
    const BufferString productstr( "Product name: ", productnmfld_->text() );
    const BufferString userstr( "User name: ", usernmfld_->text() );

    infos.add( idstr );
    infos.add( namestr );
    infos.add( ipstr );
    infos.add( timestr );
    infos.add( osstr );
    infos.add( productstr );
    infos.add( userstr );

    if ( kernelverfld_ )
    {
	const BufferString kernelstr( "Kernel Version: ",
						kernelverfld_->text() );
	infos.add( kernelstr );
    }
}

    uiGenInput*		hostidfld_;
    uiLocalHostGrp*	localhostgrp_;
    uiGenInput*		timezonefld_;
    uiGenInput*		osfld_;
    uiGenInput*		productnmfld_;
    uiGenInput*		usernmfld_;
    uiGenInput*		kernelverfld_		= nullptr;
};


// uiOdTInfoGrp

class uiOdTInfoGrp : public uiInfoGroup
{ mODTextTranslationClass( uiOdTInfoGrp )
public:
uiOdTInfoGrp( uiParent* p )
    : uiInfoGroup( p, tr("OpendTect Information") )
{
    interpreternmfld_ = new uiGenInput( this, tr("Interpreter name") );
    interpreternmfld_->setReadOnly();
    interpreternmfld_->setElemSzPol( uiObject::Wide );
    interpreternmfld_->setText( GetInterpreterName() );

    settingsfld_ = new uiGenInput( this, tr("OpendTect Settings folder") );
    settingsfld_->setReadOnly();
    settingsfld_->setElemSzPol( uiObject::Wide );
    settingsfld_->attach( alignedBelow, interpreternmfld_ );
    settingsfld_->setText( FilePath::getLongPath(GetSettingsDir()) );

    odinstfld_ = new uiGenInput( this, tr("OpendTect Installation folder") );
    odinstfld_->setReadOnly();
    odinstfld_->setElemSzPol( uiObject::Wide );
    odinstfld_->attach( alignedBelow, settingsfld_ );
    odinstfld_->setText( FilePath::getLongPath(GetSoftwareDir(true)) );

    currentpathfld_ = new uiGenInput( this, tr("Current Working Directory") );
    currentpathfld_->setReadOnly();
    currentpathfld_->setElemSzPol( uiObject::Wide );
    currentpathfld_->attach( alignedBelow, odinstfld_ );
    currentpathfld_->setText( File::getCurrentPath() );
}


~uiOdTInfoGrp()
{
}


private:

void addInfo( BufferStringSet& infos ) const override
{
    const BufferString interpreterstr( "Interpreter name: ",
					interpreternmfld_->text() );
    const BufferString settingsstr( "OpendTect Settings folder: ",
					settingsfld_->text() );
    const BufferString odinststr( "OpendTect Installation folder: ",
					odinstfld_->text() );
    const BufferString currentstr( "Current Working Directory: ",
					currentpathfld_->text() );

    infos.add( interpreterstr );
    infos.add( settingsstr );
    infos.add( odinststr );

}

    uiGenInput*		interpreternmfld_;
    uiGenInput*		settingsfld_;
    uiGenInput*		odinstfld_;
    uiGenInput*		currentpathfld_;
};


// uiGraphicInfoGrp

class uiGraphicInfoGrp : public uiInfoGroup
{ mODTextTranslationClass( uiSystemInfoGrp )
public:
uiGraphicInfoGrp( uiParent* p, const IOPar& pars )
    : uiInfoGroup( p, tr("Graphics Card Information") )
{
    const IOPar& graphicsinfo = System::graphicsInformation();
    BufferStringSet keys_;
    graphicsinfo.getKeys( keys_ );
    uiGenInput* prevfld = nullptr;
    for ( const auto* key : keys_ )
    {
	const BufferString value = graphicsinfo.find( key->buf() );
	auto* inputfld = new uiGenInput( this, toUiString(*key) );

	inputfld->setText( value );
	inputfld->setReadOnly();
	inputfld->setElemSzPol( uiObject::Wide );

	if ( prevfld )
	    inputfld->attach( alignedBelow, prevfld );

	inputfields_ += inputfld;
	prevfld = inputfld;
    }
}


~uiGraphicInfoGrp()
{
}


private:

void addInfo( BufferStringSet& infos ) const override
{
    for ( const auto* inpfld : inputfields_ )
    {
	const BufferString key = inpfld->titleText().getString();
	const BufferString value = inpfld->text();
	const BufferString info( key, ": ", value );
	infos.add( info );
    }
}

    ObjectSet<uiGenInput>	inputfields_;
};


// uiInformationDlg

uiInformationDlg::uiInformationDlg( uiParent* p )
    : uiTabStackDlg(p,Setup(tr("System Information"),
			    mODHelpKey(mInformationHelpID)))
{
    setOkCancelText( tr("Copy to Clipboard"), uiStrings::sClose() );

    uiGroup* tabgrp = tabstack_->tabGroup();
    auto* sysinfogrp = new uiSystemInfoGrp( tabgrp );
    auto* odinfogrp = new uiOdTInfoGrp( tabgrp );

    infodlggrp_.add( sysinfogrp );
    infodlggrp_.add( odinfogrp );

    const IOPar& graphicspars = System::graphicsInformation();
    if ( !graphicspars.isEmpty() )
    {
	auto* graphicinfogrp = new uiGraphicInfoGrp( tabgrp,  graphicspars );
	infodlggrp_.add( graphicinfogrp );
    }

    for ( auto* grp : infodlggrp_ )
    {
	grp->attach( hCentered );
	addGroup( grp );
    }
    mAttachCB( postFinalize(), uiInformationDlg::finalizeCB );
}


uiInformationDlg::~uiInformationDlg()
{
    detachAllNotifiers();
}


void uiInformationDlg::finalizeCB( CallBacker* )
{
    button(OK)->setIcon( "clipboard" );
}


bool uiInformationDlg::acceptOK( CallBacker* )
{
    copyToClipboard();
    return false;
}


void uiInformationDlg::copyToClipboard()
{
    BufferStringSet infos;
    for ( const auto* grp : infodlggrp_ )
	grp->addInfo( infos );

    const BufferString txt = infos.cat();
    uiClipboard::setText( txt.buf() );
    uiMSG().message( tr("Information copied to clipboard.\n"
		"When requesting a license, you can now paste the\n"
		"contents in an email and send to support@dgbes.com") );
}
