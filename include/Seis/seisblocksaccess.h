#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Apr 2017 / Dec 2019
________________________________________________________________________

*/

#include "seisblocks.h"
#include "filepath.h"
#include "threadlock.h"
#include "zdomain.h"

class DataBuffer;
class LinScaler;
class SeisTrc;
namespace Pos { class IdxPairDataSet; }
namespace PosInfo { class CubeData; }


namespace Seis
{

namespace Blocks
{


/*!\brief Base class for Reader and Writer.

  The format is designed with these principles in mind:
  * Bricking helps keep performance similar and reasonably OK in all directions
  * The total geometry setup is stored in a human-readbale summary file, which
    will make the data usable accross surveys
  * When writing, no sorting is required, although some sorting will help keep
    memory consumption down

*/

mExpClass(Seis) Access
{
public:

    virtual		~Access();

    virtual const HGeom& hGeom() const		{ return hgeom_; }
    const ZGeom&	zGeom() const		{ return zgeom_; }

    const Dimensions&	dimensions() const	{ return dims_; }
    version_type	version() const		{ return version_; }
    const char*		cubeName() const	{ return cubename_; }
    const BufferStringSet& componentNames() const { return compnms_; }
    OD::DataRepType	dataRep() const		{ return datarep_; }
    const LinScaler*	scaler() const		{ return scaler_; }
    int			nrAuxInfo() const	{ return auxiops_.size(); }
    const IOPar&	getAuxInfo( int i ) const { return *auxiops_[i]; }
    DataType		dataType() const	{ return datatype_; }
    const ZDomain::Def&	zDomain() const		{ return zdomain_; }

    const File::Path&	basePath() const	{ return basepath_; }
    BufferString	infoFileName() const;
    const char*		fileExtension() const;
    BufferString	dataFileName() const;
    BufferString	overviewFileName() const;
    static BufferString	infoFileNameFor(const char*);
    static BufferString	dataFileNameFor(const char*,bool usehdf);

    static const char*	sDataFileExt(bool forhdf5);
    static const char*	sKeyOvvwFileExt() { return "ovvw"; }
    static const char*	sKeyFileType()	  { return "Column Cube"; }
    static const char*	sKeySectionPre()  { return "Section-"; }
    static const char*	sKeyGenSection()  { return "Section-General"; }
    static const char*	sKeyOffSection()  { return "Section-Offsets"; }
    static const char*	sKeyFileIDSection()  { return "Section-FileIDs"; }
    static const char*	sKeyPosSection()  { return "Section-Positions"; }
    static const char*	sKeySurveyName()  { return "Name.Survey"; }
    static const char*	sKeyCubeName()	  { return "Name.Cube"; }
    static const char*	sKeyFmtVersion()  { return "Blocks.Version"; }
    static const char*	sKeyDimensions()  { return "Blocks.Max Dimensions"; }
    static const char*	sKeyGlobInlRg()	  { return "Blocks.Inl ID Range"; }
    static const char*	sKeyGlobCrlRg()	  { return "Blocks.Crl ID Range"; }
    static const char*	sKeyGlobZRg()	  { return "Blocks.Z ID Range"; }
    static const char*	sKeyComponents()  { return "Components"; }
    static const char*	sKeyDataType()    { return "Data Type"; }
    static const char*	sKeyDepthInFeet() { return "Depth in Feet"; }

    static bool		hdf5Active();

protected:

			Access();

    mutable Threads::Lock accesslock_;
    Pos::IdxPairDataSet& columns_;

    File::Path		basepath_;
    HGeom&		hgeom_;
    ZGeom		zgeom_;
    ZDomain::Def	zdomain_;
    Dimensions		dims_;
    version_type	version_;
    BufferString	cubename_;
    BufferStringSet	compnms_;
    LinScaler*		scaler_;
    OD::DataRepType	datarep_;
    IOPar		gensectioniop_;
    IOPar		fileidsectioniop_;
    ObjectSet<IOPar>	auxiops_;
    PosInfo::CubeData&	cubedata_;
    DataType		datatype_;
    mutable bool	needreset_;
    mutable bool	usehdf_;

    Column*		findColumn(const HGlobIdx&) const;
    void		addColumn(Column*) const;
    void		clearColumns();

};


} // namespace Blocks

} // namespace Seis
