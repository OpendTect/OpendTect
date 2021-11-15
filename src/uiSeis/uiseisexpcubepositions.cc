/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A. Huck
 Date:		Sep 2018
________________________________________________________________________

-*/

#include "uiseisexpcubepositions.h"

#include "uigeninput.h"
#include "uifileinput.h"
#include "uimsg.h"
#include "uiseisposprovgroup.h"

#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "ioobj.h"
#include "od_helpids.h"
#include "od_ostream.h"
#include "seisposprovider.h"
#include "seisread.h"


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

    outfld_ = new uiASCIIFileInput( this, false );
    outfld_->attach( alignedBelow, isascfld_ );
}


uiSeisExpCubePositionsDlg::~uiSeisExpCubePositionsDlg()
{
}


#define mGetKeyNm() ( IOPar::compKey(sKey::SeisCubePositions(),sKey::ID()) )

void uiSeisExpCubePositionsDlg::setInput( const MultiID& key )
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
bool uiSeisExpCubePositionsDlg::getPositions( const MultiID& key,
					      PosInfo::CubeData& cd ) const
{
    PtrMan<IOObj> ioobj = IOM().get( key );
    if ( !ioobj )
    {
	pErrMsg("Should not happen: dbkey not from 3D data");
	const uiString msg = toUiString( IOM().message() );
	mErrRet( msg )
    }

    const Seis::GeomType gt = Seis::Vol;
    const SeisTrcReader rdr( *ioobj, &gt );
    if ( !rdr.get3DGeometryInfo(cd) )
	mErrRet( rdr.errMsg() )

    return true;
}


bool uiSeisExpCubePositionsDlg::doExport( const PosInfo::CubeData& cd,
					  const char* filenm ) const
{
    const FilePath filepath( filenm );
    if ( !File::isWritable(filepath.pathOnly()) )
	mErrRet( uiStrings::phrCannotWrite( uiStrings::sFolder() ) )

    if ( File::exists(filenm) )
    {
	if ( !File::isWritable(filenm) || !File::remove(filenm) )
	    mErrRet( uiStrings::phrCannotRemove(toUiString(filenm)) )
    }

    od_ostream strm( filenm );
    if ( !strm.isOK() )
	mErrRet( strm.errMsg() )

    const bool ascii = isascfld_->getBoolValue();
    if ( !cd.write(strm,ascii) )
	mErrRet( uiStrings::phrCannotWrite(uiStrings::sFile()) )

    return true;
}


bool uiSeisExpCubePositionsDlg::acceptOK( CallBacker* )
{
    IOPar par;
    MultiID key;
    if ( !inpfld_->fillPar(par) || !par.get(mGetKeyNm(),key) )
	mErrRet( uiStrings::phrCannotRead(uiStrings::sFile()) )

    const BufferString filenm( outfld_->fileName() );
    uiString errmsg = tr("Select output file");
    if ( filenm.isEmpty() )
	mErrRet( errmsg )

    if ( File::exists(filenm) &&
	 !uiMSG().askOverwrite(tr("%1 exists. %2?").arg( uiStrings::sFile() )
					.arg( uiStrings::sOverwrite()) ) )
	return false;

    PosInfo::CubeData cd;
    const bool success = getPositions( key, cd ) && doExport( cd, filenm );
    if ( !success )
	return false;

    uiStringSet msg( tr("Successfully exported %1")
			.arg(Pos::SeisProvider3D::sKeyType()) );
    msg.add(tr("%1 %2?").arg(uiStrings::sClose())
			.arg(uiStrings::sWindow().toLower()));
    if ( dontask_ )
    {
	uiMSG().message( msg[0] );
	return false;
    }

    return uiMSG().askGoOn( msg.cat(), true, &dontask_ );
}
