#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          10-12-1999
________________________________________________________________________


-*/

#include "algomod.h"
#include "task.h"
#include "sets.h"
#include "odcomplex.h"

class ArrayNDInfo;
template <class T> class ArrayND;


/*!
\brief Lets any 1D orthogonal transform (GenericTransformND::GenericTransform1D)
be extended to ND. Most transform fftw can be implemented as a subclass of
GenericTransformND.
*/

mExpClass(Algo) GenericTransformND : public SequentialTask
{
public:
			GenericTransformND();
    bool		setInputInfo(const ArrayNDInfo&);
    const ArrayNDInfo&	getInputInfo() const;

    void		setInput(const float*);
    void		setInput(const float_complex*);
    void		setOutput(float*);
    void		setOutput(float_complex*);

    void		setSampling(int);
    void		setScope(int nr,int batchsampling);
			/*!<\param nr number of signals
			    \param batchsampling number of samples between
			    	   signal starts. */
    void		setScope(int nr,const int* batchstarts);
			/*!<\param nr number of signals
			    \param batchstarts pointer to array with the indices
				   of the signal starts.*/

    bool		setDir(bool forward);
    bool		getDir() const	{ return forward_; }
    			//true for forward
    bool		run(bool parallel);
    			//SequentialTask::execute can be used as well

			~GenericTransformND();
protected:
    virtual bool		setup();			
    int				nextStep();

    class			Transform1D;
    virtual Transform1D*	createTransform() const		= 0;

    ObjectSet<Transform1D>	transforms_;
    ObjectSet<int>		transforms1dstarts_;
    TypeSet<int>		nr1dtransforms_;

    bool			forward_;
    int				sampling_;
    int				batchsampling_;
    const int*			batchstarts_;
    int				nr_;

    ArrayNDInfo*		info_;
    int				curdim_;
    bool			parallel_;

    const float_complex*	cinput_;
    const float*		rinput_;
    float_complex*		coutput_;
    float*			routput_;

    mExpClass(Algo) Transform1D
    {
    public:
	virtual		~Transform1D()				{}
	void		setInputData(const float_complex*);
	void		setInputData(const float*);
	void		setOutputData(float_complex*);
	void		setOutputData(float*);

	void		setSize(int);
	void		setDir(bool forward);
	void		setSampling(int);
			//!<\param samplespacing sampling within a signal.
	void		setScope(int nr,int batchsampling);
			/*!<\param nr number of signals
			    \param batchsampling number of samples between
			    	   signal starts. */
	void		setScope(int nr,const int* batchstarts);
			/*!<\param nr number of signals
			    \param batchstarts pointer to array with the indices
				   of the signal starts.*/

	virtual bool	init() { return true; }
	virtual bool	run(bool parallel)				= 0;

    protected:
				
    				Transform1D();

	int			sz_;
	bool			forward_;
	int			sampling_;
	int			nr_;
	int			batchsampling_;
	const int*		batchstarts_;

	const float_complex*	cinput_;
	const float*		rinput_;
	float_complex*		coutput_;
	float*			routput_;
    };
};

