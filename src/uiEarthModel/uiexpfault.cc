/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		May 2008
________________________________________________________________________

-*/

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
#include "od_ostream.h"
#include "ptrman.h"
// #include "strmprov.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "uibutton.h"
#include "uichecklist.h"
#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uistrings.h"
#include "uitaskrunner.h"
#include "uiunitsel.h"
#include "od_helpids.h"
#include "dbman.h"
#include "emioobjinfo.h"
#include "uiioobjselgrp.h"

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


uiExportFault::uiExportFault( uiParent* p, const char* typ, bool issingle )
    : uiDialog(p,uiDialog::Setup(mGetTitle(typ),mNoDlgTitle,
				 mGet(typ,mODHelpKey(mExportFaultStickHelpID),
				 mODHelpKey(mExportFaultHelpID)) ))
    , ctio_(mGetCtio(typ))
    , linenmfld_(0)
    , issingle_(issingle)
    , singleinfld_(0)
    , bulkinfld_(0)
{
    setModal( false );
    setDeleteOnClose( false );
    setOkCancelText( uiStrings::sExport(), uiStrings::sClose() );
    uiIOObjSelGrp::Setup su; su.choicemode_ = !issingle_ ?
	    OD::ChoiceMode::ChooseAtLeastOne : OD::ChoiceMode::ChooseOnlyOne;
    if ( issingle_ )
	singleinfld_ = new uiIOObjSel( this, ctio_, mGetLbl(typ) );
    else
	bulkinfld_ = new uiIOObjSelGrp( this, ctio_, mGetLbl(typ), su );

    coordfld_ = new uiGenInput( this, tr("Write coordinates as"),
				BoolInpSpec(true,tr("X/Y"),tr("Inl/Crl")) );
    if ( issingle_ )
	coordfld_->attach( alignedBelow, singleinfld_ );
    else
	coordfld_->attach( alignedBelow, bulkinfld_ );

    uiUnitSel::Setup unitselsu( PropertyRef::surveyZType(), tr("Z in") );
    zunitsel_ = new uiUnitSel( this, unitselsu );
    zunitsel_->attach( rightTo, coordfld_ );

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

    outfld_ = new uiFileInput( this, uiStrings::sOutputASCIIFile(),
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


static int stickNr( EM::EMObject* emobj, int stickidx )
{
    mDynamicCastGet(Geometry::FaultStickSet*,fss,emobj->geometryElement());
    return fss->rowRange().atIndex( stickidx );
}


static int nrSticks( EM::EMObject* emobj )
{
    mDynamicCastGet(Geometry::FaultStickSet*,fss,emobj->geometryElement());
    return fss->nrSticks();
}


static int nrKnots( EM::EMObject* emobj, int stickidx )
{
    mDynamicCastGet(Geometry::FaultStickSet*,fss,emobj->geometryElement());
    const int sticknr = fss->rowRange().atIndex( stickidx );
    return fss->nrKnots( sticknr );
}


static Coord3 getCoord( EM::EMObject* emobj, int stickidx,
			int knotidx )
{
    mDynamicCastGet(Geometry::FaultStickSet*,fss,emobj->geometryElement());
    const int sticknr = fss->rowRange().atIndex(stickidx);
    const int knotnr = fss->colRange(sticknr).atIndex(knotidx);
    return fss->getKnot( RowCol(sticknr,knotnr) );
}


#define mErrRet(s) { uiMSG().error(s); return false; }


bool uiExportFault::getInputDBKeys( DBKeySet& dbkeyset )
{
    if ( issingle_ )
    {
	const IOObj* ioobj = ctio_.ioobj_;
	if ( !ioobj ) return false;
	dbkeyset.add(ioobj->key());
    }
    else
	dbkeyset = bulkinfld_->getIOObjIds();
    return true;
}

bool uiExportFault::writeAscii()
{
    DBKeySet dbkeyset;
    if ( !getInputDBKeys(dbkeyset) )
	mErrRet(tr("No faults selected"))

    const BufferString fname = outfld_->fileName();
    od_ostream ostrm( fname );
    if ( !ostrm.isOK() )
	return false;

    uiTaskRunner taskrunner( this );
    BufferString typnm = issingle_ ? ctio_.ioobj_->group() :
				    bulkinfld_->getCtxtIOObj().ioobj_->group();
    RefObjectSet<EM::EMObject> loadedobjs =
		EM::EMM().loadObjects( typnm, dbkeyset, 0, &taskrunner );
    if ( loadedobjs.isEmpty() )
	return false;

    for ( int idx=0; idx<loadedobjs.size(); idx++ )
    {
	mDynamicCastGet(EM::Fault3D*,f3d,loadedobjs[idx])
	mDynamicCastGet(EM::FaultStickSet*,fss,loadedobjs[idx])
	if ( !f3d && !fss ) return false;

	BufferString objnm = f3d ? f3d->name() : fss->name();
	BufferString str;
	const UnitOfMeasure* unit = zunitsel_->getUnit();
	const bool doxy = coordfld_->getBoolValue();
	const bool inclstickidx = stickidsfld_->isChecked( 0 );
	const bool inclknotidx = stickidsfld_->isChecked( 1 );
	const int nrsticks = nrSticks( loadedobjs[idx] );
	for ( int stickidx=0; stickidx<nrsticks; stickidx++ )
	{
	    const int nrknots = nrKnots( loadedobjs[idx], stickidx );
	    for ( int knotidx=0; knotidx<nrknots; knotidx++ )
	    {
		const Coord3 crd =
			getCoord( loadedobjs[idx], stickidx, knotidx );
		if ( !crd.isDefined() )
		    continue;
		if ( !issingle_ )
		    ostrm << objnm << "\t";
		if ( !doxy )
		{
		    const BinID bid = SI().transform( crd.getXY() );
		    ostrm << bid.inl() << '\t' << bid.crl();
		}
		else
		{
		    // ostreams print doubles awfully
		    str.setEmpty();
		    str += crd.x_; str += "\t"; str += crd.y_;
		    ostrm << str;
		}

		ostrm << '\t' << unit->userValue( crd.z_ );

		if ( inclstickidx )
		    ostrm << '\t' << stickidx;
		if ( inclknotidx )
		    ostrm << '\t' << knotidx;

		if ( fss )
		{
		    const int sticknr = stickNr( loadedobjs[idx], stickidx );

		    bool pickedon2d =
			fss->geometry().pickedOn2DLine( sticknr );
		    if ( pickedon2d && linenmfld_->isChecked() )
		    {
			Pos::GeomID geomid =
					fss->geometry().pickedGeomID( sticknr );
			const char* linenm = Survey::GM().getName( geomid );

			if ( linenm )
			    ostrm << '\t' << linenm;
		    }
		}

		ostrm << '\n';
	    }
	}
    }

    return true;
}


bool uiExportFault::acceptOK()
{
    BufferStringSet fltnms;
    bool isobjsel(true);
    if ( issingle_ )
	isobjsel = singleinfld_->commitInput();
    else
    {
	bulkinfld_->getChosen(fltnms);
	if ( fltnms.isEmpty() ) isobjsel = false;
    }

    if ( !isobjsel )
	mErrRet( uiStrings::phrSelect(tr("the input fault")) );

    const BufferString outfnm( outfld_->fileName() );
    if ( outfnm.isEmpty() )
	mErrRet( uiStrings::sSelOutpFile() );

    if ( File::exists(outfnm)
      && !uiMSG().askOverwrite(uiStrings::sOutputFileExistsOverwrite()))
	return false;
    bool res = writeAscii();

    if ( !res )	return false;
    const IOObj* ioobj = issingle_ ? ctio_.ioobj_ :
					    bulkinfld_->getCtxtIOObj().ioobj_;

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
