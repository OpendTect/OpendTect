/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jul 2005
________________________________________________________________________

-*/

#include "picksettr.h"
#include "pickset.h"
#include "ascstream.h"
#include "ioobjctxt.h"
#include "binidvalset.h"
#include "datapointset.h"
#include "ioobj.h"
#include "dbman.h"
#include "hdf5reader.h"
#include "hdf5writer.h"
#include "hdf5arraynd.h"
#include "iopar.h"
#include "ptrman.h"
#include "separstr.h"
#include "survinfo.h"
#include "streamconn.h"
#include "polygon.h"
#include "trigonometry.h"
#include "keystrs.h"
#include "unitofmeasure.h"
#include "uistrings.h"

static const char* sKeyDirections = "Directions";
static const char* sKeyLabels = "Labels";
static const char* sKeyGroups = "Groups";
static const char* sKeyGeomID = "GeomID";
static const char* sKeyGeomIDs = "GeomIDs";


defineTranslatorGroup(PickSet,"PickSet Group");
defineTranslator(dgb,PickSet,mDGBKey);
mDefSimpleTranslatorioContext( PickSet, Loc )
mDefSimpleTranslatorSelector( PickSet )
uiString PickSetTranslatorGroup::sTypeName( int num)
{ return uiStrings::sPickSet( num ); }


// In earlier versions, 'Type' and 'Category' were mixed
// We want to go to a situation where a pickset is
// Type: either 'Polygon' or 'PickSet'
// Category: empty, or things like 'ArrowAnnotations', 'Text', ...
// This requires some juggling


static bool getCatFromType( const IOPar& iop, BufferString& cat )
{
    FileMultiString fms;
    iop.get( sKey::Type(), fms );
    const int fmssz = fms.size();
    for ( int idx=0; idx<fmssz; idx++ )
    {
	const BufferString typ = fms[idx];
	if ( !typ.isEmpty() && typ != sKey::PickSet() && typ != sKey::Polygon())
	{
	    cat.set( typ );
	    return true;
	}
    }
    return false;
}


static bool getIsPolygon( const IOObj& ioobj, bool& ispoly )
{
    FileMultiString fms;
    ioobj.pars().get( sKey::Type(), fms );
    const int fmssz = fms.size();
    ispoly = false;
    for ( int idx=0; idx<fmssz; idx++ )
    {
	const BufferString typ = fms[idx];
	if ( typ == sKey::PickSet() )
	    return true;
	else if ( typ == sKey::Polygon() )
	    { ispoly = true; return true; }
    }
    return false;
}


bool PickSetTranslator::retrieve( Pick::Set& ps, const IOObj* ioobj,
				  uiString& errmsg )
{
    if ( !ioobj )
    {
	errmsg = uiStrings::phrCannotFindDBEntry( ioobj->uiName() );
	return false;
    }

    mDynamicCast(PickSetTranslator*,PtrMan<PickSetTranslator> tr,
		 ioobj->createTranslator());
    if ( !tr )
    {
	errmsg = uiStrings::phrSelectObjectWrongType( uiStrings::sPickSet() );
	return false;
    }

    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
    if ( !conn )
    {
	errmsg = uiStrings::phrCannotOpen( ioobj->uiName() );
	return false;
    }

    ChangeNotifyBlocker cnb( ps );

    errmsg = tr->read( ps, *conn );
    if ( !errmsg.isEmpty() )
	return false;

    if ( FixedString(ps.category()).isEmpty() )
	ps.setCategory( getCategory(*ioobj,&ps) );

    bool ispoly = false;
    if ( getIsPolygon(*ioobj,ispoly) )
	ps.setIsPolygon( ispoly );

    ps.setName( ioobj->name() );
    return true;
}


bool PickSetTranslator::isPolygon( const IOObj& ioobj )
{
    bool ispoly = false;
    getIsPolygon( ioobj, ispoly );
    return ispoly;
}


BufferString PickSetTranslator::getCategory( const IOObj& ioobj, Pick::Set* ps )
{
    BufferString cat = ioobj.pars().find( sKey::Category() );
    if ( cat.isEmpty() )
    {
	if ( !getCatFromType(ioobj.pars(),cat) && ps )
	    getCatFromType( ps->pars(), cat );
    }
    return cat;
}


