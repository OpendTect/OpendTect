/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Satyaki Maitra
 * DATE     : April 2010
-*/

static const char* rcsID = "$Id: emioobjinfo.cc,v 1.1 2010-04-27 05:32:14 cvssatyaki Exp $";

#include "emioobjinfo.h"
#include "emsurfaceio.h"
#include "emsurfacetr.h"
#include "embodytr.h"
#include "ioman.h"
#include "iodir.h"
#include "iopar.h"
#include "survinfo.h"
#include "cubesampling.h"
#include "bufstringset.h"

#define mGoToEMDir() \
    IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::Surf)->id) )

using namespace EM;

IOObjInfo::IOObjInfo( const IOObj* ioobj )
    : ioobj_(ioobj ? ioobj->clone() : 0)
    , reader_(0)
{
    setType();
}


IOObjInfo::IOObjInfo( const IOObj& ioobj )
    : ioobj_(ioobj.clone())
    , reader_(0)
{
    setType();
}


IOObjInfo::IOObjInfo( const MultiID& id )
    : ioobj_(IOM().get(id))
    , reader_(0)
{
    setType();
}


IOObjInfo::IOObjInfo( const char* ioobjnm )
    : ioobj_(0)
    , reader_(0)
{
    mGoToEMDir();
    ioobj_ = IOM().getLocal( ioobjnm );
}


IOObjInfo::IOObjInfo( const IOObjInfo& sii )
    : type_(sii.type_)
    , reader_(0)
{
    ioobj_ = sii.ioobj_ ? sii.ioobj_->clone() : 0;
}


IOObjInfo::~IOObjInfo()
{
    delete ioobj_;
}


bool IOObjInfo::isOK() const
{
    return ioobj_;
}


IOObjInfo& IOObjInfo::operator =( const IOObjInfo& sii )
{
    if ( &sii != this )
    {
	delete ioobj_;
	ioobj_ = sii.ioobj_ ? sii.ioobj_->clone() : 0;
    	type_ = sii.type_;
    }
    return *this;
}


void IOObjInfo::setType()
{
    if ( !ioobj_ )
	return;

    const BufferString trgrpnm( ioobj_->group() );
    if ( trgrpnm == EMHorizon3DTranslatorGroup::keyword() )
	type_ = IOObjInfo::Horizon3D;
    else if ( trgrpnm == EMHorizon2DTranslatorGroup::keyword() )
	type_ = IOObjInfo::Horizon2D;
    else if ( trgrpnm == EMFaultStickSetTranslatorGroup::keyword() )
	type_ = IOObjInfo::FaultStickSet;
    else if ( trgrpnm == EMFault3DTranslatorGroup::keyword() )
	type_ = IOObjInfo::Fault;
    else if( trgrpnm == EMBodyTranslatorGroup::sKeyword() )
	type_ = IOObjInfo::Horizon2D;
}


#define mGetReader \
if ( !reader_ ) \
    reader_ = new dgbSurfaceReader( *ioobj_, ioobj_->group() );

bool IOObjInfo::getSectionIDs( TypeSet<SectionID>& secids ) const
{
    mGetReader
    if ( !reader_ )
	return false;

    for ( int idx=0; idx<reader_->nrSections(); idx++ )
	secids += reader_->sectionID(idx);
    return true;
}


bool IOObjInfo::getSectionNames( BufferStringSet& secnames ) const
{
    if ( !reader_ )
	return false;

    for ( int idx=0; idx<reader_->nrSections(); idx++ )
	secnames.add( reader_->sectionName(idx) );
    return true;
}


bool IOObjInfo::getAttribNames( BufferStringSet& attrnames ) const
{
    mGetReader
    if ( !reader_ )
	return false;

    for ( int idx=0; idx<reader_->nrAuxVals(); idx++ )
	attrnames.add( reader_->auxDataName(idx) );
    return true;
}


Interval<float> IOObjInfo::getZRange() const
{
    mGetReader
    if ( !reader_ )
	return Interval<float>(mUdf(float),mUdf(float));
    return reader_->zInterval();
}


StepInterval<int> IOObjInfo::getInlRange() const
{
    mGetReader
    if ( !reader_ )
	return Interval<int>(mUdf(float),mUdf(float));
    return reader_->rowInterval();
}


StepInterval<int> IOObjInfo::getCrlRange() const
{
    mGetReader
    if ( !reader_ )
	return Interval<int>(mUdf(float),mUdf(float));
    return reader_->colInterval();
}


bool IOObjInfo::getBodyRange( CubeSampling& cs ) const
{
    if ( type_ != IOObjInfo::Body )
	return false;

    mGetReader
    if ( !reader_ )
	return false;
    return true;
}


bool IOObjInfo::getLineSets( BufferStringSet& linesets ) const
{
    mGetReader
    if ( !reader_ )
	return false;

    for ( int idx=0; idx<reader_->nrLines(); idx++ )
	linesets.add( reader_->lineSet(idx) );
    return true;
}


bool IOObjInfo::getLineNames( BufferStringSet& linenames ) const
{
    mGetReader
    if ( !reader_ )
	return false;

    for ( int idx=0; idx<reader_->nrLines(); idx++ )
	linenames.add( reader_->lineName(idx) );
    return true;
}


int IOObjInfo::nrSticks() const
{
    mGetReader
    if ( !reader_ )
	return false;

    Interval<float> rowrange;
    reader_->pars()->get( "Row range", rowrange );
    return rowrange.width();
}


bool IOObjInfo::getTrcRanges( TypeSet< StepInterval<int> >& trcranges ) const
{
    mGetReader
    if ( !reader_ )
	return false;

    for ( int idx=0; idx<reader_->nrLines(); idx++ )
	trcranges.add( reader_->lineTrcRanges(idx) );
    return true;
}
