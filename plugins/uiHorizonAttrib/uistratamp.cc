/*+
   * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
   * AUTHOR   : Nageswara
   * DATE     : Mar 2008
 -*/

static const char* rcsID = "$Id: uistratamp.cc,v 1.16 2010-12-07 22:59:52 cvskris Exp $";

#include "uistratamp.h"
#include "stratamp.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribdescsetsholder.h"
#include "attriboutput.h"
#include "cubesampling.h"
#include "emhorizon3d.h"
#include "emioobjinfo.h"
#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "emsurfaceiodata.h"
#include "emsurfacetr.h"
#include "ioobj.h"
#include "survinfo.h"
#include "uiattrsel.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiioobjsel.h"
#include "uipossubsel.h"
#include "uitaskrunner.h"


static const char* statstrs[] = { "Min", "Max", "Average", "RMS", "Sum", 0 };

uiStratAmpCalc::uiStratAmpCalc( uiParent* p )
    : uiDialog( p, Setup("Stratal Amplitude","Specify process parameters",
			 "104.4.3") )
    , horctio1_(*mMkCtxtIOObj(EMHorizon3D))
    , horctio2_(*mMkCtxtIOObj(EMHorizon3D))
{
    const Attrib::DescSet* ads = Attrib::DSHolder().getDescSet(false,false);
    inpfld_ = new uiAttrSel( this, *ads, "Quantity to output" );
    inpfld_->selectionDone.notify( mCB(this,uiStratAmpCalc,inpSel) );

    winoption_= new uiGenInput( this, "Window Option",
	                        BoolInpSpec(true, "Single Horizon",
				"Double Horizon") );
    winoption_->valuechanged.notify( mCB(this,uiStratAmpCalc,choiceSel) );
    winoption_->attach( alignedBelow, inpfld_ );

    horfld1_ = new uiIOObjSel( this, horctio1_, "    Horizon" );
    horfld1_->selectionDone.notify( mCB(this,uiStratAmpCalc,inpSel) );
    horfld1_->attach( alignedBelow, winoption_ );

    horfld2_ = new uiIOObjSel( this, horctio2_, "Bottom Horizon" );
    horfld2_->selectionDone.notify( mCB(this,uiStratAmpCalc,inpSel) );
    horfld2_->attach( alignedBelow, horfld1_ );

    BufferString lbltxt = "Z Offset ";
    lbltxt += SI().getZUnitString(); lbltxt += " Top";
    tophorshiftfld_ = new uiGenInput( this, lbltxt,
	    			            FloatInpSpec(0).setName("Top") );
    tophorshiftfld_->attach( alignedBelow, horfld2_ );
    tophorshiftfld_->setElemSzPol( uiObject::Small );
    bothorshiftfld_ = new uiGenInput( this, "Bottom", FloatInpSpec(0) );
    bothorshiftfld_->attach( rightTo, tophorshiftfld_ );
    bothorshiftfld_->setElemSzPol( uiObject::Small );

    rangefld_= new uiPosSubSel( this, uiPosSubSel::Setup(false,false) );
    rangefld_->attach( alignedBelow, tophorshiftfld_ );

    ampoptionfld_ = new uiLabeledComboBox( this, statstrs, "Amplitude Option" );
    ampoptionfld_->attach( alignedBelow, rangefld_ );

    selfld_= new uiGenInput( this, "Add result as an attribute to",
			     BoolInpSpec(true,"Top Horizon","Bottom Horizon") );
    selfld_->attach( alignedBelow, ampoptionfld_ );

    attribnamefld_ = new uiGenInput( this, "Attribute name",
			             StringInpSpec("Stratal Amplitude") );
    attribnamefld_->attach( alignedBelow, selfld_ );

    finaliseDone.notify( mCB(this,uiStratAmpCalc,choiceSel) );
}


