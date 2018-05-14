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
#include "keystrs.h"
#include "unitofmeasure.h"
#include "uistrings.h"

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

    virtual uiString	read(Pick::Set&);
    virtual uiString	write(const Pick::Set&);

    const BufferString	filenm_;

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
    psiter.retire();

    astrm.newParagraph();
    return astrm.isOK() ? uiString::empty()
			: uiStrings::phrCannotWrite( uiStrings::sPickSet() );
}


uiString dgbPickSetTranslatorHDF5BackEnd::read( Pick::Set& ps )
{
    PtrMan<HDF5::Reader> rdr = HDF5::mkReader();
    if ( !rdr )
	return HDF5::Access::sHDF5NotAvailable( filenm_ );

    uiRetVal uirv = rdr->open( filenm_ );
    if ( !uirv.isOK() )
	return uirv;

    HDF5::DataSetKey dsky;
    rdr->setScope( dsky );
    IOPar iop;
    uirv = rdr->getInfo( iop );
    if ( uirv.isOK() )
	ps.usePar( iop );

    dsky.setDataSetName( sKey::Positions() );
    if ( !rdr->setScope(dsky) )
	return HDF5::Access::sDataSetNotFound( dsky );

    ArrayND<double>* posns = HDF5::ArrayNDTool<double>::createArray( *rdr );
    if ( !posns )
	return HDF5::Access::sCannotReadDataSet( dsky );

    HDF5::ArrayNDTool<double> arrtool( *posns );
    uirv = arrtool.getAll( *rdr );
    if ( !uirv.isOK() )
	return uirv;

    //TODO read the rest

    return uirv;
}


uiString dgbPickSetTranslatorHDF5BackEnd::write( const Pick::Set& ps )
{
    PtrMan<HDF5::Writer> wrr = HDF5::mkWriter();
    if ( !wrr )
	return HDF5::Access::sHDF5NotAvailable( filenm_ );

    uiRetVal uirv = wrr->open( filenm_ );
    if ( !uirv.isOK() )
	return uirv;

    IOPar iop; fillPar( ps, iop );
    HDF5::DataSetKey dsky;
    uirv = wrr->putInfo( dsky, iop );
    if ( !uirv.isOK() )
	return uirv;

    const int sz = ps.size();
    if ( sz < 1 )
	return uiString::empty();

    dsky.setDataSetName( sKey::Positions() );
    Array2DImpl<double> posns( 3, sz );
    Pick::SetIter psiter( ps );
    while ( psiter.next() )
    {
	const Coord3& pos = psiter.get().pos();
	const int ipos = psiter.curIdx();
	posns.set( 0, ipos, pos.x_ );
	posns.set( 1, ipos, pos.y_ );
	posns.set( 2, ipos, pos.z_ );
    }
    psiter.retire();
    HDF5::ArrayNDTool<double> arrtool( posns );
    uirv = arrtool.put( *wrr, dsky );
    if ( !uirv.isOK() )
	return uirv;

    //TODO write the rest

    return uirv;
}
