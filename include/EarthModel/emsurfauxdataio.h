#ifndef emsurfauxdataio_h
#define emsurfauxdataio_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emsurfauxdataio.h,v 1.1 2003-06-17 13:50:25 kristofer Exp $
________________________________________________________________________


-*/

#include "emposid.h"
#include "executor.h"


class BinIDSampler;
template <class T> class DataInterpreter;

namespace EM
{

class Surface;

/*!
Writes auxdata to file
*/

class dgbSurfDataWriter : public Executor
{
public:
    			dgbSurfDataWriter( const EM::Surface& surf, int dataidx,
					const BinIDSampler* sel, bool binary,
			       		const char* filename );
			/*!<\param surf		The surface with the values
			    \param dataidx	The index of the data to be
			    			written
			    \param sel		A selection of whic data
			    			that should be written.
						Can be null, i.e. no selection
			    \param binary	Specified wether the data shoul
			    			be written in binary format
			*/

			~dgbSurfDataWriter();

    int			nextStep();
    int			nrDone() const;
    int			totalNr() const;


    static const char*	attrnmstr;
    static const char*	binarystr;

    static BufferString	createHovName(const char* base, int idx );


protected:
    int				dataidx;
    const EM::Surface&		surf;
    const BinIDSampler*		sel;
  
    TypeSet<EM::SubID>		subids;
    TypeSet<float>		values;
    int				patchindex;

    int				chunksize;
    int				nrdone;
    int				totalnr;

    ofstream*			stream;
    DataInterpreter<int>*	subidinterpreter;
    DataInterpreter<float>*	datainterpreter;
};

/*!
Writes auxdata to file
*/


class dgbSurfDataReader : public Executor
{
public:
    			dgbSurfDataReader( const char* filename );
			~dgbSurfDataReader();

    const char*		dataName() const;

    void		setSurface( EM::Surface& surf );

    int			nextStep();
    int			nrDone() const;
    int			totalNr() const;

protected:
    BufferString		dataname;
    int				dataidx;
    const EM::Surface*		surf;
    const BinIDSampler*		sel;
  
    TypeSet<EM::SubID>		subids;
    TypeSet<float>		values;
    int				patchindex;

    int				chunksize;
    int				nrdone;
    int				totalnr;

    ifstream*			stream;
    DataInterpreter<int>*	subidinterpreter;
    DataInterpreter<float>*	datainterpreter;
};

};

#endif
