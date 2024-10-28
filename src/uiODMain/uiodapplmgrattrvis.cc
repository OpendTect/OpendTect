/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodapplmgrattrvis.h"

#include "attribdescset.h"
#include "coltabmapper.h"
#include "coltabsequence.h"
#include "filepath.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "scaler.h"
#include "seisioobjinfo.h"
#include "survinfo.h"
#include "zdomain.h"

#include "uiattribpartserv.h"
#include "uicolortable.h"
#include "uiemattribpartserv.h"
#include "uimsg.h"
#include "uinlapartserv.h"
#include "uiodapplmgr.h"
#include "uiviscoltabed.h"
#include "uivispartserv.h"
#include "uiwellattribpartserv.h"
#include "visseis2ddisplay.h"
#include "vissurvobj.h"


uiODApplMgrAttrVisHandler::uiODApplMgrAttrVisHandler(
					uiODApplMgr& a, uiParent* p )
    : am_(a)
    , par_(p)
{}


uiODApplMgrAttrVisHandler::~uiODApplMgrAttrVisHandler()
{}


void uiODApplMgrAttrVisHandler::survChg( bool before )
{
}


bool uiODApplMgrAttrVisHandler::editNLA( bool is2d )
{
    uiNLAPartServer* nlaserv = am_.nlaServer();
    if ( !nlaserv )
	return false;

    nlaserv->set2DEvent( is2d );
    const bool res = nlaserv->go();
    if ( !res )
	am_.attrServer()->setNLAName( nlaserv->modelName() );

    return res;
}


bool uiODApplMgrAttrVisHandler::uvqNLA( bool is2d )
{
    uiNLAPartServer* nlaserv = am_.nlaServer();
    if ( !nlaserv )
	return false;

    nlaserv->set2DEvent( is2d );
    const bool res = nlaserv->doUVQ();
    return res;
}


void uiODApplMgrAttrVisHandler::createHorOutput( int tp, bool is2d )
{
    uiEMAttribPartServer* emattrserv = am_.EMAttribServer();
    uiNLAPartServer* nlaserv = am_.nlaServer();
    emattrserv->setDescSet( am_.attrServer()->curDescSet(is2d) );
    MultiID nlaid;
    const NLAModel* nlamdl = nullptr;
    if ( nlaserv )
    {
	nlaserv->set2DEvent( is2d );
	nlaid = nlaserv->modelId();
	nlamdl = &nlaserv->getModel();
    }

    emattrserv->setNLA( nlamdl, nlaid );
    const auto type = sCast(uiEMAttribPartServer::HorOutType,tp);
    emattrserv->createHorizonOutput( type );
}


void uiODApplMgrAttrVisHandler::saveNLA(CallBacker*)
{
    uiNLAPartServer* nlaserv = am_.nlaServer();
    if ( nlaserv )
	nlaserv->doStore();
}


void uiODApplMgrAttrVisHandler::createVol( bool is2d, bool multiattrib )
{
    MultiID nlaid;
    uiNLAPartServer* nlaserv = am_.nlaServer();
    if ( nlaserv )
    {
	nlaserv->set2DEvent( is2d );
	nlaid = nlaserv->modelId();
    }

    am_.attrServer()->outputVol( nlaid, is2d, multiattrib );
    mAttachCB( am_.attrServer()->needSaveNLA,
	       uiODApplMgrAttrVisHandler::saveNLA );
}


void uiODApplMgrAttrVisHandler::doXPlot()
{
    const Attrib::DescSet* ads = am_.attrServer()->getUserPrefDescSet();
    if ( !ads )
	return;

    am_.wellAttribServer()->setAttribSet( *ads );
    am_.wellAttribServer()->doXPlot();
}


void uiODApplMgrAttrVisHandler::crossPlot()
{
    const Attrib::DescSet* ads = am_.attrServer()->getUserPrefDescSet();
    if ( !ads )
	return;

    am_.attrServer()->set2DEvent( ads->is2D() );
    am_.attrServer()->showXPlot( nullptr );
}


void uiODApplMgrAttrVisHandler::setZStretch()
{
    am_.visServer()->setZStretch();
}