bool PickSetTranslator::store( const Pick::Set& ps, const IOObj* ioobj,
			       uiString& errmsg )
{
    ConstRefMan<Pick::Set> psrefman( &ps ); // keep it alive
    if ( !ioobj )
    {
	errmsg = uiStrings::phrCannotFindDBEntry( ioobj->uiName() );
	return false;
    }

    mDynamicCast(PickSetTranslator*,PtrMan<PickSetTranslator> tr,
		 ioobj->createTranslator());
    if ( !tr )
    {
	errmsg = uiStrings::phrSelectObjectWrongType( uiStrings::sPickSet() );
	return false;
    }

    PtrMan<Conn> conn = ioobj->getConn( Conn::Write );
    if ( !conn )
	{ errmsg = uiStrings::phrCannotOpen( ioobj->uiName() ); return false; }

    errmsg = tr->write( ps, *conn );
    if ( !errmsg.isEmpty() )
	{ conn->rollback(); return false; }

    // Now that we have the set, make sure it gets a standard entry in the omf
    bool needcommit = false;
    const FixedString ioobjpstype = ioobj->pars().find(sKey::Type());
    if ( ioobjpstype != ps.type() )
    {
	ioobj->pars().set( sKey::Type(), ps.type() );
	needcommit = true;
    }
    const FixedString ioobjpscat = ioobj->pars().find( sKey::Category() );
    if ( ioobjpscat != ps.category() )
    {
	ioobj->pars().set( sKey::Category(), ps.category() );
	needcommit = true;
    }

    if ( needcommit )
	DBM().setEntry( *ioobj );

    return true;
}


static bool wantHDF5()
{
    return HDF5::isEnabled( HDF5::sPickSetType() );
}


const char* dgbPickSetTranslator::defExtension() const
{
    return wantHDF5() ? HDF5::sFileExtension() : "pck";
}


class dgbPickSetTranslatorBackEnd
{
public:

    virtual		~dgbPickSetTranslatorBackEnd()	{}
    virtual uiString	read(Pick::Set&)		= 0;
    virtual uiString	write(const Pick::Set&)		= 0;

void fillPar( const Pick::Set& ps, IOPar& iop ) const
{
    ps.fillPar( iop );
    iop.set( sKey::ZUnit(),
	     UnitOfMeasure::surveyDefZStorageUnit()->name() );
}

};


class dgbPickSetTranslatorStreamBackEnd : public dgbPickSetTranslatorBackEnd
{
public:
			dgbPickSetTranslatorStreamBackEnd( od_stream& strm )
			    : strm_(strm)		{}

    virtual uiString	read(Pick::Set&);
    virtual uiString	write(const Pick::Set&);

    od_stream&		strm_;

};


class dgbPickSetTranslatorHDF5BackEnd : public dgbPickSetTranslatorBackEnd
{
public:

			dgbPickSetTranslatorHDF5BackEnd( const char* fnm )
			    : filenm_(fnm)		{}
			~dgbPickSetTranslatorHDF5BackEnd()
			{ delete rdr_; delete wrr_; }

    virtual uiString	read(Pick::Set&);
    virtual uiString	write(const Pick::Set&);

    const BufferString	filenm_;
    HDF5::DataSetKey	dsky_;
    HDF5::Reader*	rdr_		= 0;
    HDF5::Writer*	wrr_		= 0;

    bool		setPositions(Pick::Set&,uiRetVal&);
    void		setDirs(Pick::Set&);
    void		setGroups(Pick::Set&);
    void		setTexts(Pick::Set&);
    void		setGeomIDs(Pick::Set&,Pos::GeomID);

    bool		putPositions(const Pick::Set&,uiRetVal&);
    bool		putDirs(const Pick::Set&,uiRetVal&);
    bool		putGroups(const Pick::Set&,uiRetVal&);
    bool		putTexts(const Pick::Set&,uiRetVal&);
    bool		putGeomIDs(const Pick::Set&,uiRetVal&);

    Array2D<double>*	readDArr(uiRetVal&);

};


