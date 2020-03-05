/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jul 2005
________________________________________________________________________

-*/

#include "picksettr.h"
#include "pickset.h"
#include "picksetmanager.h"
#include "ascstream.h"
#include "ioobjctxt.h"
#include "binnedvalueset.h"
#include "bufstring.h"
#include "datapointset.h"
#include "ioobj.h"
#include "file.h"
#include "filepath.h"
#include "hdf5reader.h"
#include "hdf5writer.h"
#include "hdf5arraynd.h"
#include "iopar.h"
#include "ptrman.h"
#include "separstr.h"
#include "survinfo.h"
#include "streamconn.h"
#include "trckey.h"
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
{ return uiStrings::sPointSet( num ); }


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


uiRetVal PickSetTranslator::retrieve( Pick::Set& ps, const IOObj& ioobj )
{
    mDynamicCast(PickSetTranslator*,PtrMan<PickSetTranslator> tr,
		 ioobj.createTranslator());
    if ( !tr )
	return uiStrings::phrSelectObjectWrongType( uiStrings::sPointSet() );

    ChangeNotifyBlocker cnb( ps );

    uiRetVal uirv = tr->read( ps, ioobj );
    if ( uirv.isOK() )
    {
	if ( FixedString(ps.category()).isEmpty() )
	    ps.setCategory( getCategory(ioobj,&ps) );

	bool ispoly = true;
	if ( getIsPolygon(ioobj,ispoly) )
	    ps.setIsPolygon( ispoly );

	ps.setName( ioobj.name() );
    }
    return uirv;
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


uiRetVal PickSetTranslator::store( const Pick::Set& ps, const IOObj& ioobj )
{
    ConstRefMan<Pick::Set> psrefman( &ps ); // keep it alive
    mDynamicCast(PickSetTranslator*,PtrMan<PickSetTranslator> tr,
		 ioobj.createTranslator());
    if ( !tr )
	return uiStrings::phrSelectObjectWrongType( uiStrings::sPointSet() );

    uiRetVal uirv = tr->write( ps, ioobj );

    Pick::SetMGR().writeDisplayPars( ioobj.key(), ps );

    if ( uirv.isOK() )
    {
	// Now make sure it gets a standard entry in the omf
	bool needcommit = false;
	const FixedString ioobjpstype = ioobj.pars().find(sKey::Type());
	if ( ioobjpstype != ps.type() )
	{
	    ioobj.pars().set( sKey::Type(), ps.type() );
	    needcommit = true;
	}
	const FixedString ioobjpscat = ioobj.pars().find( sKey::Category() );
	if ( ioobjpscat != ps.category() )
	{
	    ioobj.pars().set( sKey::Category(), ps.category() );
	    needcommit = true;
	}

	if ( needcommit )
	    ioobj.commitChanges();
    }
    return uirv;
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

			dgbPickSetTranslatorBackEnd( const char* fnm )
			    : filenm_(fnm)		{}
    virtual		~dgbPickSetTranslatorBackEnd()	{}
    virtual uiRetVal	read(Pick::Set&)		= 0;
    virtual uiRetVal	write(const Pick::Set&)		= 0;

void fillPar( const Pick::Set& ps, IOPar& iop ) const
{
    ps.fillPar( iop );
    iop.set( sKey::ZUnit(),
	     UnitOfMeasure::surveyDefZStorageUnit()->name() );
}

    const BufferString	filenm_;
protected:
    Pick::Set::Disp	readDisp();
};


Pick::Set::Disp dgbPickSetTranslatorBackEnd::readDisp()
{
    Pick::Set::Disp disp;
    File::Path fp( filenm_ );
    fp.setExtension( "disp" );
    if ( !fp.exists() )
	return disp;

    od_istream dispstrm( fp.fullPath() );
    ascistream adispstrm( dispstrm );
    if ( !adispstrm.isOK() )
	return disp;

    IOPar iopar;
    iopar.getFrom( adispstrm );
    if ( iopar.isEmpty() )
	return disp;

    int picksz;
    if ( iopar.get(sKey::Size(),picksz) )
    {
	disp.mkstyle_.size_ = picksz;
    }

    Color mkcolor;
    if ( iopar.get(sKey::Color(),mkcolor) )
    {
	disp.mkstyle_.color_ = mkcolor;
	disp.mkstyle_.color_.setTransparency( 0 );
    }
    BufferString mkststr;
    if ( iopar.get(Pick::Set::sKeyMarkerType(),mkststr) )
    {
	OD::MarkerStyle3D::TypeDef().parse( iopar,
		     Pick::Set::sKeyMarkerType(), disp.mkstyle_.type_ );
    }
    Color lncolor;
    if ( iopar.get(Pick::Set::sKeyLineColor(),lncolor) )
    {
	disp.lnstyle_.color_ = lncolor ;
    }
    int lnsz;
    if ( iopar.get(Pick::Set::sKeyWidth(),lnsz) )
	disp.lnstyle_.width_ = lnsz;

    BufferString lnststr;
    if ( iopar.get(Pick::Set::sKeyLineType(),lnststr) )
	OD::LineStyle::TypeDef().parse( iopar,
		     Pick::Set::sKeyLineType(), disp.lnstyle_.type_ );

    BufferString connect;
    if ( iopar.get(Pick::Set::sKeyConnect(),connect) )
	Pick::Set::Disp::ConnectionDef().parse( iopar,
		     Pick::Set::sKeyConnect(), disp.connect_ );

    Color fillcolor;
    if ( iopar.get(Pick::Set::sKeySurfaceColor(),fillcolor) )
    {
	disp.fillcol_ = fillcolor;
    }

    bool fill;
    if ( iopar.getYN(Pick::Set::sKeyFill(), fill) )
	disp.filldodraw_ = fill;

    bool line;
    if ( iopar.getYN(Pick::Set::sKeyLine(), line) )
	disp.linedodraw_ = line;

    return disp;
}


class dgbPickSetTranslatorStreamBackEnd : public dgbPickSetTranslatorBackEnd
{
public:

			dgbPickSetTranslatorStreamBackEnd( const char* fnm )
			    : dgbPickSetTranslatorBackEnd(fnm)	    {}

    virtual uiRetVal	read(Pick::Set&);
    virtual uiRetVal	write(const Pick::Set&);
/*
protected:
    Pick::Set::Disp	readDisp();*/


};


class dgbPickSetTranslatorHDF5BackEnd : public dgbPickSetTranslatorBackEnd
{
public:

			dgbPickSetTranslatorHDF5BackEnd( const char* fnm )
			    : dgbPickSetTranslatorBackEnd(fnm)	{}
			~dgbPickSetTranslatorHDF5BackEnd()
			{ delete rdr_; delete wrr_; }

    virtual uiRetVal	read(Pick::Set&);
    virtual uiRetVal	write(const Pick::Set&);

    HDF5::Reader*	rdr_		= nullptr;
    HDF5::Writer*	wrr_		= nullptr;

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

    template <class T>
    Array2D<T>*		readFPArr(const HDF5::DataSetKey&,uiRetVal&);

};


uiRetVal dgbPickSetTranslator::read( Pick::Set& ps, const IOObj& ioobj )
{
    const BufferString fnm( ioobj.mainFileName() );
    bool usehdf5 = HDF5::isHDF5File( fnm );

    PtrMan<dgbPickSetTranslatorBackEnd> be;
    if ( !usehdf5 )
	be = new dgbPickSetTranslatorStreamBackEnd( fnm );
    else
	be = new dgbPickSetTranslatorHDF5BackEnd( fnm );

    return be->read( ps );
}


uiRetVal dgbPickSetTranslator::write( const Pick::Set& ps, const IOObj& ioobj )
{
    PtrMan<dgbPickSetTranslatorBackEnd> be;
    const BufferString fnm( ioobj.mainFileName() );
    if ( !wantHDF5() )
	be = new dgbPickSetTranslatorStreamBackEnd( fnm );
    else
	be = new dgbPickSetTranslatorHDF5BackEnd( fnm );

    return be->write( ps );
}


uiRetVal dgbPickSetTranslatorStreamBackEnd::read( Pick::Set& ps )
{
    od_istream strm( filenm_ );
    ascistream astrm( strm );
    if ( !astrm.isOK() )
	return uiStrings::phrCannotOpenInpFile();
    else if ( !astrm.isOfFileType(mTranslGroupName(PickSet)) )
	return uiStrings::phrSelectObjectWrongType( uiStrings::sPointSet() );
    if ( atEndOfSection(astrm) )
	astrm.next();
    if ( atEndOfSection(astrm) )
	return uiStrings::sNoValidData();

    uiRetVal uirv;
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
		const int markertype = astrm.getIValue() + 1;
		disp.mkstyle_.type_ = (OD::MarkerStyle3D::Type)markertype;
		astrm.next();
	    }
	    disp = readDisp();
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
	const Pick::Set::Disp disp( readDisp() );
	astrm.next();
	while ( !atEndOfSection(astrm) )
	{
	    Pick::Location loc;
	    if ( loc.fromString(astrm.keyWord()) )
		ps.add( loc );

	    astrm.next();
	}
	ps.setDisp( disp );
    }

    if ( ps.isEmpty() )
	uirv = uiStrings::sNoValidData();
    return uirv;
}


