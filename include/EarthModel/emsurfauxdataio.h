#ifndef emsurfauxdataio_h
#define emsurfauxdataio_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emsurfauxdataio.h,v 1.10 2004-07-23 12:54:54 kristofer Exp $
________________________________________________________________________


-*/

#include "emposid.h"
#include "executor.h"


class BinIDSampler;
template <class T> class DataInterpreter;

namespace EM
{

class Surface;

/*!\brief  Writes auxdata to file */

class dgbSurfDataWriter : public Executor
{
public:
    				dgbSurfDataWriter(const EM::Surface& surf,
						  int dataidx,
						  const BinIDSampler* sel,
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

				~dgbSurfDataWriter();

    int				nextStep();
    int				nrDone() const;
    int				totalNr() const;


    static const char*		attrnmstr;
    static const char*		infostr;
    static const char*		intdatacharstr;
    static const char*		longlongdatacharstr;
    static const char*		floatdatacharstr;
    static const char*		filetypestr;
    static const char*		shiftstr;

    static BufferString		createHovName(const char* base,int idx);


protected:
    bool			writeInt(int);
    bool			writeLongLong(long long);
    bool			writeFloat(float);
    int				dataidx;
    const EM::Surface&		surf;
    const BinIDSampler*		sel;
  
    TypeSet<EM::SubID>		subids;
    TypeSet<float>		values;
    int				sectionindex;

    int				chunksize;
    int				nrdone;
    int				totalnr;

    std::ostream*		stream;
    bool			binary;
};


/*!\brief Reads auxdata from file */

class dgbSurfDataReader : public Executor
{
public:
    				dgbSurfDataReader(const char* filename);
				~dgbSurfDataReader();

    const char*			dataName() const;
    const char*			dataInfo() const;

    void			setSurface(EM::Surface& surf);

    int				nextStep();
    int				nrDone() const;
    int				totalNr() const;

protected:
    bool			readInt(int&);
    bool			readLongLong(long long&);
    bool			readFloat(float&);
    BufferString		dataname;
    BufferString		datainfo;
    int				dataidx;
    float			shift;
    EM::Surface*		surf;
    const BinIDSampler*		sel;
  
    int				sectionindex;
    int				nrsections;
    EM::SectionID		currentsection;
    int				valsleftonsection;

    int				chunksize;
    int				nrdone;
    int				totalnr;

    std::istream*		stream;
    DataInterpreter<int>*	intinterpreter;
    DataInterpreter<long long>*	longlonginterpreter;
    DataInterpreter<float>*	floatinterpreter;
    bool			error;
};

};

#endif