uiString dgbPickSetTranslator::read( Pick::Set& ps, Conn& conn )
{
    if ( !conn.forRead() || !conn.isStream() )
	return uiStrings::sCantOpenInpFile();

    od_istream& strm = ((StreamConn&)conn).iStream();
    const BufferString fnm( strm.fileName() );
    bool usehdf5 = HDF5::isHDF5File( fnm );

    dgbPickSetTranslatorBackEnd* be;
    if ( !usehdf5 )
	be = new dgbPickSetTranslatorStreamBackEnd( strm );
    else
    {
	conn.close();
	be = new dgbPickSetTranslatorHDF5BackEnd( fnm );
    }

    uiString ret = be->read( ps );
    delete be;
    return ret;
}


uiString dgbPickSetTranslator::write( const Pick::Set& ps, Conn& conn )
{
    if ( !conn.forWrite() || !conn.isStream() )
	return uiStrings::sCantOpenOutpFile();

    od_ostream& strm = ((StreamConn&)conn).oStream();

    dgbPickSetTranslatorBackEnd* be;
    if ( !wantHDF5() )
	be = new dgbPickSetTranslatorStreamBackEnd( strm );
    else
    {
	const BufferString fnm( strm.fileName() );
	conn.close();
	be = new dgbPickSetTranslatorHDF5BackEnd( fnm );
    }

    uiString ret = be->write( ps );
    delete be;
    return ret;
}


uiString dgbPickSetTranslatorStreamBackEnd::read( Pick::Set& ps )
{
    ascistream astrm( static_cast<od_istream&>(strm_) );
    if ( !astrm.isOK() )
	return uiStrings::sCantOpenInpFile();
    else if ( !astrm.isOfFileType(mTranslGroupName(PickSet)) )
	return uiStrings::phrSelectObjectWrongType( uiStrings::sPickSet() );
    if ( atEndOfSection(astrm) )
	astrm.next();
    if ( atEndOfSection(astrm) )
	return uiStrings::sNoValidData();

    if ( astrm.hasKeyword("Ref") )
    {
	Pick::Set::Disp disp;
	// In old format we can find mulitple pick sets. Just gather them all
	// in the pick set
	for ( int ips=0; !atEndOfSection(astrm); ips++ )
	{
	    astrm.next();
	    if ( astrm.hasKeyword(sKey::Color()) )
	    {
		disp.mkstyle_.color_.use( astrm.value() );
		disp.mkstyle_.color_.setTransparency( 0 );
		astrm.next();
	    }
	    if ( astrm.hasKeyword(sKey::Size()) )
	    {
		disp.mkstyle_.size_ = astrm.getIValue();
		astrm.next();
	    }
	    if ( astrm.hasKeyword(Pick::Set::sKeyMarkerType()) )
	    {
		// OD::MarkerStyle3D::Type used to start with -1. This has
		// changed and thus a '+1' is needed to keep the same shapes
		const int markertype = astrm.getIValue() + 1;
		disp.mkstyle_.type_ = (OD::MarkerStyle3D::Type)markertype;
		astrm.next();
	    }
	    while ( !atEndOfSection(astrm) )
	    {
		Pick::Location loc;
		if ( !loc.fromString(astrm.keyWord()) )
		    break;

		ps.add( loc );
		astrm.next();
	    }
	    while ( !atEndOfSection(astrm) ) astrm.next();
	    astrm.next();
	}
	ps.setDisp( disp );
    }
    else // Post-3.2 format
    {
	IOPar iopar; iopar.getFrom( astrm );
	ps.usePar( iopar );

	astrm.next();
	while ( !atEndOfSection(astrm) )
	{
	    Pick::Location loc;
	    if ( loc.fromString(astrm.keyWord()) )
		ps.add( loc );

	    astrm.next();
	}
    }

    return ps.size() ? uiString::empty() : uiStrings::sNoValidData();
}


uiString dgbPickSetTranslatorStreamBackEnd::write( const Pick::Set& ps )
{
    ascostream astrm( static_cast<od_ostream&>(strm_) );
    astrm.putHeader( mTranslGroupName(PickSet) );
    if ( !astrm.isOK() )
	return uiStrings::sCantOpenOutpFile();

    IOPar par; fillPar( ps, par );
    par.putTo( astrm );

    od_ostream& strm = astrm.stream();
    Pick::SetIter psiter( ps );
    while ( psiter.next() )
    {
	BufferString str;
	psiter.get().toString( str );
	strm << str << od_newline;
    }

    astrm.newParagraph();
    return astrm.isOK() ? uiString::empty()
			: uiStrings::phrCannotWrite( uiStrings::sPickSet() );
}


