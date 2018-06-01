/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		June 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiprestackexpmute.h"

#include "uifileinput.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uistrings.h"

#include "ctxtioobj.h"
#include "file.h"
#include "mathfunc.h"
#include "oddirs.h"
#include "prestackmutedef.h"
#include "prestackmutedeftransl.h"
#include "strmprov.h"
#include "survinfo.h"
#include "od_helpids.h"

namespace PreStack
{

uiExportMute::uiExportMute( uiParent* p )
    : uiDialog(p,uiDialog::Setup( uiStrings::phrExport( tr("Mute Function") ),
				 mNoDlgTitle,
                                 mODHelpKey(mPreStackExportMuteHelpID) ))
    , ctio_(*mMkCtxtIOObj(MuteDef))
{
    setOkCancelText( uiStrings::sExport(), uiStrings::sClose() );

    infld_ = new uiIOObjSel( this, ctio_, tr("Mute Definition") );

    coordfld_ = new uiGenInput( this, tr("Write coordinates as"),
			        BoolInpSpec(true,tr("X/Y"),tr("Inl/Crl")) );
    coordfld_->attach( alignedBelow, infld_ );

    outfld_ = new uiFileInput( this, uiStrings::sOutputASCIIFile(),
			       uiFileInput::Setup().forread(false) );
    outfld_->attach( alignedBelow, coordfld_ );
}


uiExportMute::~uiExportMute()
{
    delete ctio_.ioobj_; delete &ctio_;
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiExportMute::writeAscii()
{
    if ( !infld_->ioobj() )
	return false;

    PtrMan<MuteDefTranslator> trans =
	(MuteDefTranslator*)ctio_.ioobj_->createTranslator();
    if ( !trans ) return false;

    MuteDef mutedef;
    uiString errstr;
    const bool retval = trans->retrieve( mutedef, ctio_.ioobj_, errstr );
    if ( !retval ) mErrRet( errstr );

    const BufferString fname = outfld_->fileName();
    StreamData sdo = StreamProvider( fname ).makeOStream();
    if ( !sdo.usable() )
    {
	sdo.close();
	mErrRet( uiStrings::sCantOpenOutpFile() );
    }

    const bool isxy = coordfld_->getBoolValue();

    BufferString str;
    for ( int pos=0; pos<mutedef.size(); pos++ )
    {
	const BinID binid = mutedef.getPos( pos );
	const TypeSet<float>& offsetvals = mutedef.getFn( pos ).xVals();
	for ( int offsetidx=0; offsetidx<offsetvals.size(); offsetidx++ )
	{
	    if( !isxy )
		*sdo.ostrm << binid.inl() << '\t' << binid.crl();
	    else
	    {
		const Coord coord = SI().transform( binid );
		// ostreams print doubles awfully
		str.setEmpty();
		str += coord.x; str += "\t"; str += coord.y;
		*sdo.ostrm << str;
	    }

	    *sdo.ostrm << '\t' << offsetvals[offsetidx] << '\t' <<
		       mutedef.getFn( pos ).getValue( offsetvals[offsetidx] );

	    *sdo.ostrm << '\n';
	}
    }

    sdo.close();
    return true;
}


bool uiExportMute::acceptOK( CallBacker* )
{
    const BufferString outfnm( outfld_->fileName() );
    if ( outfnm.isEmpty() )
	mErrRet( uiStrings::sSelOutpFile() );

    if ( File::exists(outfnm)
	&& !uiMSG().askContinue(
		uiStrings::phrExistsConinue( uiStrings::sOutputFile(), false )))
	return false;

    if ( writeAscii() )
	uiMSG().message(
		    uiStrings::phrSuccessfullyExported( uiStrings::sMute() ));
    return false;
}

} // namespace PreStack
