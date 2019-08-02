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
#include "emfaultset3d.h"
#include "lmkemfaulttransl.h"
#include "emmanager.h"
#include "executor.h"
#include "file.h"
#include "ioobj.h"
#include "od_ostream.h"
#include "ptrman.h"
#include "survgeom.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "uibutton.h"
#include "uichecklist.h"
#include "uifilesel.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uistrings.h"
#include "uitaskrunner.h"
#include "od_helpids.h"
#include "uiunitsel.h"
#include "od_helpids.h"
#include "emioobjinfo.h"
#include "uiioobjselgrp.h"
#include "unitofmeasure.h"
#include "od_ostream.h"
#include "uit2dconvsel.h"
#include "zaxistransform.h"

#define mGetObjNr \
    issingle ? 1 : mPlural \

#define mGet( tp, fss, f3d, fset ) \
    FixedString(tp) == EMFaultStickSetTranslatorGroup::sGroupName() ? fss : \
    (FixedString(tp) == EMFaultSet3DTranslatorGroup::sGroupName() ? fset : f3d )

#define mGetCtio(tp) \
    mGet( tp, *mMkCtxtIOObj(EMFaultStickSet), *mMkCtxtIOObj(EMFault3D), \
	    *mMkCtxtIOObj(EMFaultSet3D) )
#define mGetTitle(tp) \
    mGet( tp, uiStrings::phrExport( uiStrings::sFaultStickSet() ), \
	      uiStrings::phrExport( uiStrings::sFault() ), \
	      uiStrings::phrExport( uiStrings::sFaultSet() ) )

#define mGetLbl(tp) \
    mGet( tp, uiStrings::phrInput( uiStrings::sFaultStickSet() ), \
	      uiStrings::phrInput( uiStrings::sFault() ), \
	      uiStrings::phrInput( uiStrings::sFaultSet() ) )

#define mGetDispString(typ) \
    dispstr_ = mGet( typ, uiStrings::sFaultStickSet(mGetObjNr), \
			  uiStrings::sFault(mGetObjNr), \
			  uiStrings::sFaultSet(mGetObjNr) )

uiExportFault::uiExportFault( uiParent* p, const char* typ, bool issingle )
    : uiDialog(p,uiDialog::Setup(mGetTitle(typ),mNoDlgTitle,
				 mGet(typ,mODHelpKey(mExportFaultStickHelpID),
				 mODHelpKey(mExportFaultHelpID),mTODOHelpKey) ))
    , ctio_(mGetCtio(typ))
    , linenmfld_(0)
    , issingle_(issingle)
    , singleinfld_(0)
    , bulkinfld_(0)
{
    mGetDispString(typ);
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
    mAttachCB( coordfld_->valuechanged, uiExportFault::exportCoordSysChgCB );


    coordsysselfld_ = new Coords::uiCoordSystemSel( this );
    coordsysselfld_->attach( alignedBelow, coordfld_);
    coordsysselfld_->display(false);

    uiStringSet zmodes;
    zmodes.add(uiStrings::sYes());
    zmodes.add(uiStrings::sNo());
    zmodes.add(tr("Transformed"));

    zfld_ = new uiGenInput( this, uiStrings::phrOutput( uiStrings::sZ() ),
			    StringListInpSpec(zmodes) );
    zfld_->valuechanged.notify( mCB(this,uiExportFault,addZChg ) );
    zfld_->attach( alignedBelow, coordsysselfld_ );

    uiT2DConvSel::Setup stup( 0, false );
    stup.ist2d( SI().zIsTime() );
    transfld_ = new uiT2DConvSel( this, stup );
    transfld_->display( false );
    transfld_->attach( alignedBelow, zfld_ );

    uiUnitSel::Setup unitselsu( PropertyRef::surveyZType(), tr("Z in") );
    zunitsel_ = new uiUnitSel( this, unitselsu );
    zunitsel_->attach( alignedBelow, transfld_ );

    stickidsfld_ = new uiCheckList( this, uiCheckList::ChainAll,
				    OD::Horizontal );
    stickidsfld_->setLabel( uiStrings::sWrite() );
    stickidsfld_->addItem( tr("Stick index") ).addItem( tr("Node index" ));
    stickidsfld_->setChecked( 0, true ); stickidsfld_->setChecked( 1, true );
    stickidsfld_->attach( alignedBelow, zunitsel_ );

    if ( mGet(typ,true,false,false) )
    {
	linenmfld_ = new uiCheckBox( this,
				     tr("Write line name if picked on 2D") );
	linenmfld_->setChecked( true );
	linenmfld_->attach( alignedBelow, stickidsfld_ );
    }

    uiFileSel::Setup fssu; fssu.setForWrite();
    outfld_ = new uiFileSel( this,
		    uiStrings::phrOutput(uiStrings::sASCIIFile()), fssu );
    if ( linenmfld_ )
	outfld_->attach( alignedBelow, linenmfld_ );
    else
	outfld_->attach( alignedBelow, stickidsfld_ );
    exportCoordSysChgCB(0);
}