uiRetVal dgbPickSetTranslatorStreamBackEnd::write( const Pick::Set& ps )
{
    od_ostream strm( filenm_ );
    ascostream astrm( strm );
    astrm.putHeader( mTranslGroupName(PickSet) );
    if ( !astrm.isOK() )
	return uiStrings::phrCannotOpenOutpFile();

    IOPar par; fillPar( ps, par );
    par.putTo( astrm );

    Pick::SetIter psiter( ps );
    while ( psiter.next() )
    {
	BufferString str;
	psiter.get().toString( str );
	strm << str << od_newline;
    }

    Pick::Set::Disp disp;
    File::Path fp(filenm_);
    fp.setExtension( "disp" );
    od_ostream dispstrm( fp.fullPath() );
    ascostream adispstrm( dispstrm );
    IOPar disppar;
    disppar.merge( ps.disppars() );
    disppar.putTo( adispstrm );

    astrm.newParagraph();
    uiRetVal uirv;
    if ( !astrm.isOK() )
	uirv = uiStrings::phrCannotWrite( uiStrings::sPointSet() );

    return uirv;
}


template <class T>
Array2D<T>* dgbPickSetTranslatorHDF5BackEnd::readFPArr(
				const HDF5::DataSetKey& dsky, uiRetVal& uirv )
{
    ArrayND<T>* arrnd = HDF5::ArrayNDTool<T>::createArray( dsky, *rdr_ );
    mDynamicCastGet( Array2D<T>*, arr2d, arrnd );
    if ( !arr2d )
    {
	uirv.add( HDF5::Access::sCannotReadDataSet( dsky ) );
	return nullptr;
    }

    HDF5::ArrayNDTool<T> arrtool( *arr2d );
    uirv = arrtool.getAll( dsky, *rdr_ );
    if ( !uirv.isOK() )
	deleteAndZeroPtr( arr2d );

    return arr2d;
}