Array2D<double>* dgbPickSetTranslatorHDF5BackEnd::readDArr( uiRetVal& uirv )
{
    ArrayND<double>* arrnd = HDF5::ArrayNDTool<double>::createArray( *rdr_ );
    mDynamicCastGet( Array2D<double>*, arr2d, arrnd );
    if ( !arr2d )
	{ uirv = HDF5::Access::sCannotReadDataSet( dsky_ ); return 0; }

    HDF5::ArrayNDTool<double> arrtool( *arr2d );
    uirv = arrtool.getAll( *rdr_ );
    if ( !uirv.isOK() )
	{ delete arr2d; arr2d = 0; }

    return arr2d;
}


bool dgbPickSetTranslatorHDF5BackEnd::setPositions( Pick::Set& ps,
						    uiRetVal& uirv )
{
    dsky_.setDataSetName( sKey::Positions() );
    if ( !rdr_->setScope(dsky_) )
	{ uirv = HDF5::Access::sDataSetNotFound( dsky_ ); return false; }

    Array2D<double>* posns = readDArr( uirv );
    if ( !posns )
	return false;

    const int nrpoints = posns->getSize( 1 );
    for ( int ipt=0; ipt<nrpoints; ipt++ )
    {
	const double x = posns->get( 0, ipt );
	const double y = posns->get( 1, ipt );
	const double z = posns->get( 2, ipt );
	ps.add( Pick::Location(x,y,z) );
    }

    delete posns;
    return true;
}


void dgbPickSetTranslatorHDF5BackEnd::setDirs( Pick::Set& ps )
{
    dsky_.setDataSetName( sKeyDirections );
    if ( !rdr_->setScope(dsky_) )
	return;

    uiRetVal uirv;
    Array2D<double>* dirs = readDArr( uirv );
    if ( !dirs )
	return;

    Pick::SetIter4Edit it( ps );
    while ( it.next() )
    {
	int ipt = it.curIdx();
	const Sphere sph( dirs->get(0,ipt), dirs->get(1,ipt), dirs->get(2,ipt));
	it.get().setDir( sph );
    }

    delete dirs;
}


void dgbPickSetTranslatorHDF5BackEnd::setGroups( Pick::Set& ps )
{
    dsky_.setDataSetName( sKeyGroups );
    if ( !rdr_->setScope(dsky_) )
	return;
    //TODO
}


void dgbPickSetTranslatorHDF5BackEnd::setTexts( Pick::Set& ps )
{
    dsky_.setDataSetName( sKeyLabels );
    if ( !rdr_->setScope(dsky_) )
	return;
    //TODO
}


void dgbPickSetTranslatorHDF5BackEnd::setGeomIDs( Pick::Set& ps,
						  Pos::GeomID geomid )
{
    //TODO
}


uiString dgbPickSetTranslatorHDF5BackEnd::read( Pick::Set& ps )
{
    rdr_ = HDF5::mkReader();
    if ( !rdr_ )
	return HDF5::Access::sHDF5NotAvailable( filenm_ );

    uiRetVal uirv = rdr_->open( filenm_ );
    if ( !uirv.isOK() )
	return uirv;

    rdr_->setScope( dsky_ );
    IOPar iop;
    uirv = rdr_->getInfo( iop );
    if ( uirv.isOK() )
	ps.usePar( iop );

    if ( !setPositions(ps,uirv) )
	return uirv;

    bool havedirs = false, havetexts = false, havegrps = false;
    iop.getYN( sKeyDirections, havedirs );
    iop.getYN( sKeyLabels, havetexts );
    iop.getYN( sKeyGroups, havegrps );

    if ( havedirs )
	setDirs( ps );
    if ( havetexts )
	setTexts( ps );
    if ( havegrps )
	setGroups( ps );

    Pos::GeomID geomid = mUdfGeomID;;
    iop.get( sKeyGeomID, geomid );
    setGeomIDs( ps, geomid );

    return uirv;
}


