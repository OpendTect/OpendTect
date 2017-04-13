#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Apr 2017
________________________________________________________________________

*/

#include "seisblocksdata.h"
#include "seistrc.h"
#include "filepath.h"
#include "uistring.h"

class Task;
class Scaler;
class SeisTrc;


namespace Seis
{
namespace Blocks
{

class Data;

/*!\brief Writes provided data into Block Storage */

mExpClass(Seis) Writer : public DataStorage
{ mODTextTranslationClass(Seis::Blocks::Writer);
public:

			Writer(const SurvGeom* sg=0);
			~Writer();

    void		setBasePath(const File::Path&);
    void		setFileNameBase(const char*);
    void		setFPRep(OD::FPDataRepType);
    void		setScaler(const Scaler*);
    void		setComponent(int);

    uiRetVal		add(const SeisTrc&);
    Task*		finisher();
			//!< May return null; destructor will run it eventually

protected:

    typedef std::pair<IdxType,float>	ZEvalPos;
    typedef TypeSet<ZEvalPos>		ZEvalPosSet;

    struct ZEvalInfo
    {
			ZEvalInfo( IdxType gidx )   : globidx_(gidx)	{}
	const IdxType	globidx_;
	ZEvalPosSet	evalpositions_;
    };

    File::Path		basepath_;
    BufferString	filenamebase_;
    OD::FPDataRepType	specfprep_;
    OD::FPDataRepType	usefprep_;
    int			component_;
    Scaler*		scaler_;
    bool		needreset_;

    void		resetZ(const Interval<float>&);
    Interval<IdxType>	globzidxrg_;
    ObjectSet<ZEvalInfo> zevalinfos_;

    bool		add2Block(const GlobIdx&,const SeisTrc&,
				  const ZEvalPosSet&,uiRetVal&);
    Data&		getData(const GlobIdx&);
    bool		isComplete(const GlobIdx&) const;
    void		writeBlock(Data&,uiRetVal&);

};


} // namespace Blocks

} // namespace Seis