bool dgbPickSetTranslatorHDF5BackEnd::setPositions( Pick::Set& ps,
						    uiRetVal& uirv )
{
    const HDF5::DataSetKey dsky( nullptr, sKey::Position(mPlural) );
    Array2D<double>* posns = readFPArr<double>( dsky, uirv );
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
    const HDF5::DataSetKey dsky( nullptr, sKeyDirections );
    uiRetVal uirv;
    Array2D<float>* dirs = readFPArr<float>( dsky, uirv );
    if ( !dirs )
	return;

    const ArrayNDInfo::size_type arrsz = dirs->getSize( 1 );
    Pick::SetIter4Edit it( ps );
    while ( it.next() )
    {
	const int idx = it.curIdx();
	if ( idx >= arrsz )
	    break;
	const Sphere sph( dirs->get(0,idx), dirs->get(1,idx), dirs->get(2,idx));
	it.get().setDir( sph );
    }

    delete dirs;
}


void dgbPickSetTranslatorHDF5BackEnd::setGroups( Pick::Set& ps )
{
    const HDF5::DataSetKey dsky( nullptr, sKeyGroups );
    TypeSet<Pick::GroupLabel::ID::IDType> lblids;
    uiRetVal uirv = rdr_->get( dsky, lblids );
    if ( !uirv.isOK() )
	return;

    Pick::SetIter4Edit it( ps );
    while ( it.next() )
    {
	const int idx = it.curIdx();
	if ( idx >= lblids.size() )
	    break;
	it.get().setGroupLabelID( Pick::GroupLabel::ID(lblids[idx]) );
    }
}


void dgbPickSetTranslatorHDF5BackEnd::setTexts( Pick::Set& ps )
{
    const HDF5::DataSetKey dsky( nullptr, sKeyLabels );
    BufferStringSet lbls;
    uiRetVal uirv = rdr_->get( dsky, lbls );
    if ( !uirv.isOK() || lbls.isEmpty() )
	return;

    Pick::SetIter4Edit it( ps );
    while ( it.next() )
    {
	const int idx = it.curIdx();
	if ( idx >= lbls.size() )
	    break;
	it.get().setText( lbls.get(idx) );
    }
}