bool uiODApplMgrAttrVisHandler::selectAttrib( const VisID& id, int attrib )
{
    if ( am_.isRestoringSession() )
	return false;

    if ( !id.isValid() )
	return false;

    uiVisPartServer* visserv = am_.visServer();
    const Attrib::SelSpec* as = visserv->getSelSpec( id, attrib );
    if ( !as )
	return false;

    if ( as->id()==Attrib::SelSpec::cAttribNotSel() &&
	 !visserv->isAttribEnabled( id, attrib ) )
	return false;

    uiString attribposname;
    visserv->getAttribPosName( id, attrib, attribposname );

    const Pos::GeomID geomid = visserv->getGeomID( id );
    const ZDomain::Info* zdinf = visserv->zDomainInfo( visserv->getSceneID(id));
    const bool issi = !zdinf || zdinf->def_.isSI();
    Attrib::SelSpec myas( *as );
    const bool selok = am_.attrServer()->selectAttrib( myas, issi ? 0 : zdinf,
						    geomid, attribposname);
    const TypeSet<Attrib::SelSpec>& ass = am_.attrServer()->getTargetSelSpecs();
    if ( selok && !ass.isEmpty() )
	visserv->setSelSpecs( id, attrib, ass );

    return selok;
}


void uiODApplMgrAttrVisHandler::setHistogram( const VisID& visid, int attrib )
{
    am_.colTabEd().setHistogram( am_.visServer()->getHistogram(visid,attrib) );
}


bool uiODApplMgrAttrVisHandler::setRandomPosData( const VisID& visid,
					int attrib, const DataPointSet& data )
{
    ConstRefMan<DataPack> cachedp =
				am_.visServer()->getDataPack( visid, attrib );
    if ( !cachedp )
	am_.useDefColTab( visid, attrib );

    return am_.visServer()->setRandomPosData( visid, attrib, &data );
}


void uiODApplMgrAttrVisHandler::pageUpDownPressed( bool pageup )
{
    uiVisPartServer* visserv = am_.visServer();
    const VisID visid = visserv->getEventObjId();
    const int attrib = visserv->getSelAttribNr();
    if ( attrib<0 || attrib>=visserv->getNrAttribs(visid) )
	return;

    int texture = visserv->selectedTexture( visid, attrib );
    const int nrtextures = visserv->nrTextures( visid, attrib );
    if ( pageup )
    {
	texture--;
	if ( texture < 0 )
	    texture = nrtextures - 1;
    }
    else
    {
	texture++;
	if ( texture >= nrtextures )
	    texture = 0;
    }

    visserv->selectTexture( visid, attrib, texture );
    updateColorTable( visid, attrib );
}


void uiODApplMgrAttrVisHandler::updateColorTable( const VisID& visid,
						  int attrib  )
{
    uiVisPartServer* visserv = am_.visServer();
    if ( attrib<0 || attrib>=visserv->getNrAttribs(visid) )
    {
	am_.colTabEd().setColTab( 0, false, 0, false );
	return;
    }

    mDynamicCastGet( visSurvey::SurveyObject*, so,
		     visserv->getObject( visid ) );
    if ( so )
	am_.colTabEd().setColTab( so, attrib, mUdf(int) );
    else
    {
	am_.colTabEd().setColTab(
	    visserv->getColTabSequence( visid, attrib ), true,
	    visserv->getColTabMapperSetup(visid,attrib),
	    visserv->canHandleColTabSeqTrans(visid,attrib) );
    }

    setHistogram( visid, attrib );
}


void uiODApplMgrAttrVisHandler::colMapperChg()
{
    uiVisPartServer* visserv = am_.visServer();
    ConstRefMan<visBase::DataObject> dataobj = am_.colTabEd().getDataObj();
    const VisID visid = dataobj ? dataobj->id()
				: visserv->getSelObjectId();
    int attrib = dataobj ? am_.colTabEd().getChannel()
			 : visserv->getSelAttribNr();
    if ( attrib == -1 )
	attrib = 0;

    visserv->setColTabMapperSetup( visid, attrib,
				   am_.colTabEd().getColTabMapperSetup());
    setHistogram( visid, attrib );

    //Autoscale may have changed ranges, so update.
    mDynamicCastGet( visSurvey::SurveyObject*, so,
		     visserv->getObject(visid) );
    if ( so )
	am_.colTabEd().setColTab( so, attrib, mUdf(int) );
    else
    {
	am_.colTabEd().setColTab(
	    visserv->getColTabSequence( visid, attrib ), true,
	    visserv->getColTabMapperSetup(visid,attrib),
	    visserv->canHandleColTabSeqTrans(visid,attrib) );
    }
}


