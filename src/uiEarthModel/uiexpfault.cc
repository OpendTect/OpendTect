/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		May 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiexpfault.h"

#include "bufstring.h"
#include "ctxtioobj.h"
#include "emfault3d.h"
#include "emfaultstickset.h"
#include "lmkemfaulttransl.h"
#include "emmanager.h"
#include "executor.h"
#include "file.h"
#include "filepath.h"
#include "ioobj.h"
#include "ptrman.h"
#include "strmprov.h"
#include "survinfo.h"
#include "uibutton.h"
#include "uichecklist.h"
#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uistrings.h"
#include "uitaskrunner.h"
#include "od_helpids.h"

#define mGet( tp, fss, f3d ) \
    FixedString(tp) == EMFaultStickSetTranslatorGroup::sGroupName() ? fss : f3d

#define mGetCtio(tp) \
    mGet( tp, *mMkCtxtIOObj(EMFaultStickSet), *mMkCtxtIOObj(EMFault3D) )
#define mGetTitle(tp) \
    mGet( tp, uiStrings::phrExport( uiStrings::sFaultStickSet() ), \
	      uiStrings::phrExport( uiStrings::sFault() ) )

#define mGetLbl(tp) \
    mGet( tp, uiStrings::phrInput( uiStrings::sFaultStickSet() ), \
	      uiStrings::phrInput( uiStrings::sFault() ) )


uiExportFault::uiExportFault( uiParent* p, const char* typ )
    : uiDialog(p,uiDialog::Setup(mGetTitle(typ),mNoDlgTitle,
				 mGet(typ,mODHelpKey(mExportFaultStickHelpID),
				 mODHelpKey(mExportFaultHelpID)) ))
    , ctio_(mGetCtio(typ))
    , linenmfld_(0)
{
    setModal( false );
    setDeleteOnClose( false );
    setOkCancelText( uiStrings::sExport(), uiStrings::sClose() );

    infld_ = new uiIOObjSel( this, ctio_, mGetLbl(typ) );

    coordfld_ = new uiGenInput( this, tr("Write coordinates as"),
				BoolInpSpec(true,tr("X/Y"),tr("Inl/Crl")) );
    coordfld_->attach( alignedBelow, infld_ );

    bool setchk = true;
    if ( SI().zIsTime() )
	zbox_ = new uiGenInput( this, tr("Z in"),
				BoolInpSpec(true,uiStrings::sMsec(),
				uiStrings::sSec()) );
    else
    {
	zbox_ = new uiGenInput( this, tr("Z in"),
				BoolInpSpec(true,uiStrings::sFeet(),
				uiStrings::sMeter()) );
	setchk = SI().depthsInFeet();
    }
    zbox_->setValue( setchk );
    zbox_->attach( rightTo, coordfld_ );

    stickidsfld_ = new uiCheckList( this, uiCheckList::ChainAll,
				    OD::Horizontal );
    stickidsfld_->setLabel( uiStrings::sWrite() );
    stickidsfld_->addItem( tr("Stick index") ).addItem( tr("Node index" ));
    stickidsfld_->setChecked( 0, true ); stickidsfld_->setChecked( 1, true );
    stickidsfld_->attach( alignedBelow, coordfld_ );

    if ( mGet(typ,true,false) )
    {
	linenmfld_ = new uiCheckBox( this,
				     tr("Write line name if picked on 2D") );
	linenmfld_->setChecked( true );
	linenmfld_->attach( alignedBelow, stickidsfld_ );
    }

    outfld_ = new uiFileInput( this, uiStrings::phrOutput(uiStrings::phrASCII(
			       uiStrings::sFile())),
			       uiFileInput::Setup().forread(false) );
    if ( linenmfld_ )
	outfld_->attach( alignedBelow, linenmfld_ );
    else
	outfld_->attach( alignedBelow, stickidsfld_ );
}


uiExportFault::~uiExportFault()
{
    delete ctio_.ioobj_; delete &ctio_;
}


static int stickNr( EM::EMObject* emobj, EM::SectionID sid, int stickidx )
{
    mDynamicCastGet(Geometry::FaultStickSet*,fss,emobj->sectionGeometry(sid));
    return fss->rowRange().atIndex( stickidx );
}


static int nrSticks( EM::EMObject* emobj, EM::SectionID sid )
{
    mDynamicCastGet(Geometry::FaultStickSet*,fss,emobj->sectionGeometry(sid));
    return fss->nrSticks();
}


static int nrKnots( EM::EMObject* emobj, EM::SectionID sid, int stickidx )
{
    mDynamicCastGet(Geometry::FaultStickSet*,fss,emobj->sectionGeometry(sid));
    const int sticknr = fss->rowRange().atIndex( stickidx );
    return fss->nrKnots( sticknr );
}


