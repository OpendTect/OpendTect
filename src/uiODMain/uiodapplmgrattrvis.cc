/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2009
________________________________________________________________________

-*/

#include "uiodapplmgraux.h"
#include "uiodapplmgr.h"
#include "uimsg.h"

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
#include "uinlapartserv.h"
#include "uiviscoltabed.h"
#include "uivispartserv.h"
#include "uiwellattribpartserv.h"
#include "visseis2ddisplay.h"
#include "vissurvobj.h"

void uiODApplMgrAttrVisHandler::survChg( bool before )
{
}


bool uiODApplMgrAttrVisHandler::editNLA( bool is2d )
{
    if ( !am_.nlaserv_ ) return false;

    am_.nlaserv_->set2DEvent( is2d );
    const bool res = am_.nlaserv_->go();
    if ( !res ) am_.attrserv_->setNLAName( am_.nlaserv_->modelName() );
    return res;
}


bool uiODApplMgrAttrVisHandler::uvqNLA( bool is2d )
{
    if ( !am_.nlaserv_ ) return false;

    am_.nlaserv_->set2DEvent( is2d );
    const bool res = am_.nlaserv_->doUVQ();
    return res;
}


void uiODApplMgrAttrVisHandler::createHorOutput( int tp, bool is2d )
{
    am_.emattrserv_->setDescSet( am_.attrserv_->curDescSet(is2d) );
    MultiID nlaid; const NLAModel* nlamdl = 0;
    if ( am_.nlaserv_ )
    {
	am_.nlaserv_->set2DEvent( is2d );
	nlaid = am_.nlaserv_->modelId();
	nlamdl = &am_.nlaserv_->getModel();
    }
    am_.emattrserv_->setNLA( nlamdl, nlaid );

    uiEMAttribPartServer::HorOutType type =
	  tp==0 ? uiEMAttribPartServer::OnHor :
	( tp==1 ? uiEMAttribPartServer::AroundHor :
		  uiEMAttribPartServer::BetweenHors );
    am_.emattrserv_->createHorizonOutput( type );
}


void uiODApplMgrAttrVisHandler::saveNLA(CallBacker*)
{
    if ( am_.nlaserv_ )
	am_.nlaserv_->doStore();
}



void uiODApplMgrAttrVisHandler::createVol( bool is2d, bool multiattrib )
{
    MultiID nlaid;
    if ( am_.nlaserv_ )
    {
	am_.nlaserv_->set2DEvent( is2d );
	nlaid = am_.nlaserv_->modelId();
    }
    am_.attrserv_->outputVol( nlaid, is2d, multiattrib );
    mAttachCB( am_.attrserv_->needSaveNLA,uiODApplMgrAttrVisHandler::saveNLA );
}


void uiODApplMgrAttrVisHandler::doXPlot()
{
    const Attrib::DescSet* ads = am_.attrserv_->getUserPrefDescSet();
    if ( !ads ) return;

    am_.wellattrserv_->setAttribSet( *ads );
    am_.wellattrserv_->doXPlot();
}


void uiODApplMgrAttrVisHandler::crossPlot()
{
    const Attrib::DescSet* ads = am_.attrserv_->getUserPrefDescSet();
    if ( !ads ) return;

    am_.attrserv_->set2DEvent( ads->is2D() );
    am_.attrserv_->showXPlot(0);
}


void uiODApplMgrAttrVisHandler::setZStretch()
{
    am_.visserv_->setZStretch();
}


bool uiODApplMgrAttrVisHandler::selectAttrib( int id, int attrib )
{
    if ( am_.appl_.isRestoringSession() ) return false;

    if ( id < 0 ) return false;
    const Attrib::SelSpec* as = am_.visserv_->getSelSpec( id, attrib );
    if ( !as ) return false;

    if ( as->id()==Attrib::SelSpec::cAttribNotSel() &&
	 !am_.visserv_->isAttribEnabled( id, attrib ) )
	return false;

    uiString attribposname;
    am_.visserv_->getAttribPosName( id, attrib, attribposname );

    const Pos::GeomID geomid = am_.visserv_->getGeomID( id );
    const ZDomain::Info* zdinf =
	am_.visserv_->zDomainInfo( am_.visserv_->getSceneID(id) );
    const bool issi = !zdinf || zdinf->def_.isSI();
    Attrib::SelSpec myas( *as );
    const bool selok = am_.attrserv_->selectAttrib( myas, issi ? 0 : zdinf,
						    geomid, attribposname);
    if ( selok )
	am_.visserv_->setSelSpecs(
		id, attrib, am_.attrserv_->getTargetSelSpecs() );
    return selok;
}


void uiODApplMgrAttrVisHandler::setHistogram( int visid, int attrib )
{
    am_.appl_.colTabEd().setHistogram(
	    	am_.visserv_->getHistogram(visid,attrib) );
}


void uiODApplMgrAttrVisHandler::setRandomPosData( int visid, int attrib,
						 const DataPointSet& data )
{
    DataPack::ID cacheid = am_.visserv_->getDataPackID( visid, attrib );
    if ( cacheid == -1 )
	am_.useDefColTab( visid, attrib );

    am_.visserv_->setRandomPosData( visid, attrib, &data );
}


