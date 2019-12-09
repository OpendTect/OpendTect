/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2009
________________________________________________________________________

-*/

#include "uiodapplmgraux.h"
#include "uiodapplmgr.h"

#include "attribdescset.h"
#include "coltabmapper.h"
#include "coltabseqmgr.h"
#include "filepath.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "scaler.h"
#include "seisioobjinfo.h"
#include "survinfo.h"
#include "zdomain.h"

#include "uiattribpartserv.h"
#include "uiemattribpartserv.h"
#include "uinlapartserv.h"
#include "uiviscoltabed.h"
#include "uicoltabsel.h"
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
    DBKey nlaid; const NLAModel* nlamdl = 0;
    if ( am_.nlaserv_ )
    {
	am_.nlaserv_->set2DEvent( is2d );
	nlaid = am_.nlaserv_->modelId();
	nlamdl = am_.nlaserv_->getModel();
    }
    am_.emattrserv_->setNLA( nlamdl, nlaid );

    uiEMAttribPartServer::HorOutType type =
	  tp==0 ? uiEMAttribPartServer::OnHor :
	( tp==1 ? uiEMAttribPartServer::AroundHor :
		  uiEMAttribPartServer::BetweenHors );
    am_.emattrserv_->createHorizonOutput( type, is2d );
}


void uiODApplMgrAttrVisHandler::createVol( bool is2d, bool multiattrib )
{
    DBKey nlaid;
    if ( am_.nlaserv_ )
    {
	am_.nlaserv_->set2DEvent( is2d );
	nlaid = am_.nlaserv_->modelId();
    }
    am_.attrserv_->outputVol( nlaid, is2d, multiattrib );
}


void uiODApplMgrAttrVisHandler::doXPlot()
{
    const Attrib::DescSet* ads = am_.attrserv_->getUserPrefDescSet(par_);
    if ( ads )
	am_.wellattrserv_->doXPlot( ads->is2D() );
}


void uiODApplMgrAttrVisHandler::crossPlot()
{
    const Attrib::DescSet* ads = am_.attrserv_->getUserPrefDescSet(par_);
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

    if ( as->id()==Attrib::SelSpec::cAttribNotSelID() &&
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
	am_.visserv_->setSelSpec( id, attrib, myas );
    return selok;
}


void uiODApplMgrAttrVisHandler::setRandomPosData( int visid, int attrib,
						 const DataPointSet& data )
{
    DataPack::ID cacheid = am_.visserv_->getDataPackID( visid, attrib );
    if ( cacheid.isInvalid() )
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


void uiODApplMgrAttrVisHandler::hideColorTable()
{
    am_.appl_.colTabEd().display( false );
}


void uiODApplMgrAttrVisHandler::updateColorTable( int visid, int attrib  )
{
    if ( visid<0 || attrib<0 || attrib>=am_.visserv_->getNrAttribs(visid) )
    {
	hideColorTable();
	return;
    }

    mDynamicCastGet( visSurvey::SurveyObject*, so,
		     am_.visserv_->getObject( visid ) );
    if ( so )
    {
	am_.visserv_->setColTabMapper( visid, attrib,
			am_.visserv_->getColTabMapper(visid,attrib) );
	am_.visserv_->setColTabSequence( visid, attrib,
			am_.visserv_->getColTabSequence( visid, attrib ) );
    }
    else
    {
	ColTab::Mapper& mapper = const_cast<ColTab::Mapper&>(
			am_.visserv_->getColTabMapper(visid,attrib) );
	am_.appl_.colTabEd().setColTab(
	    am_.visserv_->getColTabSequence( visid, attrib ), mapper );
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

    if ( attrib == -1 )
	attrib = 0;

    am_.visserv_->setColTabSequence( visid, attrib,
	    am_.appl_.colTabEd().getColTabSequence() );
}


void uiODApplMgrAttrVisHandler::useDefColTab( int visid, int attrib )
{
    if ( am_.appl_.isRestoringSession() ) return;

    const Attrib::SelSpec* as = am_.visserv_->getSelSpec( visid, attrib );
    if ( !as || !as->isUsable() )
	return;

    ConstRefMan<ColTab::Sequence> ctseq =
		&am_.visserv_->getColTabSequence( visid, attrib );
    ColTab::Mapper& ctmap = const_cast<ColTab::Mapper&>(
		am_.visserv_->getColTabMapper( visid, attrib ) );

    PtrMan<IOObj> ioobj = am_.attrserv_->getIOObj( *as );
    if ( ioobj )
    {
	SeisIOObjInfo seisobj( ioobj );
	IOPar iop;
	if ( seisobj.getDisplayPars(iop) )
	{
	    const char* ctname = iop.find( sKey::Name() );
	    ctseq = ColTab::SeqMGR().getAny( ctname );
	    ctmap.setup().usePar( iop );
	}
    }

    am_.visserv_->setColTabMapper( visid, attrib, ctmap );
    am_.visserv_->setColTabSequence( visid, attrib, *ctseq );
    am_.appl_.colTabEd().colTabSel().setMapper( ctmap );
    am_.appl_.colTabEd().colTabSel().setSeqName( ctseq->name() );
    updateColorTable( visid, attrib );
}


void uiODApplMgrAttrVisHandler::saveDefColTab( int visid, int attrib )
{
    const Attrib::SelSpec* as = am_.visserv_->getSelSpec(visid,attrib);
    PtrMan<IOObj> ioobj = am_.attrserv_->getIOObj( *as );
    if ( !ioobj ) return;

    const ColTab::Sequence& ctseq =
		am_.visserv_->getColTabSequence( visid, attrib );
    const ColTab::Mapper& mapper =
		am_.visserv_->getColTabMapper( visid, attrib );

    File::Path fp( ioobj->mainFileName() );
    fp.setExtension( sParFileExtension() );
    IOPar iop; iop.read( fp.fullPath(), sKey::Pars() );
    iop.set( sKey::Name(), ctseq.name() );

    mapper.setup().fillPar( iop );

    iop.write( fp.fullPath(), sKey::Pars() );
}
