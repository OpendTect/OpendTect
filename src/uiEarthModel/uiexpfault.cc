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
#include "ioman.h"
#include "uiunitsel.h"
#include "emioobjinfo.h"
#include "uiioobjselgrp.h"
#include "unitofmeasure.h"
#include "od_ostream.h"
#include "uit2dconvsel.h"
#include "zaxistransform.h"
#include "hiddenparam.h"

HiddenParam<uiExportFault,FixedString*> typ_(0);

#define mGetObjNr \
    issingle ? 1 : mPlural \

#define mGet( tp, fss, f3d ) \
    FixedString(tp) == EMFaultStickSetTranslatorGroup::sGroupName() ? fss : f3d

#define mGetCtio(tp) \
    mGet( tp, *mMkCtxtIOObj(EMFaultStickSet), *mMkCtxtIOObj(EMFault3D) )
#define mGetTitle(tp) \
    mGet( tp, uiStrings::phrExport( uiStrings::sFaultStickSet(mGetObjNr) ), \
	      uiStrings::phrExport( uiStrings::sFault(mGetObjNr) ) )

#define mGetLbl(tp) \
    mGet( tp, uiStrings::phrInput( uiStrings::sFaultStickSet(mGetObjNr) ), \
	      uiStrings::phrInput( uiStrings::sFault(mGetObjNr) ) )

