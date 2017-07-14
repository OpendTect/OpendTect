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
#include "ptrman.h"
#include "strmprov.h"
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
#include "ioman.h"
#include "uiunitsel.h"
#include "od_helpids.h"
#include "dbman.h"
#include "emioobjinfo.h"
#include "uiioobjselgrp.h"
#include "unitofmeasure.h"
#include "od_ostream.h"
#include "uit2dconvsel.h"
#include "zaxistransform.h"

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

    uiStringSet zmodes;
    zmodes.add(uiStrings::sYes());
    zmodes.add(uiStrings::sNo());
    zmodes.add(tr("Transformed"));

    zfld_ = new uiGenInput( this, uiStrings::phrOutput( toUiString("Z") ),
			    StringListInpSpec(zmodes) );
    zfld_->valuechanged.notify( mCB(this,uiExportFault,addZChg ) );
    zfld_->attach( alignedBelow, coordfld_ );

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

    if ( mGet(typ,true,false) )
    {
	linenmfld_ = new uiCheckBox( this,
				     tr("Write line name if picked on 2D") );
	linenmfld_->setChecked( true );
	linenmfld_->attach( alignedBelow, stickidsfld_ );
    }

    uiFileSel::Setup fssu; fssu.setForWrite();
    outfld_ = new uiFileSel( this, uiStrings::phrOutput(uiStrings::phrASCII(
			       uiStrings::sFile())), fssu );
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
	mErrRet(tr("Cannot find object in database"))

    const BufferString fname = outfld_->fileName();

    StreamData sdo = StreamProvider( fname ).makeOStream();

    RefMan<ZAxisTransform> zatf = 0;
    if ( zfld_->getIntValue()==2 )
    {
	zatf = transfld_->getSelection();
	if ( !zatf )
	{
	    uiMSG().message(tr("Transform of selected option is "
							"not implemented"));
	    return false;
	}
    }

    uiTaskRunner taskrunner( this );
    PtrMan<Executor> objloader = EM::EMM().objectLoader( dbkeyset );

    if ( !objloader || !TaskRunner::execute(&taskrunner, *objloader) )
    {
	mErrRet( uiStrings::phrCannotRead(tr("the object") ));
	return false;
    }

    const UnitOfMeasure* unit = zunitsel_->getUnit();
    const bool doxy = coordfld_->getBoolValue();
    const bool inclstickidx = stickidsfld_->isChecked( 0 );
    const bool inclknotidx = stickidsfld_->isChecked( 1 );

    for ( int idx=0; idx<dbkeyset.size(); idx++ )
    {
	EM::ObjectID objid = EM::EMM().getObjectID( dbkeyset[idx] );
	if ( objid < 0 ) continue;
	RefMan<EM::EMObject> emobj = EM::EMM().getObject(objid);
	emobj->setDBKey( dbkeyset.get(idx) );
	mDynamicCastGet(EM::Fault3D*,f3d,emobj.ptr())
	mDynamicCastGet(EM::FaultStickSet*,fss,emobj.ptr())
	if ( !f3d && !fss ) return false;

	PtrMan<Executor> loader = emobj->loader();
	if ( !loader ) mErrRet( uiStrings::phrCannotRead(
							uiStrings::sFault() ))

	uiTaskRunner taskrnnr( this );
	if ( !TaskRunner::execute( &taskrnnr, *loader ) ) return false;


	if ( !sdo.usable() )
	{
	    sdo.close();
	    mErrRet( uiStrings::sCantOpenOutpFile() );
	}
	const EM::SectionID sectionid = emobj->sectionID( 0 );
	const int nrsticks = nrSticks( emobj.ptr(), sectionid );

	BufferString objnm = f3d ? f3d->name() : fss->name();

	BufferString str;

	TrcKeyZSampling bbox(true);
	bool first = true;
	int zatvoi = -1;
	if ( zatf && zatf->needsVolumeOfInterest() ) //Get BBox
	{
	    for ( int stickidx=0; stickidx<nrsticks; stickidx++ )
	    {
		const int nrknots = nrKnots( emobj, sectionid, stickidx );
		for ( int knotidx=0; knotidx<nrknots; knotidx++ )
		{
		    Coord3 crd = getCoord( emobj, sectionid,
						stickidx, knotidx );
		    if ( !crd.isDefined() )
			continue;
		    const BinID bid = SI().transform( crd.getXY() );
		    if ( first )
		    {
			first = false;
			bbox.hsamp_.start_ = bbox.hsamp_.stop_ = bid;
			bbox.zsamp_.start = bbox.zsamp_.stop = (float) crd.z_;
		    }
		    else
		    {
			bbox.hsamp_.include( bid );
			bbox.zsamp_.include( (float) crd.z_ );
		    }
		}
	    }

	    uiTaskRunner taskrnr( this );
	    if ( bbox.isDefined() )
	    {
		if ( zatvoi == -1 )
		    zatvoi = zatf->addVolumeOfInterest( bbox, false );
		else
		    zatf->setVolumeOfInterest( zatvoi, bbox, false );
		if ( zatvoi>=0 )
			zatf->loadDataIfMissing( zatvoi, &taskrnr );
	    }
	}




	for ( int stickidx=0; stickidx<nrsticks; stickidx++ )
	{
	    const int nrknots = nrKnots( emobj.ptr(), sectionid, stickidx );
	    for ( int knotidx=0; knotidx<nrknots; knotidx++ )
	    {
		Coord3 crd = getCoord( emobj.ptr(), sectionid,
					     stickidx, knotidx );
		if ( !crd.isDefined() )
		    continue;
		if ( !issingle_ )
		    *sdo.oStrm() << objnm << "\t";
		const BinID bid = SI().transform( crd.getXY() );
		if ( zatf )
		    crd.z_ =  zatf->transformTrc( bid, (float)crd.z_ );
		if ( !doxy )
		{
		    *sdo.oStrm() << bid.inl() << '\t' << bid.crl();
		}
		else
		{
		    // ostreams print doubles awfully
		    str.setEmpty();
		    str += crd.x_; str += "\t"; str += crd.y_;
		    *sdo.oStrm() << str;
		}

		*sdo.oStrm() << '\t' << unit->userValue( crd.z_ );

		if ( inclstickidx )
		    *sdo.oStrm() << '\t' << stickidx;
		if ( inclknotidx )
		    *sdo.oStrm() << '\t' << knotidx;

		if ( fss )
		{
		    const int sticknr = stickNr( emobj.ptr(), sectionid,
								    stickidx );

		    bool pickedon2d =
			fss->geometry().pickedOn2DLine( sectionid, sticknr );
		    if ( pickedon2d && linenmfld_->isChecked() )
		    {
			Pos::GeomID geomid =
			    fss->geometry().pickedGeomID( sectionid, sticknr );
			const char* linenm = Survey::GM().getName( geomid );

			if ( linenm )
			    *sdo.oStrm() << '\t' << linenm;
		    }
		}

		*sdo.oStrm() << '\n';
	    }
	}

    }
    sdo.close();

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