static Coord3 getCoord( EM::EMObject* emobj, EM::SectionID sid, int stickidx,
			int knotidx )
{
    mDynamicCastGet(Geometry::FaultStickSet*,fss,emobj->sectionGeometry(sid));
    const int sticknr = fss->rowRange().atIndex(stickidx);
    const int knotnr = fss->colRange(sticknr).atIndex(knotidx);
    return fss->getKnot( RowCol(sticknr,knotnr) );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiExportFault::writeAscii()
{
    const IOObj* ioobj = ctio_.ioobj_;
    if ( !ioobj ) mErrRet(tr("Cannot find fault in database"));

    RefMan<EM::EMObject> emobj = EM::EMM().createTempObject( ioobj->group() );
    if ( !emobj ) mErrRet(tr("Cannot add fault to EarthModel"))

    emobj->setMultiID( ioobj->key() );
    mDynamicCastGet(EM::Fault3D*,f3d,emobj.ptr())
    mDynamicCastGet(EM::FaultStickSet*,fss,emobj.ptr())
    if ( !f3d && !fss ) return false;

    PtrMan<Executor> loader = emobj->loader();
    if ( !loader ) mErrRet( uiStrings::phrCannotRead( uiStrings::sFault() ))

    uiTaskRunner taskrunner( this );
    if ( !TaskRunner::execute( &taskrunner, *loader ) ) return false;

    const BufferString fname = outfld_->fileName();
    StreamData sdo = StreamProvider( fname ).makeOStream();
    if ( !sdo.usable() )
    {
	sdo.close();
	mErrRet( uiStrings::sCantOpenOutpFile() );
    }

    BufferString str;
    float zfac = 1.f;
    if ( SI().zIsTime() )
	zfac = zbox_->getBoolValue() ? 1000.f : 1.f;
    else if ( SI().depthsInFeet() )
	zfac = zbox_->getBoolValue() ? 1 : mFromFeetFactorF;
    else // meter
	zfac = zbox_->getBoolValue() ? mToFeetFactorF : 1;

    const bool doxy = coordfld_->getBoolValue();
    const bool inclstickidx = stickidsfld_->isChecked( 0 );
    const bool inclknotidx = stickidsfld_->isChecked( 1 );
    const EM::SectionID sectionid = emobj->sectionID( 0 );
    const int nrsticks = nrSticks( emobj.ptr(), sectionid );
    for ( int stickidx=0; stickidx<nrsticks; stickidx++ )
    {
	const int nrknots = nrKnots( emobj.ptr(), sectionid, stickidx );
	for ( int knotidx=0; knotidx<nrknots; knotidx++ )
	{
	    const Coord3 crd = getCoord( emobj.ptr(), sectionid,
					 stickidx, knotidx );
	    if ( !crd.isDefined() )
		continue;

	    if ( !doxy )
	    {
		const BinID bid = SI().transform( crd );
		*sdo.ostrm << bid.inl() << '\t' << bid.crl();
	    }
	    else
	    {
		// ostreams print doubles awfully
		str.setEmpty();
		str += crd.x; str += "\t"; str += crd.y;
		*sdo.ostrm << str;
	    }

	    *sdo.ostrm << '\t' << crd.z*zfac;

	    if ( inclstickidx )
		*sdo.ostrm << '\t' << stickidx;
	    if ( inclknotidx )
		*sdo.ostrm << '\t' << knotidx;

	    if ( fss )
	    {
		const int sticknr = stickNr( emobj.ptr(), sectionid, stickidx );

		bool pickedon2d =
		    fss->geometry().pickedOn2DLine( sectionid, sticknr );
		if ( pickedon2d && linenmfld_->isChecked() )
		{
		    Pos::GeomID geomid =
			fss->geometry().pickedGeomID( sectionid, sticknr );
		    const char* linenm = Survey::GM().getName( geomid );

		    if ( linenm )
			*sdo.ostrm << '\t' << linenm;
		}
	    }

	    *sdo.ostrm << '\n';
	}
    }

    sdo.close();
    return true;
}


bool uiExportFault::acceptOK( CallBacker* )
{
    if ( !infld_->commitInput() )
	mErrRet( tr("Please select the input fault") );
    const BufferString outfnm( outfld_->fileName() );
    if ( outfnm.isEmpty() )
	mErrRet( uiStrings::sSelOutpFile() );

    if ( File::exists(outfnm)
      && !uiMSG().askOverwrite(uiStrings::sOutputFileExistsOverwrite()))
	return false;

    const bool res = writeAscii();

    if ( !res )	return false;

    const IOObj* ioobj = ctio_.ioobj_;

    const uiString tp =
      EMFaultStickSetTranslatorGroup::sGroupName() == ioobj->group()
	? uiStrings::sFaultStickSet()
	: uiStrings::sFault();
    const uiString tps =
     EMFaultStickSetTranslatorGroup::sGroupName() == ioobj->group()
	? uiStrings::sFaultStickSet(mPlural)
	: uiStrings::sFault(mPlural);
    uiString msg = tr( "%1 successfully exported.\n\n"
		    "Do you want to export more %2?" )
	.arg(tp).arg(tps);
    bool ret = uiMSG().askGoOn( msg, uiStrings::sYes(),
				tr("No, close window") );
    return !ret;
}