uiExportFault::uiExportFault( uiParent* p, const char* typ, bool issingle )
    : uiDialog(p,uiDialog::Setup(mGetTitle(typ),mNoDlgTitle,
				 mGet(typ,mODHelpKey(mExportFaultStickHelpID),
				 mODHelpKey(mExportFaultHelpID)) ))
    , ctio_(mGetCtio(typ))
    , linenmfld_(0)
    , issingle_(issingle)
    , infld_(0)
    , bulkinfld_(0)
{
    typ_.setParam( this, new FixedString(typ) );
    setModal( false );
    setDeleteOnClose( false );
    setOkCancelText( uiStrings::sExport(), uiStrings::sClose() );

    uiIOObjSelGrp::Setup su; su.choicemode_ = OD::ChooseAtLeastOne;
    if ( issingle_ )
	infld_ = new uiIOObjSel( this, ctio_, mGetLbl(typ) );
    else
	bulkinfld_ = new uiIOObjSelGrp( this, ctio_, mGetLbl(typ), su );

    coordfld_ = new uiGenInput( this, tr("Write coordinates as"),
				BoolInpSpec(true,tr("X/Y"),tr("Inl/Crl")) );
    if ( issingle_ )
	coordfld_->attach( alignedBelow, infld_ );
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

    outfld_ = new uiFileInput( this, uiStrings::phrOutput(uiStrings::phrASCII(
			       uiStrings::sFile())),
			       uiFileInput::Setup().forread(false) );
    if ( linenmfld_ )
	outfld_->attach( alignedBelow, linenmfld_ );
    else
	outfld_->attach( alignedBelow, stickidsfld_ );

    dispstr_ = EMFaultStickSetTranslatorGroup::sGroupName() == typ
				? uiStrings::sFaultStickSet(mGetObjNr)
				: uiStrings::sFault(mGetObjNr);	
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


bool uiExportFault::getInputMIDs( TypeSet<MultiID>& midset )
{
    if ( issingle_ )
    {
	const IOObj* ioobj = ctio_.ioobj_;
	if ( !ioobj ) return false;
	MultiID mid = ioobj->key();
	midset.add(mid);
    }
    else
	bulkinfld_->getChosen( midset );

    return true;
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiExportFault::writeAscii()
{
    TypeSet<MultiID> midset;
    if ( !getInputMIDs(midset) )
	mErrRet(tr("Cannot find object in database"))
    const BufferString fname = outfld_->fileName();


    od_ostream ostrm( fname );
    if ( !ostrm.isOK() )
	return false;

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
    PtrMan<Executor> objloader = EM::EMM().objectLoader( midset );

    if ( objloader && !TaskRunner::execute(&taskrunner, *objloader) )
	return false;

    const UnitOfMeasure* unit = zunitsel_->getUnit();
    const bool doxy = coordfld_->getBoolValue();
    const bool inclstickidx = stickidsfld_->isChecked( 0 );
    const bool inclknotidx = stickidsfld_->isChecked( 1 );

    for ( int idx=0; idx<midset.size(); idx++ )
    {
	EM::ObjectID objid = EM::EMM().getObjectID( midset[idx] );
	if ( objid < 0 ) continue;
	EM::EMObject* emobj = EM::EMM().getObject(objid);
	mDynamicCastGet(EM::Fault3D*,f3d,emobj)
	mDynamicCastGet(EM::FaultStickSet*,fss,emobj)
	if ( !f3d && !fss ) return false;
	const EM::SectionID sectionid = emobj->sectionID( 0 );
	const int nrsticks = nrSticks( emobj, sectionid );

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
		    const Coord3 crd = getCoord( emobj, sectionid,
						stickidx, knotidx );
		    if ( !crd.isDefined() )
			continue;
		    const TrcKey tk( bbox.hsamp_.toTrcKey(crd) );
		    const BinID& bid = tk.position();
		    if ( first )
		    {
			first = false;
			bbox.hsamp_.start_ = bbox.hsamp_.stop_ = bid;
			bbox.zsamp_.start = bbox.zsamp_.stop = (float) crd.z;
		    }
		    else
		    {
			bbox.hsamp_.include( bid );
			bbox.zsamp_.include( (float) crd.z );
		    }
		}
	    }

	    uiTaskRunner taskr( this );
	    if ( bbox.isDefined() )
	    {
		if ( zatvoi == -1 )
		    zatvoi = zatf->addVolumeOfInterest( bbox, false );
		else
		    zatf->setVolumeOfInterest( zatvoi, bbox, false );
		if ( zatvoi>=0 )
			zatf->loadDataIfMissing( zatvoi, &taskr );
	    }
	}

	for ( int stickidx=0; stickidx<nrsticks; stickidx++ )
	{
	    const int nrknots = nrKnots( emobj, sectionid, stickidx );
	    for ( int knotidx=0; knotidx<nrknots; knotidx++ )
	    {
		Coord3 crd = getCoord( emobj, sectionid,
					    stickidx, knotidx );
		if ( !crd.isDefined() )
		    continue;
		if ( !issingle_ )
		    ostrm << "\""<< objnm <<"\"" << "\t";

		const TrcKey tk( bbox.hsamp_.toTrcKey(crd) );
		const BinID& bid = tk.position();

		if ( zatf )
		    crd.z =  zatf->transformTrc( bid, (float)crd.z );

		if ( !doxy )
		{
		    ostrm << bid.inl() << '\t' << bid.crl();
		}
		else
		{
		    // ostreams print doubles awfully
		    str.setEmpty();
		    str += crd.x; str += "\t"; str += crd.y;
		    ostrm << str;
		}

		ostrm << '\t' << unit->userValue( crd.z );

		if ( inclstickidx )
		    ostrm << '\t' << stickidx;
		if ( inclknotidx )
		    ostrm << '\t' << knotidx;

		if ( fss )
		{
		    const int sticknr = stickNr( emobj, sectionid, stickidx );
		    bool pickedon2d =
			fss->geometry().pickedOn2DLine( sectionid, sticknr );
		    if ( pickedon2d && linenmfld_->isChecked() )
		    {
			Pos::GeomID geomid =
			    fss->geometry().pickedGeomID( sectionid, sticknr );
			const char* linenm = Survey::GM().getName( geomid );

			if ( linenm )
			    ostrm << '\t' << linenm;
		    }
		}

		ostrm << '\n';
	  }
      }

    }
    ostrm.close();

    return true;
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


bool uiExportFault::acceptOK( CallBacker* )
{
    if ( zfld_->getIntValue()==2 )
    {
	if ( !transfld_->acceptOK() )
	    return false;
    }

    BufferStringSet fltnms;
    bool isobjsel(true);

    if ( issingle_ )
	isobjsel = infld_->commitInput();
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

    if ( !writeAscii() )
    {
	uiMSG().error(uiStrings::phrCannotCreateDBEntryFor(
						    tr("selected faults")));
	return false;
    }
    uiString msg = tr( "%1 successfully exported.\n\n"
		    "Do you want to export more %2?" ).arg(dispstr_)
		    .arg(*typ_.getParam(this) ==
				EMFaultStickSetTranslatorGroup::sGroupName()
				? uiStrings::sFaultStickSet(mPlural)
				: uiStrings::sFault(mPlural));
    bool ret = uiMSG().askGoOn( msg, uiStrings::sYes(),
				tr("No, close window") );
    return !ret;
}