void uiODApplMgrAttrVisHandler::colSeqChg()
{
    uiVisPartServer* visserv = am_.visServer();
    ConstRefMan<visBase::DataObject> dataobj = am_.colTabEd().getDataObj();
    const VisID visid = dataobj ? dataobj->id() : visserv->getSelObjectId();
    int attrib = dataobj ? am_.colTabEd().getChannel()
			 : visserv->getSelAttribNr();

    if ( attrib == -1 )
	attrib = 0;

    setHistogram( visid, attrib );
    visserv->setColTabSequence( visid, attrib,
				am_.colTabEd().getColTabSequence() );
}


NotifierAccess* uiODApplMgrAttrVisHandler::colorTableSeqChange()
{
    return &am_.colTabEd().seqChange();
}


void uiODApplMgrAttrVisHandler::useDefColTab( const VisID& visid, int attrib )
{
    if ( am_.isRestoringSession() )
	return;

    uiVisPartServer* visserv = am_.visServer();
    const Attrib::SelSpec* as = visserv->getSelSpec( visid, attrib );
    if ( !as || as->id().asInt() < 0 )
	return;

    PtrMan<IOObj> ioobj = am_.attrServer()->getIOObj( *as );

    ColTab::Sequence seq;
    const ColTab::Sequence* ctseq = visserv->getColTabSequence( visid, attrib );
    if ( ctseq )
	seq = *ctseq;

    ColTab::MapperSetup mapper;
    const ColTab::MapperSetup* ctmap =
			    visserv->getColTabMapperSetup( visid, attrib );
    if ( ctmap )
	mapper = *ctmap;

    if ( ioobj )
    {
	const SeisIOObjInfo seisobj( ioobj.ptr() );
	IOPar iop;
	if ( seisobj.isOK() && seisobj.getDisplayPars(iop) )
	{
	    const BufferString ctname = iop.find( sKey::Name() );
	    seq = ColTab::Sequence( ctname );
	    mapper.usePar( iop );
	}
	else if ( !seisobj.is2D() && seisobj.nrComponents() == 1 )
	{
	    uiMSG().message( tr("No saved color settings found for the selected"
			" cube. Default settings will be loaded. For changing "
			"these settings, click on \"Save Color Settings\" "
			"option in tree."), uiString::emptyString(),
			uiString::emptyString(), true );
	    saveDefColTab( visid, attrib );
	}
    }

    visserv->setColTabMapperSetup( visid, attrib, mapper );
    visserv->setColTabSequence( visid, attrib, seq );
    am_.colTabEd().colTab().setMapperSetup( &mapper );
    am_.colTabEd().colTab().setSequence( &seq, true );
    updateColorTable( visid, attrib );
}


void uiODApplMgrAttrVisHandler::saveDefColTab( const VisID& visid, int attrib )
{
    uiVisPartServer* visserv = am_.visServer();
    const Attrib::SelSpec* as = visserv->getSelSpec(visid,attrib);
    PtrMan<IOObj> ioobj = am_.attrServer()->getIOObj( *as );
    if ( !ioobj )
	return;

    const ColTab::Sequence* ctseq =
			    visserv->getColTabSequence( visid, attrib );
    const ColTab::MapperSetup* mapper =
			    visserv->getColTabMapperSetup( visid, attrib );

    if ( mIsUdf(mapper->range_.start_) || mIsUdf(mapper->range_.stop_) )
	return;

    FilePath fp( ioobj->fullUserExpr(true) );
    fp.setExtension( "par" );
    IOPar iop;
    if ( ctseq )
	iop.set( sKey::Name(), ctseq->name() );

    if ( mapper )
	mapper->fillPar( iop );

    iop.set( sKey::Type(), "Fixed" );

    iop.write( fp.fullPath(), sKey::Pars() );
}