void dgbPickSetTranslatorHDF5BackEnd::setGeomIDs( Pick::Set& ps,
						  Pos::GeomID geomid )
{
    GeomIDSet geomids;
    if ( !mIsUdfGeomID(geomid) )
	geomids.setSize( ps.size(), geomid );
    else
    {
	const HDF5::DataSetKey dsky( nullptr, sKeyGeomIDs );
	GeomIDSet::IntSet ints;
	uiRetVal uirv = rdr_->get( dsky, ints );
	if ( !uirv.isOK() )
	    return;

	geomids = GeomIDSet( ints );
    }

    Pick::SetIter4Edit it( ps );
    while ( it.next() )
    {
	const int idx = it.curIdx();
	if ( idx >= geomids.size() )
	    break;

	TrcKey tk;
	tk.setGeomID( geomids[idx] );
	tk.setFrom( it.get().pos().getXY() );
	it.get().setTrcKey( tk );
    }
}


uiRetVal dgbPickSetTranslatorHDF5BackEnd::read( Pick::Set& ps )
{
    rdr_ = HDF5::mkReader();
    if ( !rdr_ )
	return HDF5::Access::sHDF5NotAvailable( filenm_ );

    uiRetVal uirv = rdr_->open( filenm_ );
    if ( !uirv.isOK() )
	return uirv;

    IOPar iop;
    uirv = rdr_->get( iop );
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

    Pos::GeomID geomid = mUdfGeomID;
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
    const HDF5::DataSetKey dsky( nullptr, sKey::Position(mPlural) );
    uirv = arrtool.put( *wrr_, dsky );
    return uirv.isOK();
}


bool dgbPickSetTranslatorHDF5BackEnd::putDirs( const Pick::Set& ps,
						    uiRetVal& uirv )
{
    Array2DImpl<float> dirs( 3, ps.size() );
    Pick::SetIter psiter( ps );
    while ( psiter.next() )
    {
	const Sphere& dir = psiter.get().dir();
	const int ipos = psiter.curIdx();
	dirs.set( 0, ipos, dir.radius_ );
	dirs.set( 1, ipos, dir.theta_ );
	dirs.set( 2, ipos, dir.phi_ );
    }
    HDF5::ArrayNDTool<float> arrtool( dirs );
    const HDF5::DataSetKey dsky( nullptr, sKeyDirections );
    uirv = arrtool.put( *wrr_, dsky );
    return uirv.isOK();
}


bool dgbPickSetTranslatorHDF5BackEnd::putGroups( const Pick::Set& ps,
						 uiRetVal& uirv )
{
    TypeSet<Pick::GroupLabel::ID::IDType> lblids;
    Pick::SetIter psiter( ps );
    while ( psiter.next() )
	lblids += psiter.get().groupLabelID().getI();
    const HDF5::DataSetKey dsky( nullptr, sKeyGroups );
    uirv = wrr_->put( dsky, lblids );
    return uirv.isOK();
}


bool dgbPickSetTranslatorHDF5BackEnd::putTexts( const Pick::Set& ps,
						uiRetVal& uirv )
{
    BufferStringSet lbls;
    Pick::SetIter psiter( ps );
    while ( psiter.next() )
	lbls.add( psiter.get().text() );
    const HDF5::DataSetKey dsky( nullptr, sKeyLabels );
    uirv = wrr_->put( dsky, lbls );
    return uirv.isOK();
}


bool dgbPickSetTranslatorHDF5BackEnd::putGeomIDs( const Pick::Set& ps,
						  uiRetVal& uirv )
{
    GeomIDSet geomids;
    Pick::SetIter psiter( ps );
    while ( psiter.next() )
	geomids += psiter.get().geomID();
    GeomIDSet::IntSet ints;
    geomids.getIntSet( ints );
    const HDF5::DataSetKey dsky( nullptr, sKeyGeomIDs );
    uirv = wrr_->put( dsky, ints );
    return uirv.isOK();
}


uiRetVal dgbPickSetTranslatorHDF5BackEnd::write( const Pick::Set& ps )
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

    IOPar iop;
    fillPar( ps, iop );

    iop.setYN( sKeyDirections, havedirs );
    iop.setYN( sKeyLabels, havetexts );
    iop.setYN( sKeyGroups, havegrps );
    if ( havegeomids )
	iop.removeWithKey( sKeyGeomID );
    else
	iop.set( sKeyGeomID, ps.first().trcKey().geomID() );

    uirv = wrr_->set( iop );
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
