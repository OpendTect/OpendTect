/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "arrayndimpl.h"
#include "ascstream.h"
#include "embody.h"
#include "embodytr.h"
#include "emmanager.h"
#include "emmarchingcubessurface.h"
#include "empolygonbody.h"
#include "emrandomposbody.h"
#include "emsurface.h"
#include "filepath.h"
#include "iodir.h"
#include "ioman.h"
#include "iostrm.h"
#include "strmprov.h"
#include "uistrings.h"


namespace EM
{

ImplicitBody::ImplicitBody()
    : arr_( 0 )
    , threshold_( mUdf(float) )
    , tkzs_( false )
{}

#define mCopyArr( ib ) \
    if ( ib.arr_ ) \
    { \
	mDeclareAndTryAlloc( Array3DImpl<float>*, newarr, \
		Array3DImpl<float>(ib.arr_->info()) ); \
	if ( newarr )  \
	{ \
	    newarr->copyFrom( *ib.arr_ ); \
	    arr_ = newarr; \
	} \
    }


ImplicitBody::ImplicitBody( const ImplicitBody& ib )
   : threshold_( ib.threshold_ )
   , tkzs_( ib.tkzs_ )
   , arr_( 0 )
{
    mCopyArr(ib)
}


ImplicitBody::~ImplicitBody()
{ delete arr_; }


ImplicitBody ImplicitBody::operator =( const ImplicitBody& ib )
{
    threshold_ = ib.threshold_;
    tkzs_ = ib.tkzs_;

    delete arr_; arr_ = 0;
    mCopyArr( ib );

    return *this;
}


ImplicitBody* Body::createImplicitBody( TaskRunner*, bool ) const
{ return 0; }


const IOObjContext& Body::getBodyContext() const
{ return EMBodyTranslatorGroup::ioContext(); }


} // namespace EM


/*False: already converted; True: need conversion. */
mGlobal(EarthModel) bool OD_Get_Body_Conversion_Status()
{
    const MultiID mid ( IOObjContext::getStdDirData(IOObjContext::Surf)->id_ );
    const IODir iodir( mid );
    const ObjectSet<IOObj>& ioobjs = iodir.getObjs();
    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	IOObj& ioobj = const_cast<IOObj&>(*ioobjs[idx]);
	if ( ioobj.group()=="MarchingCubesSurface" )
	    return true;

	const StringView translt( ioobj.translator() );
	if ( translt=="MCBody" || translt=="PolygonBody" ||
	     translt=="RandomPosBody" )
	    return true;

	BufferString objtype;
	ioobj.pars().get( sKey::Type(), objtype );

	if ( objtype=="RandomPosBody" || objtype=="PolygonBody" ||
	     objtype=="MCBody" )
	    return true;
    }

    return false;
}


mGlobal(EarthModel) bool OD_Convert_Body_To_OD5( uiString& errmsg )
{
    const MultiID mid ( IOObjContext::getStdDirData(IOObjContext::Surf)->id_ );
    const IODir iodir( mid );
    const ObjectSet<IOObj>& ioobjs = iodir.getObjs();
    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	IOObj& ioobj = const_cast<IOObj&>(*ioobjs[idx]);
	const StringView translt( ioobj.translator() );
	IOPar& iopar = ioobj.pars();
	BufferString objtype;
	iopar.get( sKey::Type(), objtype );

	const bool oldrandomposbdy =
	    objtype=="RandomPosBody" || translt=="RandomPosBody";
	const bool oldpolygonbdy =
	    objtype=="PolygonBody" || translt=="PolygonBody";
	const bool oldmcbdy =  objtype=="MCBody" || translt=="MCBody" ||
	    ioobj.group()=="MarchingCubesSurface";
	if ( !oldrandomposbdy && !oldpolygonbdy && !oldmcbdy )
	    continue;

	BufferString bdytype, oldextension;
	if ( oldrandomposbdy )
	{
	    bdytype = EM::RandomPosBody::typeStr();
	    oldextension = "rdpos";
	}
	else if ( oldpolygonbdy )
	{
	    bdytype = EM::PolygonBody::typeStr();
	    oldextension = "plg";
	}
	else
	{
	    bdytype = EM::MarchingCubesSurface::typeStr();
	    oldextension = "mc";
	}

	FilePath fp( EM::Surface::getSetupFileName(ioobj).buf() );
	fp.setExtension( oldextension );
	StreamProvider oldsp( fp.fullPath().buf() );
	FilePath newfp( fp );
	newfp.setExtension( EMBodyTranslatorGroup::sKeyExtension() );
	if ( oldsp.exists(true) )
	    oldsp.rename( newfp.fullPath().buf(), 0 );

	iopar.set( sKey::Type(), bdytype );
	ioobj.setGroup( EMBodyTranslatorGroup::sGroupName() );
	ioobj.setTranslator( EMBodyTranslatorGroup::sKeyUserWord() );
	ioobj.updateCreationPars();

	mDynamicCastGet(IOStream*,iostrm,&ioobj);
	if ( iostrm )
	    iostrm->fileSpec().setFileName( newfp.fileName() );
	if ( !IOM().commitChanges( ioobj ) )
	{
	    errmsg = uiStrings::phrCannotWriteDBEntry( ioobj.uiName() );
	    return false;
	}
    }

    return true;
}
