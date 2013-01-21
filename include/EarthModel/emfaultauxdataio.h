#ifndef emfaultauxdataio_h
#define emfaultauxdataio_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		8-3-2012
 RCS:		$Id$
________________________________________________________________________


-*/

#include "emposid.h"
#include "executor.h"
class HorSampling;
template <class T> class DataInterpreter;


namespace EM
{

class Fault3D;

/*!
\brief Writes auxdata to file.
*/

mExpClass(EarthModel) dgbFaultDataWriter : public Executor
{
public:
    				dgbFaultDataWriter(const EM::Fault3D& surf,
						  int dataidx,
						  const HorSampling*,
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
			*/

				~dgbFaultDataWriter();

    virtual int			nextStep();
    virtual od_int64		nrDone() const;
    virtual od_int64		totalNr() const;
    virtual const char*		message() const;

    static const char*		sKeyAttrName();
    static const char*		sKeyIntDataChar();
    static const char*		sKeyInt64DataChar();
    static const char*		sKeyFloatDataChar();
    static const char*		sKeyFileType();

    static BufferString		createHovName(const char* base,int idx);
    static bool			writeDummyHeader(const char* fnm,
						 const char* attrnm);

protected:

    bool			writeInt(int);
    bool			writeInt64(od_int64);
    bool			writeFloat(float);
    int				dataidx_;
    const EM::Fault3D&		surf_;
    const HorSampling*		sel_;
  
    TypeSet<EM::SubID>		subids_;
    TypeSet<float>		values_;
    int				sectionindex_;

    int				chunksize_;
    int				nrdone_;
    int				totalnr_;
    BufferString		errmsg_;

    std::ostream*		stream_;
    bool			binary_;
    BufferString		filename_;
};


/*!
\brief Reads auxdata from file.
*/

mExpClass(EarthModel) dgbFaultDataReader : public Executor
{
public:
    				dgbFaultDataReader(const char* filename);
				~dgbFaultDataReader();

    const char*			dataName() const;
    const char*			dataInfo() const;

    void			setSurface(EM::Fault3D&);

    virtual int			nextStep();
    virtual od_int64		nrDone() const;
    virtual od_int64		totalNr() const;
    virtual const char*		message() const;

protected:

    bool			readInt(int&);
    bool			readInt64(od_int64&);
    bool			readFloat(float&);
    BufferString		dataname_;
    BufferString		datainfo_;
    int				dataidx_;
    EM::Fault3D*		surf_;
    const HorSampling*		sel_;
  
    int				sectionindex_;
    int				nrsections_;
    EM::SectionID		currentsection_;
    int				valsleftonsection_;

    int				chunksize_;
    int				nrdone_;
    int				totalnr_;
    BufferString		errmsg_;

    std::istream*		stream_;
    DataInterpreter<int>*	intinterpreter_;
    DataInterpreter<od_int64>*	int64interpreter_;
    DataInterpreter<float>*	floatinterpreter_;
    bool			error_;
};

};

#endif
