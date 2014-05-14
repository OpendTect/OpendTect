/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		May 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiseisrandto2dline.h"

#include "ctxtioobj.h"
#include "linekey.h"
#include "randomlinegeom.h"
#include "randomlinetr.h"
#include "seisrandlineto2d.h"
#include "seistrctr.h"
#include "seisioobjinfo.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "posinfo2d.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uiseissel.h"
#include "uiseislinesel.h"
#include "uitaskrunner.h"
#include "od_helpids.h"


uiSeisRandTo2DBase::uiSeisRandTo2DBase( uiParent* p, bool rdlsel )
    : uiGroup(p,"Base group")
    , rdlfld_(0)
    , change(this)
{
    if ( rdlsel )
    {
	rdlfld_ = new uiIOObjSel( this, mIOObjContext(RandomLineSet),
				  "Input RandomLine" );
	rdlfld_->selectionDone.notify( mCB(this,uiSeisRandTo2DBase,selCB) );
    }

    IOObjContext ctxt( mIOObjContext(SeisTrc) );
    inpfld_ = new uiSeisSel( this, ctxt, uiSeisSel::Setup(Seis::Vol) );
    inpfld_->selectionDone.notify( mCB(this,uiSeisRandTo2DBase,selCB) );
    if ( rdlfld_ ) inpfld_->attach( alignedBelow, rdlfld_ );

    IOObjContext ctxt2d( mIOObjContext(SeisTrc) );
    ctxt2d.forread = false;
    outpfld_ = new uiSeisSel( this, ctxt2d, uiSeisSel::Setup(Seis::Line) );
    outpfld_->setConfirmOverwrite( false );
    outpfld_->attach( alignedBelow, inpfld_ );
    setHAlignObj( outpfld_ );
}


uiSeisRandTo2DBase::~uiSeisRandTo2DBase()
{}


void uiSeisRandTo2DBase::selCB( CallBacker* )
{ change.trigger(); }


#define mErrRet(s) { uiMSG().error(s); return false; }
bool uiSeisRandTo2DBase::checkInputs()
{
    if ( rdlfld_ && !rdlfld_->ioobj() )
	return false;

    if ( !inpfld_->ioobj() || !outpfld_->ioobj() )
	return false;

    return true;
}


const IOObj* uiSeisRandTo2DBase::getInputIOObj() const
{ return inpfld_->ioobj(true); }

const IOObj* uiSeisRandTo2DBase::getOutputIOObj() const
{ return outpfld_->ioobj(true); }


bool uiSeisRandTo2DBase::getRandomLineGeom( Geometry::RandomLineSet& geom) const
{
    if ( !rdlfld_ || !rdlfld_->ioobj(true) )
	return false;

    BufferString msg;
    if ( !RandomLineSetTranslator::retrieve(geom,rdlfld_->ioobj(),msg) )
	mErrRet( msg );

    return true;
}


uiSeisRandTo2DLineDlg::uiSeisRandTo2DLineDlg( uiParent* p,
					      const Geometry::RandomLine* rln )
    : uiDialog(p,uiDialog::Setup("Save as 2D line","",
                                 mODHelpKey(mSeisRandTo2DLineDlgHelpID) ))
    , rdlgeom_(rln)
{
    basegrp_ = new uiSeisRandTo2DBase( this, !rln );

    linenmfld_ = new uiSeis2DLineNameSel( this, false );
    linenmfld_->attach( alignedBelow, basegrp_ );

    trcnrfld_ = new uiGenInput( this, "First Trace Nr", IntInpSpec(1) );
    trcnrfld_->attach( alignedBelow, linenmfld_ );
}


bool uiSeisRandTo2DLineDlg::acceptOK( CallBacker* )
{
    if ( !basegrp_->checkInputs() )
	return false;

    const BufferString linenm = linenmfld_->getInput();
    if ( linenm.isEmpty() )
	mErrRet( tr("Please enter a Line Name") )

    const int trcnrstart = trcnrfld_->getIntValue();
    if ( mIsUdf(trcnrstart) || trcnrstart <= 0 )
	mErrRet( tr("Please specify a valid start trace number") )

    Geometry::RandomLineSet geom;
    const Geometry::RandomLine* rdl = rdlgeom_;
    if ( !rdl )
    {
	basegrp_->getRandomLineGeom( geom );
	rdl = geom.isEmpty() ? 0 : geom.lines()[0];
    }
    if ( !rdl )
	mErrRet( tr("Selected Random line is empty") );

    Pos::GeomID geomid = Survey::GM().getGeomID( linenm );
    if ( geomid != Survey::GeometryManager::cUndefGeomID() )
    {
	uiString msg = tr("The 2D Line '%1' already exists. If you overwrite "
			  "its geometry, all the associated data will be "
			  "affected. Do you still want to overwrite?")
			.arg(linenm);
	if ( !uiMSG().askOverwrite(msg) )
	    return false;

	mDynamicCastGet( Survey::Geometry2D*, geom2d,
			 Survey::GMAdmin().getGeometry(geomid) );
	if ( !geom2d )
	    return false;

	geom2d->dataAdmin().setEmpty();
	geom2d->touch();
    }
    else
    {
	Survey::Geometry2D* newgeom = new Survey::Geometry2D;
	newgeom->dataAdmin().setLineName( linenm );
	uiString msg;
	geomid = Survey::GMAdmin().addNewEntry( newgeom, msg );
    }

    const IOObj* inobj = basegrp_->getOutputIOObj();
    const IOObj* outobj = basegrp_->getOutputIOObj();
    SeisRandLineTo2D exec( *inobj, *outobj, geomid, trcnrstart, *rdl );
    uiTaskRunner dlg( this );
    if ( !TaskRunner::execute( &dlg, exec ) )
	return false;
    
    if ( !SI().has2D() )
	uiMSG().warning( tr("You need to change survey type to 'Both 2D and 3D'"
			 " in survey setup to display the 2D line") );

    return true;
}

#undef mErrRet