void uiODApplMgrAttrVisHandler::pageUpDownPressed( bool pageup )
{
    const int visid = am_.visserv_->getEventObjId();
    const int attrib = am_.visserv_->getSelAttribNr();
    if ( attrib<0 || attrib>=am_.visserv_->getNrAttribs(visid) )
	return;

    int texture = am_.visserv_->selectedTexture( visid, attrib );
    if ( texture<am_.visserv_->nrTextures(visid,attrib)-1 && !pageup )
	texture++;
    else if ( texture && pageup )
	texture--;

    am_.visserv_->selectTexture( visid, attrib, texture );
    updateColorTable( visid, attrib );
}


void uiODApplMgrAttrVisHandler::updateColorTable( int visid, int attrib  )
{
    if ( attrib<0 || attrib>=am_.visserv_->getNrAttribs(visid) )
    {
	am_.appl_.colTabEd().setColTab( 0, false, 0, false );
	return;
    }

    mDynamicCastGet( visSurvey::SurveyObject*, so,
	am_.visserv_->getObject( visid ) );
    if ( so )
	am_.appl_.colTabEd().setColTab( so, attrib, mUdf(int) );
    else
    {
 	am_.appl_.colTabEd().setColTab(
	    am_.visserv_->getColTabSequence( visid, attrib ),
	    true, am_.visserv_->getColTabMapperSetup(visid,attrib),
	    am_.visserv_->canHandleColTabSeqTrans(visid,attrib) );
    }

    setHistogram( visid, attrib );
}


void uiODApplMgrAttrVisHandler::colMapperChg()
{
    mDynamicCastGet(const visBase::DataObject*,dataobj,
		    am_.appl_.colTabEd().getSurvObj())
    const int visid = dataobj ? dataobj->id() : am_.visserv_->getSelObjectId();
    int attrib = dataobj
	? am_.appl_.colTabEd().getChannel() : am_.visserv_->getSelAttribNr();
    if ( attrib == -1 ) attrib = 0;

    am_.visserv_->setColTabMapperSetup( visid, attrib,
	    am_.appl_.colTabEd().getColTabMapperSetup() );
    setHistogram( visid, attrib );

    //Autoscale may have changed ranges, so update.
    mDynamicCastGet( visSurvey::SurveyObject*, so,
	am_.visserv_->getObject( visid ) );
    if ( so )
	am_.appl_.colTabEd().setColTab( so, attrib, mUdf(int) );
    else
    {
 	am_.appl_.colTabEd().setColTab(
	    am_.visserv_->getColTabSequence( visid, attrib ),
	    true, am_.visserv_->getColTabMapperSetup(visid,attrib),
	    am_.visserv_->canHandleColTabSeqTrans(visid,attrib) );
    }
}


void uiODApplMgrAttrVisHandler::colSeqChg()
{
    mDynamicCastGet(const visBase::DataObject*,dataobj,
		    am_.appl_.colTabEd().getSurvObj())
    const int visid = dataobj ? dataobj->id() : am_.visserv_->getSelObjectId();
    int attrib = dataobj
	? am_.appl_.colTabEd().getChannel()
	: am_.visserv_->getSelAttribNr();

    if ( attrib == -1 ) attrib = 0;
    setHistogram( visid, attrib );

    am_.visserv_->setColTabSequence( visid, attrib,
	    am_.appl_.colTabEd().getColTabSequence() );
}


NotifierAccess* uiODApplMgrAttrVisHandler::colorTableSeqChange()
{
    return &am_.appl_.colTabEd().seqChange();
}


void uiODApplMgrAttrVisHandler::useDefColTab( int visid, int attrib )
{
    if ( am_.appl_.isRestoringSession() ) return;

    const Attrib::SelSpec* as = am_.visserv_->getSelSpec( visid, attrib );
    if ( !as || as->id().asInt() < 0 ) return;

    PtrMan<IOObj> ioobj = am_.attrserv_->getIOObj( *as );

    ColTab::Sequence seq( 0 );
    const ColTab::Sequence* ctseq =
		am_.visserv_->getColTabSequence( visid, attrib );
    if ( ctseq ) seq = *ctseq;

    ColTab::MapperSetup mapper;
    const ColTab::MapperSetup* ctmap =
		am_.visserv_->getColTabMapperSetup( visid, attrib );
    if ( ctmap ) mapper = *ctmap;

    if ( ioobj )
    {
	SeisIOObjInfo seisobj( ioobj );
	IOPar iop;
	if ( seisobj.getDisplayPars( iop ) )
    	{
    	    const char* ctname = iop.find( sKey::Name() );
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

    am_.visserv_->setColTabMapperSetup( visid, attrib, mapper );
    am_.visserv_->setColTabSequence( visid, attrib, seq );
    am_.appl_.colTabEd().colTab().setMapperSetup( &mapper );
    am_.appl_.colTabEd().colTab().setSequence( &seq, true );
    updateColorTable( visid, attrib );
}


void uiODApplMgrAttrVisHandler::saveDefColTab( int visid, int attrib )
{
    const Attrib::SelSpec* as = am_.visserv_->getSelSpec(visid,attrib);
    PtrMan<IOObj> ioobj = am_.attrserv_->getIOObj( *as );
    if ( !ioobj ) return;

    const ColTab::Sequence* ctseq =
		am_.visserv_->getColTabSequence( visid, attrib );
    const ColTab::MapperSetup* mapper =
		am_.visserv_->getColTabMapperSetup( visid, attrib );

    if ( mIsUdf(mapper->range_.start) || mIsUdf(mapper->range_.stop) )
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
