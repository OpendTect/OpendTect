/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A. Huck
 Date:		Sep 2018
________________________________________________________________________

-*/

#include "uiseisexpcubepositions.h"

#include "uigeninput.h"
#include "uifilesel.h"
#include "uimsg.h"
#include "uiseisposprovgroup.h"

#include "file.h"
#include "od_helpids.h"
#include "od_ostream.h"
#include "seisprovider.h"
#include "seisposprovider.h"


uiSeisExpCubePositionsDlg::uiSeisExpCubePositionsDlg( uiParent* p )
    : uiDialog(p,Setup(tr("Export cube positions"),
		       tr("Specify export parameters"),
		       mODHelpKey(mSeisExpCubePosDlgHelpID))
		 .modal(false))
{
    setOkCancelText( uiStrings::sExport(), uiStrings::sClose() );
    setModal( false );
    setDeleteOnClose( false );

    const uiPosProvGroup::Setup su( false, false, false );
    inpfld_ = new uiSeisPosProvGroup( this, su );

    isascfld_ = new uiGenInput( this, tr("File type"),
				BoolInpSpec(true,uiStrings::sASCII(),
						 uiStrings::sBinary()) );
    isascfld_->attach( alignedBelow, inpfld_ );

    uiFileSel::Setup fssu; fssu.isForWrite();
    outfld_ = new uiFileSel( this, uiStrings::phrOutput(uiStrings::sFile()
				   .toLower()), fssu );
    outfld_->attach( alignedBelow, isascfld_ );
}


uiSeisExpCubePositionsDlg::~uiSeisExpCubePositionsDlg()
{
}


#define mGetKeyNm() ( IOPar::compKey(sKey::SeisCubePositions(),sKey::ID()) )

void uiSeisExpCubePositionsDlg::setInput( const DBKey& key )
{
    IOPar par;
    par.set( mGetKeyNm(), key );
    inpfld_->usePar( par );
}


void uiSeisExpCubePositionsDlg::show()
{
    dontask_ = false;
    uiDialog::show();
}


#define mErrRet(msg) { uiMSG().error( msg ); return false; }
bool uiSeisExpCubePositionsDlg::getPositions( const DBKey& key,
					      PosInfo::CubeData& cd ) const
{
    uiRetVal ret;
    PtrMan<Seis::Provider> prov = Seis::Provider::create( key, &ret );
    if ( !ret.isOK() )
	mErrRet(ret)
    else if ( prov->is2D() )
	{ pErrMsg("Should not happen: dbkey not from 3D data"); return false; }

    cd = prov->as3D()->possibleCubeData();
    return true;
}


bool uiSeisExpCubePositionsDlg::doExport( const PosInfo::CubeData& cd,
					  const char* filenm ) const
{
    uiString errmsg;
    if ( !File::checkDirectory(filenm,false,errmsg) )
	mErrRet( errmsg )

    if ( File::exists(filenm) )
    {
	if ( !File::isWritable(filenm) || !File::remove(filenm) )
	    mErrRet( uiStrings::phrCannotRemove( filenm ) )
    }

    od_ostream strm( filenm );
    if ( !strm.isOK() )
	mErrRet( strm.errMsg() )

    const bool ascii = isascfld_->getBoolValue();
    if ( !cd.write(strm,ascii) )
	mErrRet( uiStrings::phrErrDuringWrite() )

    return true;
}


bool uiSeisExpCubePositionsDlg::acceptOK()
{
    IOPar par;
    DBKey key;
    if ( !inpfld_->fillPar(par) || !par.get(mGetKeyNm(),key) )
	mErrRet( uiStrings::phrCannotReadInp() )

    const BufferString filenm( outfld_->fileName() );
    if ( filenm.isEmpty() )
	mErrRet( uiStrings::phrSelOutpFile() )

    if ( File::exists(filenm) &&
	 !uiMSG().askOverwrite(uiStrings::phrOutputFileExistsOverwrite()) )
	return false;

    PosInfo::CubeData cd;
    const bool success = getPositions( key, cd ) && doExport( cd, filenm );
    if ( !success )
	return false;

    uiStringSet msg(
	uiStrings::phrSuccessfullyExported(Pos::SeisProvider3D::dispType()) );
    msg.add(tr("%1?").arg(uiStrings::phrClose(uiStrings::sWindow().toLower())));
    if ( dontask_ )
    {
	uiMSG().message( msg.get(0) );
	return false;
    }

    return uiMSG().askGoOn( msg.cat(), true, &dontask_ );
}