bool dgbPickSetTranslatorHDF5BackEnd::putPositions( const Pick::Set& ps,
						    uiRetVal& uirv )
{
    Array2DImpl<double> posns( 3, ps.size() );
    Pick::SetIter psiter( ps );
    while ( psiter.next() )
    {
	const Coord3& pos = psiter.get().pos();
	const int ipos = psiter.curIdx();
	posns.set( 0, ipos, pos.x_ );
	posns.set( 1, ipos, pos.y_ );
	posns.set( 2, ipos, pos.z_ );
    }
    HDF5::ArrayNDTool<double> arrtool( posns );
    dsky_.setDataSetName( sKey::Positions() );
    uirv = arrtool.put( *wrr_, dsky_ );
    return uirv.isOK();
}


bool dgbPickSetTranslatorHDF5BackEnd::putDirs( const Pick::Set& ps,
						    uiRetVal& uirv )
{
    Array2DImpl<double> dirs( 3, ps.size() );
    Pick::SetIter psiter( ps );
    while ( psiter.next() )
    {
	const Sphere& dir = psiter.get().dir();
	const int ipos = psiter.curIdx();
	dirs.set( 0, ipos, dir.radius_ );
	dirs.set( 1, ipos, dir.theta_ );
	dirs.set( 2, ipos, dir.phi_ );
    }
    HDF5::ArrayNDTool<double> arrtool( dirs );
    dsky_.setDataSetName( sKeyDirections );
    uirv = arrtool.put( *wrr_, dsky_ );
    return uirv.isOK();
}


bool dgbPickSetTranslatorHDF5BackEnd::putGroups( const Pick::Set& ps,
						 uiRetVal& uirv )
{
    typedef Pick::GroupLabel::ID::IDType IDType;
    TypeSet<IDType> lblids;
    Pick::SetIter psiter( ps );
    while ( psiter.next() )
	lblids += psiter.get().groupLabelID().getI();
    dsky_.setDataSetName( sKeyGroups );
    uirv = wrr_->put( dsky_, lblids );
    return uirv.isOK();
}


bool dgbPickSetTranslatorHDF5BackEnd::putTexts( const Pick::Set& ps,
						uiRetVal& uirv )
{
    BufferStringSet lbls;
    Pick::SetIter psiter( ps );
    while ( psiter.next() )
	lbls.add( psiter.get().text() );
    dsky_.setDataSetName( sKeyLabels );
    uirv = wrr_->put( dsky_, lbls );
    return uirv.isOK();
}


bool dgbPickSetTranslatorHDF5BackEnd::putGeomIDs( const Pick::Set& ps,
						  uiRetVal& uirv )
{
    TypeSet<Pos::GeomID> geomids;
    Pick::SetIter psiter( ps );
    while ( psiter.next() )
	geomids += psiter.get().geomID();
    dsky_.setDataSetName( sKeyGeomIDs );
    uirv = wrr_->put( dsky_, geomids );
    return uirv.isOK();
}


uiString dgbPickSetTranslatorHDF5BackEnd::write( const Pick::Set& ps )
{
    wrr_ = HDF5::mkWriter();
    if ( !wrr_ )
	return HDF5::Access::sHDF5NotAvailable( filenm_ );

    uiRetVal uirv = wrr_->open( filenm_ );
    if ( !uirv.isOK() )
	return uirv;

    MonitorLock ml( ps );
    const bool havedirs = ps.haveDirections();
    const bool havetexts = ps.haveTexts();
    const bool havegrps = ps.haveGroupLabels();
    const bool havegeomids = ps.haveMultipleGeomIDs();
    const int sz = ps.size();

    IOPar iop;
    fillPar( ps, iop );
    if ( sz < 1 )
	return uirv;

    iop.setYN( sKeyDirections, havedirs );
    iop.setYN( sKeyLabels, havetexts );
    iop.setYN( sKeyGroups, havegrps );
    if ( !havegeomids )
	iop.set( sKeyGeomID, ps.first().trcKey().geomID() );

    uirv = wrr_->putInfo( dsky_, iop );
    if ( !uirv.isOK() )
	return uirv;

    if ( ps.isEmpty() )
	return uiString::empty();

    if ( !putPositions(ps,uirv)  )
	return uirv;
    if ( havedirs && !putDirs(ps,uirv) )
	return uirv;
    if ( havegrps && !putGroups(ps,uirv) )
	return uirv;
    if ( havetexts && !putTexts(ps,uirv) )
	return uirv;
    if ( havegeomids && !putGeomIDs(ps,uirv) )
	return uirv;

    return uirv;
}
