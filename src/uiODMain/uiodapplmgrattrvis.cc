/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiodapplmgraux.h"
#include "uiodapplmgr.h"

#include "attribdescset.h"
#include "coltabmapper.h"
#include "coltabsequence.h"
#include "filepath.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "seis2dline.h"
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


void uiODApplMgrAttrVisHandler::createVol( bool is2d )
{
    MultiID nlaid;
    if ( am_.nlaserv_ )
    {
	am_.nlaserv_->set2DEvent( is2d );
	nlaid = am_.nlaserv_->modelId();
    }
    am_.attrserv_->outputVol( nlaid, is2d );
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

    const ZDomain::Info* zdinf =
	am_.visserv_->zDomainInfo( am_.visserv_->getSceneID(id) );
    const bool issi = !zdinf || zdinf->def_.isSI();
    Attrib::SelSpec myas( *as );
    const bool selok = am_.attrserv_->selectAttrib( myas, issi ? 0 : zdinf,
	    					    myas.is2D() );
    if ( selok )
	am_.visserv_->setSelSpec( id, attrib, myas );
    return selok;
}


void uiODApplMgrAttrVisHandler::setHistogram( int visid, int attrib )
{
    am_.appl_.colTabEd().setHistogram(
	    	am_.visserv_->getHistogram(visid,attrib) );
}


void uiODApplMgrAttrVisHandler::createAndSetMapDataPack( int visid, int attrib,
					   const DataPointSet& data, int colnr )
{
    DataPack::ID cacheid = am_.visserv_->getDataPackID( visid, attrib );
    if ( cacheid == -1 )
	am_.useDefColTab( visid, attrib );

    am_.visserv_->setRandomPosData( visid, attrib, &data );
    const int dpid = am_.createMapDataPack( data, colnr );
    am_.visserv_->setDataPackID( visid, attrib, dpid );
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


bool uiODApplMgrAttrVisHandler::set2DDataFileName(
		    int visid, const Attrib::SelSpec* as,
		    const IOObj& ioobj, FilePath& fp )
{
    mDynamicCastGet( visSurvey::Seis2DDisplay*, s2d, 
		     am_.visserv_->getObject( visid ) ); 
    if ( !s2d ) 
	return false;

    const char* linenm = s2d->getLineName();
    LineKey lk( linenm, as->userRef() ); 
    Seis2DLineSet seis2dlnset( ioobj ); 
    const int lineidx = seis2dlnset.indexOf( lk ); 
    if ( lineidx < 0 ) 
	return false; 
    const IOPar par2d = seis2dlnset.getInfo( lineidx ); 
    BufferString fnm; 
    par2d.get( "File name", fnm );
    fp.setFileName( fnm );
    return true;
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

    ColTab::MapperSetup mapper, mapper1;
    const ColTab::MapperSetup* ctmap = !ioobj ?
		0 : am_.visserv_->getColTabMapperSetup( visid, attrib );
    if ( ctmap ) mapper = *ctmap;

    IOPar iop;
    if ( ioobj )
    {
    	FilePath fp( ioobj->fullUserExpr(true) );
    	if ( as->is2D() && !set2DDataFileName(visid,as,*ioobj,fp) )
    	    return;

    	fp.setExtension( "par" );
    	if ( iop.read( fp.fullPath(), sKey::Pars()) && !iop.isEmpty() )
    	{
    	    const char* ctname = iop.find( sKey::Name() );
    	    seq = ColTab::Sequence( ctname );
    	    mapper.usePar( iop );
    	}    
    }
    const bool isempt = iop.isEmpty();    

    am_.visserv_->setColTabMapperSetup( visid, attrib,!isempt? mapper:mapper1 );
    am_.visserv_->setColTabSequence( visid, attrib, seq );
    am_.appl_.colTabEd().colTab()->setMapperSetup( !isempt ? &mapper:&mapper1 );
    am_.appl_.colTabEd().colTab()->setSequence( &seq, true );
    updateColorTable( visid, attrib );
}

#define mSetPar \
IOPar iop; \
if ( ctseq ) \
    iop.set( sKey::Name(), ctseq->name() ); \
if ( mapper ) \
    mapper->fillPar( iop ); \
iop.write( fp.fullPath(), sKey::Pars() );

void uiODApplMgrAttrVisHandler::saveDefColTab( int visid, int attrib )
{
    const Attrib::SelSpec* as = am_.visserv_->getSelSpec(visid,attrib);
    PtrMan<IOObj> ioobj = am_.attrserv_->getIOObj( *as );
    if ( !ioobj ) return;

    const ColTab::Sequence* ctseq =
		am_.visserv_->getColTabSequence( visid, attrib );
    const ColTab::MapperSetup* mapper =
		am_.visserv_->getColTabMapperSetup( visid, attrib );
    if ( !as->is2D() )
    {
	FilePath fp( ioobj->fullUserExpr(true) );
	fp.setExtension( "par" );
	mSetPar;
    }
    else
    {
	Seis2DLineSet seis2dlnset( *ioobj ); 
	BufferStringSet linenames;
        seis2dlnset.getLineNamesWithAttrib( linenames, as->userRef() );	
	for ( int lidx=0; lidx<linenames.size(); lidx++ )
	{
	    LineKey lk( linenames.get(lidx), as->userRef() ); 
	    const int lineidx = seis2dlnset.indexOf( lk ); 
	    if ( lineidx < 0 ) 
		continue; 
	    const IOPar par2d = seis2dlnset.getInfo( lineidx ); 
	    BufferString fnm; 
	    par2d.get( "File name", fnm );
	    FilePath fp( ioobj->fullUserExpr(true) );
	    fp.setFileName( fnm );
	    fp.setExtension( "par" );
	    mSetPar;
	}
    }
}
