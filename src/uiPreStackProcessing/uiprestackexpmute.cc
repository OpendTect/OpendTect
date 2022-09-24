/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiprestackexpmute.h"

#include "uifileinput.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uistrings.h"

#include "file.h"
#include "mathfunc.h"
#include "oddirs.h"
#include "prestackmutedef.h"
#include "prestackmutedeftransl.h"
#include "survinfo.h"
#include "od_helpids.h"
#include "od_ostream.h"

namespace PreStack
{

uiExportMute::uiExportMute( uiParent* p )
    : uiDialog(p,uiDialog::Setup( uiStrings::phrExport( tr("Mute Function") ),
				 mNoDlgTitle,
				 mODHelpKey(mPreStackExportMuteHelpID) ))
{
    setOkCancelText( uiStrings::sExport(), uiStrings::sClose() );

    const IOObjContext ctxt = mIOObjContext( MuteDef );
    infld_ = new uiIOObjSel( this, ctxt, tr("Mute Definition") );

    coordfld_ = new uiGenInput( this, tr("Write coordinates as"),
				BoolInpSpec(true,tr("X/Y"),tr("Inl/Crl")) );
    coordfld_->attach( alignedBelow, infld_ );
    coordfld_->valuechanged.notify( mCB(this,uiExportMute,coordTypChngCB) );

    uiObject* attachobj = coordfld_->attachObj();
    if ( SI().hasProjection() )
    {
	coordsysselfld_ = new Coords::uiCoordSystemSel( this );
	coordsysselfld_->attach( alignedBelow, attachobj );
	attachobj = coordsysselfld_->attachObj();
    }

    outfld_ = new uiASCIIFileInput( this, false );
    outfld_->attach( alignedBelow, attachobj );

    coordTypChngCB(0);
}


uiExportMute::~uiExportMute()
{
}


void uiExportMute::coordTypChngCB( CallBacker* )
{
    if ( coordsysselfld_ )
	coordsysselfld_->display( coordfld_->getBoolValue() );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiExportMute::writeAscii()
{
    const IOObj* ioobj = infld_->ioobj();
    if ( !ioobj )
	return false;

    PtrMan<MuteDefTranslator> trans =
			sCast(MuteDefTranslator*,ioobj->createTranslator());
    if ( !trans ) return false;

    MuteDef mutedef;
    uiString errstr;
    const bool retval = trans->retrieve( mutedef, ioobj, errstr );
    if ( !retval ) mErrRet( errstr );

    const BufferString fname = outfld_->fileName();
    od_ostream strm( fname );
    if ( !strm.isOK() )
	mErrRet( uiStrings::sCantOpenOutpFile() );

    const bool isxy = coordfld_->getBoolValue();

    BufferString str;
    const Coords::CoordSystem* outcrs =
	coordsysselfld_ ? coordsysselfld_->getCoordSystem() : nullptr;
    const Coords::CoordSystem* syscrs = SI().getCoordSystem();
    Coord convcoord;
    Coord coord;
    const bool needsconversion = outcrs && !(*outcrs == *syscrs);
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
		coord = SI().transform( binid );
		if ( needsconversion )
		    convcoord = outcrs->convertFrom( coord, *syscrs );
		// ostreams print doubles awfully
		str.setEmpty();
		str += convcoord.x; str += "\t"; str += convcoord.y;
		strm << str;
	    }

	    strm << '\t' << offsetvals[offsetidx] << '\t' <<
		       mutedef.getFn( pos ).getValue( offsetvals[offsetidx] );

	    strm << '\n';
	}
    }

    return true;
}


bool uiExportMute::acceptOK( CallBacker* )
{
    const BufferString outfnm( outfld_->fileName() );
    if ( outfnm.isEmpty() )
	mErrRet( uiStrings::sSelOutpFile() );

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