uiExportFault::~uiExportFault()
{
    delete ctio_.ioobj_; delete &ctio_;
}


static int stickNr( EM::Object* emobj, int stickidx )
{
    mDynamicCastGet(Geometry::FaultStickSet*,fss,emobj->geometryElement());
    return fss->rowRange().atIndex( stickidx );
}


static int nrSticks( EM::Object* emobj )
{
    mDynamicCastGet(Geometry::FaultStickSet*,fss,emobj->geometryElement());
    return fss->nrSticks();
}


static int nrKnots( EM::Object* emobj, int stickidx )
{
    mDynamicCastGet(Geometry::FaultStickSet*,fss,emobj->geometryElement());
    const int sticknr = fss->rowRange().atIndex( stickidx );
    return fss->nrKnots( sticknr );
}


Coord3 uiExportFault::getCoord( EM::Object* emobj, int stickidx,
			int knotidx )
{
    mDynamicCastGet(Geometry::FaultStickSet*,fss,emobj->geometryElement());
    if ( !fss )
	return Coord3::udf();
    const int sticknr = fss->rowRange().atIndex(stickidx);
    const int knotnr = fss->colRange(sticknr).atIndex(knotidx);
    Coord3 crd = fss->getKnot( RowCol(sticknr,knotnr) );
    return crd;
}


#define mErrRet(s) { uiMSG().error(s); return false; }


void uiExportFault::addZChg( CallBacker* )
{
    transfld_->display( zfld_->getIntValue()==2 );

    const bool displayunit = zfld_->getIntValue()!=1;
    if ( displayunit )
    {
	FixedString zdomain = getZDomain();
	if ( zdomain==ZDomain::sKeyDepth() )
	    zunitsel_->setPropType( PropertyRef::Dist );
	else if ( zdomain==ZDomain::sKeyTime() )
	{
	    zunitsel_->setPropType( PropertyRef::Time );
	    zunitsel_->setUnit( "Milliseconds" );
	}
    }

    zunitsel_->display( displayunit );
}


void uiExportFault::exportCoordSysChgCB(CallBacker*)
{
    const bool shoulddisplay = SI().getCoordSystem() &&
				SI().getCoordSystem()->isProjection() &&
						    coordfld_->getBoolValue();
    coordsysselfld_->display(shoulddisplay);
}


FixedString uiExportFault::getZDomain() const
{
    FixedString zdomain = ZDomain::SI().key();

    if ( zfld_->getIntValue()==2 )
    {
	zdomain = transfld_->selectedToDomain();
    }

    return zdomain;
}


