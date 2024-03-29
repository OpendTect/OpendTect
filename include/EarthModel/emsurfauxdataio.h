#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "emposid.h"
#include "executor.h"
#include "od_iosfwd.h"

class TrcKeySampling;
template <class T> class DataInterpreter;


namespace EM
{

class Horizon3D;

/*!
\brief Writes auxdata to file.
*/

mExpClass(EarthModel) dgbSurfDataWriter : public Executor
{ mODTextTranslationClass(dgbSurfDataWriter);
public:
				dgbSurfDataWriter(const EM::Horizon3D& surf,
						  int dataidx,
						  const TrcKeySampling* sel,
						  bool binary,
						  const char* filename);
			/*!<\param surf		The surface with the values
			    \param dataidx	The index of the data to be
						written
			    \param sel		A selection of which data
						that should be written.
						Can be null, i.e. no selection
			    \param binary	Specify whether the data should
						be written in binary format
			    \param filename	File name to write to
			*/

				~dgbSurfDataWriter();

    int				nextStep() override;
    od_int64			nrDone() const override;
    od_int64			totalNr() const override;
    uiString			uiMessage() const override;
    uiString			uiNrDoneText() const override;

    static const char*		sKeyAttrName();
    static const char*		sKeyIntDataChar();
    static const char*		sKeyInt64DataChar();
    static const char*		sKeyFloatDataChar();
    static const char*		sKeyFileType();
    static const char*		sKeyShift();

    static BufferString		createHovName(const char* base,int idx);
    static bool			writeDummyHeader(const char* fnm,
						 const char* attrnm);

protected:

    bool			writeInt(int);
    bool			writeInt64(od_int64);
    bool			writeFloat(float);
    int				dataidx_;
    const EM::Horizon3D&	surf_;
    const TrcKeySampling*	sel_;

    TypeSet<EM::SubID>		subids_;
    TypeSet<float>		values_;
    int				sectionindex_	= 0;

    int				chunksize_	= 100;
    int				nrdone_		= 0;
    int				totalnr_	= 0;
    uiString		        errmsg_;

    od_ostream*			stream_		= nullptr;
    bool			binary_;
    BufferString		filename_;

    bool			writeHeader();
};


/*!
\brief Reads auxdata from file.
*/

mExpClass(EarthModel) dgbSurfDataReader : public Executor
{ mODTextTranslationClass(dgbSurfDataReader);
public:
				dgbSurfDataReader(const char* filename);
				~dgbSurfDataReader();

    const char*			dataName() const;
    float			shift() const;
    const char*			dataInfo() const;

    void			setSurface(EM::Horizon3D&);

    int				nextStep() override;
    od_int64			nrDone() const override;
    od_int64			totalNr() const override;
    uiString			uiMessage() const override;
    uiString			uiNrDoneText() const override;

    static uiString		sHorizonData();

protected:

    bool			readInt(int&);
    bool			readInt64(od_int64&);
    bool			readFloat(float&);

    BufferString		dataname_;
    BufferString		datainfo_;
    int				dataidx_		= -1;
    float			shift_			= 0;
    EM::Horizon3D*		surf_			= nullptr;

    int				sectionindex_		= 0;
    int				nrsections_		= 0;
    EM::SectionID		currentsection_;
    int				valsleftonsection_	= 0;

    int				chunksize_		= 100;
    int				nrdone_			= 0;
    int				totalnr_		= 0;
    uiString		        errmsg_;

    od_istream*			stream_			= nullptr;
    BufferString		filename_;

    DataInterpreter<int>*	intinterpreter_		= nullptr;
    DataInterpreter<od_int64>*	int64interpreter_	= nullptr;
    DataInterpreter<float>*	floatinterpreter_	= nullptr;
    bool			error_			= true;

    bool			readHeader();
};

};
