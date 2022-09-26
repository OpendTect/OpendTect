/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "dpsdispmgr.h"

DataPointSetDisplayProp::DataPointSetDisplayProp( const ColTab::Sequence& cs,
						  const ColTab::MapperSetup& cm,
						  int id )
    : coltab_(cs), coltabmappersu_(cm)
    , showsel_(false), dpscolid_(id)
{}


DataPointSetDisplayProp::DataPointSetDisplayProp( const BufferStringSet& nms,
			 const TypeSet<OD::Color>& cols )
    : selgrpnms_(nms), selgrpcols_(cols)
    , showsel_(true), dpscolid_(-1)
{}


DataPointSetDisplayProp::~DataPointSetDisplayProp()
{}


DataPointSetDisplayProp* DataPointSetDisplayProp::clone() const
{
    if ( showsel_ )
	return new DataPointSetDisplayProp( selgrpnms_, selgrpcols_ );
    else
	return new DataPointSetDisplayProp(coltab_,coltabmappersu_, dpscolid_ );
}


OD::Color DataPointSetDisplayProp::getColor( float val ) const
{
    if ( showsel_ )
	return selgrpcols_.validIdx(mNINT32(val)) ? selgrpcols_[mNINT32(val)]
						  : OD::Color::NoColor();

    if ( mIsUdf(val) )
	return coltab_.undefColor();

    ColTab::Mapper mapper;
    mapper.setup_ = coltabmappersu_;
    const float pos = mapper.position( val );
    OD::Color col = coltab_.color( pos );
    col.setTransparency( (unsigned char) mNINT32(coltab_.transparencyAt(pos)) );
    return col;
}


DataPointSetDisplayMgr::DataPointSetDisplayMgr()
{}


DataPointSetDisplayMgr::~DataPointSetDisplayMgr()
{}
