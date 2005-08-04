/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2005
 RCS:           $Id: uisteeringsel.cc,v 1.4 2005-08-04 14:27:41 cvshelene Exp $
________________________________________________________________________

-*/


#include "uisteeringsel.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribsel.h"
#include "attribstorprovider.h"
#include "ctxtioobj.h"
#include "ioobj.h"
#include "ioman.h"
#include "iopar.h"
#include "survinfo.h"
#include "seistrctr.h"
#include "uigeninput.h"

using namespace Attrib;

IOPar& uiSteeringSel::inpselhist = *new IOPar( "Steering selection history" );

uiSteeringSel::uiSteeringSel( uiParent* p, const Attrib::DescSet* ads )
    : uiGroup(p,"Steering selection")
    , ctio(*mMkCtxtIOObj(SeisTrc))
    , typfld(0)
    , inpfld(0)
    , dirfld(0)
    , dipfld(0)
    , is2d(false)
{
    static const char* steertyps[] = { "None", "Central", "Full",
				       "Constant direction", 0 };
//  if ( AF().forms(true).size() < 1 )
//  {
//	nosteerlbl = new uiLabel( this, "<Steering unavailable>" );
//	setHAlignObj( nosteerlbl );
//	return;
//  }

    typfld = new uiGenInput( this, "Steering", StringListInpSpec(steertyps) );
    typfld->valuechanged.notify( mCB(this,uiSteeringSel,typeSel));

    ctio.ctxt.forread = true;
    inpfld = new uiSteerCubeSel( this, ctio, ads );
    inpfld->getHistory( inpselhist );
    inpfld->attach( alignedBelow, typfld );

    dirfld = new uiGenInput( this, "Azimuth", FloatInpSpec() );
    dirfld->attach( alignedBelow, typfld );
    BufferString dipstr( "Apparent dip " );
    dipstr += SI().zIsTime() ? "(us/m)" : "(degrees)";
    dipfld = new uiGenInput( this, dipstr, FloatInpSpec() );
    dipfld->attach( alignedBelow, dirfld );

    setHAlignObj( inpfld );
    mainObject()->finaliseStart.notify( mCB(this,uiSteeringSel,doFinalise) );
}


uiSteeringSel::~uiSteeringSel()
{
    delete ctio.ioobj; delete &ctio;
}


void uiSteeringSel::doFinalise(CallBacker*)
{
    typeSel(0);
}


void uiSteeringSel::typeSel( CallBacker* )
{
    if ( !typfld ) return; 

    int typ = typfld->getIntValue();
    inpfld->display( typ > 0 && typ < 3 );
    dirfld->display( typ == 3 && !is2d );
    dipfld->display( typ == 3 );
}


bool uiSteeringSel::willSteer() const
{
    return typfld && typfld->getIntValue() > 0;
}


void uiSteeringSel::useDesc( const Attrib::Desc& ad )
{
    if ( !typfld ) return;

    inpfld->setDescSet( ad.descSet() );
    if ( !ad.isSteering() )
    { typfld->setValue( ((int)0) ); typeSel( 0 ); return; }

    /*
    const Attrib::Desc::Steering* st = ad.steering();
    typfld->setValue( (int)st->type );

    if ( (int)st->type != 3 )
    {
	int steernr = ad.attribSet().descNr( st->inldipid );
	if ( steernr >= 0 )
	{
	    inpfld->setDesc( &ad.attribSet().getAttribDesc(steernr) );
	    inpfld->updateHistory( inpselhist );
	}
    }
    else
    {
	dirfld->setValue( st->azimuth );
	dipfld->setValue( st->appdip );
    }
    */

    typeSel( 0 );
}