bool uiExportFault::getInputDBKeys( DBKeySet& dbkeyset )
{
    if ( issingle_ )
    {
	const IOObj* ioobj = ctio_.ioobj_;
	if ( !ioobj ) return false;
	dbkeyset.add(ioobj->key());
    }
    else
	bulkinfld_->getChosen(dbkeyset);
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

    RefMan<ZAxisTransform> zatf = 0;
    if ( zfld_->getIntValue()==2 )
    {
	zatf = transfld_->getSelection();
	if ( !zatf )
	    { uiMSG().error( mINTERNAL("Transform not impl") ); return false; }
    }

    uiTaskRunnerProvider trprov( this );
    RefObjectSet<EM::Object> loadedobjs = EM::MGR().loadObjects(
							    dbkeyset, trprov );

    if ( loadedobjs.isEmpty() )
	return false;

    const UnitOfMeasure* unit = zunitsel_->getUnit();
    const bool doxy = coordfld_->getBoolValue();
    const bool inclstickidx = stickidsfld_->isChecked( 0 );
    const bool inclknotidx = stickidsfld_->isChecked( 1 );


    for ( int idx=0; idx<loadedobjs.size(); idx++ )
    {
	EM::Object* emobj = loadedobjs[idx];
	mDynamicCastGet(EM::Fault3D*,f3d,emobj)
	mDynamicCastGet(EM::FaultStickSet*,fss,emobj)
	mDynamicCastGet(EM::FaultSet3D*,fset,emobj)
	if ( !f3d && !fss && !fset ) return false;

	const int nrobjs = fset ? fset->nrFaults() : 1;
	for ( int oidx=0; oidx<nrobjs; oidx++ )
	{
	    EM::Object* fltobj = emobj;
	    BufferString objnm = fltobj->name();

	    if ( fset )
	    {
		EM::FaultID fltid = fset->getFaultID( oidx );
		fltobj = fset->getFault3D( fltid );
		objnm = fset->name();
		objnm.add("_").add( fltid );
	    }
	    objnm.quote( '\"' );

	    const int nrsticks = nrSticks( fltobj );

	    BufferString str;

	    TrcKeyZSampling bbox(true);
	    bool first = true;
	    int zatvoi = -1;
	    if ( zatf && zatf->needsVolumeOfInterest() ) //Get BBox
	    {
		for ( int stickidx=0; stickidx<nrsticks; stickidx++ )
		{
		    const int nrknots = nrKnots( fltobj, stickidx );
		    for ( int knotidx=0; knotidx<nrknots; knotidx++ )
		    {
			Coord3 crd = getCoord( fltobj, stickidx, knotidx );
			if ( !crd.isDefined() )
			    continue;
			const BinID bid = SI().transform( crd.getXY() );
			if ( first )
			{
			    first = false;
			    bbox.hsamp_.start_ = bbox.hsamp_.stop_ = bid;
			    bbox.zsamp_.start = bbox.zsamp_.stop =
								(float) crd.z_;
			}
			else
			{
			    bbox.hsamp_.include( bid );
			    bbox.zsamp_.include( (float) crd.z_ );
			}
		    }
		}

		if ( bbox.isDefined() )
		{
		    if ( zatvoi == -1 )
			zatvoi = zatf->addVolumeOfInterest( bbox, false );
		    else
			zatf->setVolumeOfInterest( zatvoi, bbox, false );
		    if ( zatvoi>=0 )
			zatf->loadDataIfMissing( zatvoi, trprov );
		}
	    }

	    for ( int stickidx=0; stickidx<nrsticks; stickidx++ )
	    {
		const int nrknots = nrKnots( fltobj, stickidx );
		for ( int knotidx=0; knotidx<nrknots; knotidx++ )
		{
		    Coord3 crd = getCoord( fltobj, stickidx, knotidx );
		    if ( !crd.isDefined() )
			continue;
		    if ( coordsysselfld_->isDisplayed() )
			crd.setXY(
				coordsysselfld_->getCoordSystem()->convertFrom(
					crd.getXY(),*SI().getCoordSystem()));
		    if ( !issingle_ || nrobjs > 1 )
			ostrm << objnm << "\t";
		    const BinID bid = SI().transform( crd.getXY() );
		    if ( zatf )
			crd.z_ =
			    zatf->transformTrc( TrcKey(bid), (float)crd.z_ );
		    if ( !doxy )
		    {
			ostrm << bid.inl() << '\t' << bid.crl();


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
			const int sticknr = stickNr( fltobj, stickidx );

			bool pickedon2d =
			    fss->geometry().pickedOn2DLine( sticknr );
			if ( pickedon2d && linenmfld_->isChecked() )
			{
			    Pos::GeomID geomid =
					fss->geometry().pickedGeomID( sticknr );
			    const BufferString linenm = geomid.name();

			    if ( !linenm.isEmpty() )
				ostrm << '\t' << linenm;
			}
		    }

		    ostrm << '\n';
		}
	    }
	}
    }

    return true;
}


bool uiExportFault::acceptOK()
{
    if ( zfld_->getIntValue()==2 )
    {
	if ( !transfld_->acceptOK() )
	    return false;
    }

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
	mErrRet( uiStrings::phrSelOutpFile() );

    if ( File::exists(outfnm)
      && !uiMSG().askOverwrite(uiStrings::phrOutputFileExistsOverwrite()))
	return false;

    if ( !writeAscii() )
    {
	uiMSG().error(uiStrings::phrCannotCreateDBEntryFor(
						    tr("selected faults")));
	return false;
    }

    uiString msg = tr("%1 successfully exported.\n\n"
		    "Do you want to export more %1?").arg(dispstr_);
    bool ret = uiMSG().askGoOn( msg, uiStrings::sYes(),
				tr("No, close window") );
    return !ret;
}
