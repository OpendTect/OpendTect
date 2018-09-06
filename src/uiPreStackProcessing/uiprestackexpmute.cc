/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		June 2008
________________________________________________________________________

-*/

#include "uiprestackexpmute.h"

#include "uifilesel.h"
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
#include "survinfo.h"
#include "od_ostream.h"
#include "od_helpids.h"

namespace PreStack
{

uiExportMute::uiExportMute( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Export Mute Function"),mNoDlgTitle,
				 mODHelpKey(mPreStackExportMuteHelpID))
		.modal(false))
    , ctio_(*mMkCtxtIOObj(MuteDef))
{
    setOkCancelText( uiStrings::sExport(), uiStrings::sClose() );

    infld_ = new uiIOObjSel( this, ctio_, tr("Mute Definition") );

    coordfld_ = new uiGenInput( this, tr("Write coordinates as"),
			        BoolInpSpec(true,tr("X/Y"),tr("Inl/Crl")) );
    coordfld_->attach( alignedBelow, infld_ );

    coordsysselfld_ = new Coords::uiCoordSystemSel( this );
    coordsysselfld_->attach(rightOf, coordfld_);

    uiFileSel::Setup fssu; fssu.setForWrite();
    outfld_ = new uiFileSel( this, uiStrings::sOutputASCIIFile(), fssu );
    outfld_->attach( alignedBelow, coordsysselfld_ );
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
    od_ostream strm( fname );
    if ( !strm.isOK() )
	mErrRet( uiStrings::phrCannotOpenOutpFile() );

    const bool isxy = coordfld_->getBoolValue();

    BufferString str;
    for ( int pos=0; pos<mutedef.size(); pos++ )
    {
	const BinID binid = mutedef.getPos( pos );
	const TypeSet<float>& offsetvals = mutedef.getFn( pos ).xVals();
	for ( int offsetidx=0; offsetidx<offsetvals.size(); offsetidx++ )
	{
	    if( !isxy )
		strm << binid.inl() << '\t' << binid.crl();
	    else
	    {
		Coord coord = SI().transform( binid );
		coord = coordsysselfld_->getCoordSystem()->convertFrom(
						coord,*SI().getCoordSystem());
		// ostreams print doubles awfully
		str.setEmpty();
		str += coord.x_; str += "\t"; str += coord.y_;
		strm << str;
	    }

	    strm << '\t' << offsetvals[offsetidx] << '\t' <<
		       mutedef.getFn( pos ).getValue( offsetvals[offsetidx] );

	    strm << '\n';
	}
    }

    return true;
}


bool uiExportMute::acceptOK()
{
    const BufferString outfnm( outfld_->fileName() );
    if ( outfnm.isEmpty() )
	mErrRet( uiStrings::phrSelOutpFile() );

    if ( File::exists(outfnm)
	&& !uiMSG().askContinue(
		uiStrings::phrExistsContinue(uiStrings::sOutputFile(),false)) )
	return false;

    if ( writeAscii() )
	uiMSG().message(
		    uiStrings::phrSuccessfullyExported( uiStrings::sMute() ));
    return false;
}

} // namespace PreStack