void uiSteeringSel::fillDesc( Attrib::Desc& ad, ChangeTracker& chtr )
{
    if ( !typfld ) return;

    inpfld->processInput();

/*
    int sttyp = typfld->getIntValue();
    Attrib::Desc::Steering* st= const_cast<Attrib::Desc::Steering*>(ad.steering());
    if ( !sttyp && (!st || !st->type) ) return;

    chtr.update( st->type, (Attrib::Desc::Steering::Type)sttyp );
    if ( sttyp != 3 )
    {
	const DescID inldipid = inpfld->inlDipID();
	const DescID crldipid = inpfld->crlDipID();
	if ( inldipid == -1 || crldipid == -1 ) return;
	if ( crldipid == -2 )
	{
	    uiMSG().error( "Selected Steering input is not valid" );
	    return;
	}

	chtr.update( st->inldipid, inldipid );
	chtr.update( st->crldipid, crldipid );
    }
    else
    {
	float azimuth = dirfld->getValue();
	float appdip = dipfld->getValue();
	chtr.update( st->azimuth, azimuth );
	chtr.update( st->appdip, appdip );
    }
    */
}


void uiSteeringSel::setType( int nr, bool fixed )
{
    if ( !typfld ) return;
    typfld->setValue( nr );
    typfld->setSensitive( !fixed );
    typeSel(0);
}


void uiSteeringSel::set2D( bool yn )
{
    if ( !inpfld ) return;

    is2d = yn;
    inpfld->set2DPol( is2d ? Only2D : No2D );
    typeSel(0);
}




static const char* steer_seltxts[] = { "Steering Data", 0 };


uiSteerCubeSel::uiSteerCubeSel( uiParent* p, CtxtIOObj& c, 
				const DescSet* ads, const char* txt ) 
	: uiSeisSel(p,getCtio(c),SeisSelSetup().selattr(false),false,
			steer_seltxts)
	, attrdata( ads )
{
}


CtxtIOObj& uiSteerCubeSel::getCtio( CtxtIOObj& c )
{
    c.ctxt.parconstraints.set( sKey::Type, sKey::Steering );
    c.ctxt.includeconstraints = true;
    c.ctxt.allowcnstrsabsent = false;
    c.ctxt.forread = true;
    return c;
}


void uiSteerCubeSel::updateAttrSet2D()
{
    set2DPol( !attrdata.attrset || !attrdata.attrset->nrDescs() ? Both2DAnd3D
	    : (attrdata.attrset->is2D() ? Only2D : No2D) );
}


DescID uiSteerCubeSel::getDipID( int dipnr ) const
{
    if ( !attrdata.attrset )
    {
	pErrMsg( "No attribdescset set");
	return DescID::undef();
    }

    if ( !ctio.ioobj ) 
	return DescID::undef();

    Desc* desc = PF().createDescCopy( StorageProvider::attribName() );
    
    LineKey linekey( ctio.ioobj->key() );
    if ( is2D() )
	linekey.setAttrName( sKey::Steering );

    BufferString bfstr = StorageProvider::attribName(); bfstr += " "; 
    bfstr += StorageProvider::keyStr(); bfstr += "=";
    bfstr += linekey.lineName();
    desc->parseDefStr(bfstr);
    desc->setHidden( true );
    BufferString userref;
    
    if ( dipnr )
    {
	desc->updateParams();
	desc->selectOutput(1);
	userref = ctio.ioobj->name();	userref += "_crline_dip";
    }
    else
    {
	desc->selectOutput(0);
	userref = ctio.ioobj->name();	userref += "_inline_dip";
    }

    desc->setUserRef( userref );
    ValParam* keypar = (ValParam*)desc->getParam( StorageProvider::keyStr() );
    keypar->setValue( linekey );

    DescSet* ads = const_cast<DescSet*>(attrdata.attrset);
    return ads->addDesc( desc );
}


void uiSteerCubeSel::setDescSet( const DescSet* ads )
{
    attrdata.attrset = ads;
    updateAttrSet2D();
}


void uiSteerCubeSel::setDesc( const Desc* desc )
{
    if ( !desc || desc->selectedOutput() ) return;

    if ( !desc->isStored() ) return;

    const ValParam* keypar = 
		(ValParam*)desc->getParam( StorageProvider::keyStr() );
    const MultiID mid( keypar->getStringValue() );
    PtrMan<IOObj> ioobj = IOM().get( mid );
    ctio.setObj( ioobj ? ioobj->clone() : 0 );
    updateInput();
}


void uiSteerCubeSel::fillSelSpec( SelSpec& as, bool inldip )
{
    as.set( 0, inldip ? inlDipID() : crlDipID(), false, "" );
    as.setRefFromID( *attrdata.attrset );
}