uiStratAmpCalc::~uiStratAmpCalc()
{
    delete horctio1_.ioobj; delete &horctio1_;
    delete horctio2_.ioobj; delete &horctio2_;
}


void uiStratAmpCalc::choiceSel( CallBacker* )
{
    usesingle_ = winoption_->getBoolValue();
    horfld1_->setLabelText( usesingle_ ? "    Horizon" 
				       : "Top Horizon" );
    horfld2_->display( !usesingle_ );
    selfld_->display( !usesingle_ );
}


void uiStratAmpCalc::inpSel( CallBacker* )
{
    HorSampling hs;
    getAvailableRange( hs );
    CubeSampling incs( rangefld_->envelope() );
    incs.hrg = hs;
    rangefld_->setInput( incs );
}


void uiStratAmpCalc::getAvailableRange( HorSampling& hs )
{
    CubeSampling cs;
    if ( inpfld_->getRanges(cs) )
	hs.limitTo( cs.hrg );

    if ( horfld1_->commitInput() )
    {
	EM::IOObjInfo eminfo( horctio1_.ioobj->key() );
	HorSampling emhs;
	emhs.set( eminfo.getInlRange(), eminfo.getCrlRange() );
	hs.limitTo( emhs );
    }

    if ( horfld2_->commitInput() )
    {
	EM::IOObjInfo eminfo( horctio2_.ioobj->key() );
	HorSampling emhs;
	emhs.set( eminfo.getInlRange(), eminfo.getCrlRange() );
	hs.limitTo( emhs );
    }
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiStratAmpCalc::checkInpFlds()
{
    if ( inpfld_->isEmpty() )
	mErrRet( "Missing Input\nPlease select the input attribute / seismics");
    
    if ( usesingle_ && !horfld1_->commitInput() )
	mErrRet( "Missing Input\nPlease select the input Horizon" );
    if ( !usesingle_ )
    {
	if ( !horfld1_->commitInput() || !horfld2_->commitInput() )
	    mErrRet( "Missing Input\nPlease Check Top / Bottom Horizon" );
    }

    if ( !usesingle_ && horctio1_.ioobj->key() == horctio2_.ioobj->key() )
	      mErrRet( "Select Two Different Horizons" );

    return true;
}


bool uiStratAmpCalc::acceptOK( CallBacker* )
{
    if ( !checkInpFlds() ) return false;

    const bool addtotop = usesingle_ || selfld_->getBoolValue();
    EM::IOObjInfo eminfo( addtotop ? horctio1_.ioobj->key()
	    			   : horctio2_.ioobj->key() );
    BufferStringSet attrnms;
    eminfo.getAttribNames( attrnms );
    const char* attribnm = attribnamefld_->text();
    bool overwrite = false;
    if ( attrnms.indexOf( attribnm ) >= 0 )
    {
	BufferString errmsg = "Attribute name ";
	errmsg += attribnm;
	errmsg += " already exists, Overwrite?";
        
	if ( !uiMSG().askOverwrite(errmsg) )
	    return false;
	else overwrite = true;
    }
 
    HorSampling hs;
    getAvailableRange( hs );
    HorSampling inhs = rangefld_->envelope().hrg;
    hs.limitTo( inhs );
    const EM::Horizon3D* tophor = loadHor( horctio1_.ioobj, hs );
    if ( !tophor )
    {
	uiMSG().error( "Error loading horizon ", horctio1_.ioobj->name() );
	return false;
    }

    const EM::Horizon3D* bothor = usesingle_ ? 0 : loadHor(horctio2_.ioobj,hs);
    if ( !usesingle_ && !bothor )
    {
	if ( tophor ) tophor->unRef();
	uiMSG().error( "Error loading horizon ", horctio2_.ioobj->name() );
	return false;
    }

    Stats::Type typ = Stats::parseEnumType( ampoptionfld_->box()->text() );
    StratAmpCalc exec( tophor, usesingle_ ? 0 : bothor, typ, hs );
    exec.setOffsets( tophorshiftfld_->getfValue() / SI().zFactor(),
	    	     bothorshiftfld_->getfValue() / SI().zFactor() );
    
    IOPar iopar( "Stratal Amplitude Calculation parameters" );
    fillInEngineIOPar( iopar, hs );

    const int attribidx = exec.init( attribnm, addtotop, iopar );
    if ( attribidx < 0 )
	mErrRet( "Cannot add attribute to Horizon" );

    uiTaskRunner taskrunner( this );
    if ( !taskrunner.execute(exec) )
	mErrRet( "Cannot compute attribute" )

    const bool res = saveData( addtotop ? tophor : bothor ,
	    		       attribidx, overwrite );
    tophor->unRef();
    if ( bothor ) bothor->unRef();
    return res;
}


void uiStratAmpCalc::fillInEngineIOPar( IOPar& iop, const HorSampling& hs) const
{
    const Attrib::DescID targetid = inpfld_->attribID();
    Attrib::DescSet* clonedset = Attrib::DSHolder().getDescSet(
			    inpfld_->is2D(),false)->optimizeClone( targetid );
    IOPar attrpar( "Attribute Descriptions" );
    if ( !clonedset ) return;
    clonedset->fillPar( attrpar );
    for ( int idx=0; idx<attrpar.size(); idx++ )
    {
	const char* nm = attrpar.getKey( idx );
	iop.add( IOPar::compKey(Attrib::SeisTrcStorOutput::attribkey(),nm),
		   attrpar.getValue(idx) );
    }
    Attrib::Desc* desc = clonedset->getDesc( targetid );
    BufferString storedid = desc ? desc->getStoredID() : "";
    if ( !storedid.isEmpty() )
	iop.set( "Input Line Set", storedid.buf() );

    const BufferString keybase = IOPar::compKey( Attrib::Output::outputstr(),0);
    const BufferString attribkey =
	    IOPar::compKey( keybase, Attrib::SeisTrcStorOutput::attribkey() );
    iop.set( IOPar::compKey(attribkey,Attrib::DescSet::highestIDStr()), 1 );
    iop.set( IOPar::compKey(attribkey,0), targetid.asInt() );
    iop.set( IOPar::compKey(sKey::Output, sKey::Type), sKey::Cube );
    IOPar subselpar;
    hs.fillPar( subselpar );
    subselpar.set( sKey::ZRange, SI().zRange(false) );
    iop.mergeComp( subselpar, IOPar::compKey(sKey::Output,sKey::Subsel) );
}


bool uiStratAmpCalc::saveData( const EM::Horizon3D* hor, int attribidx,
			       bool overwrite )
{
    PtrMan<Executor> datasaver = 
			hor->auxdata.auxDataSaver( attribidx, overwrite );
    if ( !datasaver ) mErrRet( "Cannot save attribute" );

    uiTaskRunner taskrunner( this );
    return taskrunner.execute( *datasaver );
}


EM::Horizon3D* uiStratAmpCalc::loadHor( const IOObj* ioobj,
					const HorSampling& hs )
{
    if ( !ioobj )
	return 0;

    EM::EMManager& em = EM::EMM();
    EM::SurfaceIOData sd;
    EM::SurfaceIODataSelection sdsel( sd );
    sdsel.rg = hs;
    TypeSet<MultiID> mids; mids += ioobj->key();
    PtrMan<Executor> exec = em.objectLoader( mids, &sdsel );
    bool res = true;
    if ( exec )
    {
	uiTaskRunner taskrunner( this );
	res = taskrunner.execute( *exec );
    }

    if ( !res ) return 0;

    EM::EMObject* emobj = em.getObject( em.getObjectID(ioobj->key()) );
    if ( !emobj ) return 0;
    emobj->ref();
    mDynamicCastGet(EM::Horizon3D*,horizon,emobj)
    return horizon;
}
