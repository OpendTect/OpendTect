#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
			~GenericTransformND();

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

protected:
				GenericTransformND();

    virtual bool		setup();
    void			setNeedSetup(bool yn=true);

    bool			doPrepare(od_ostream* =nullptr) override;
    int				nextStep() override;
    bool			doFinish(bool,od_ostream* =nullptr) override;

    od_int64			nrDone() const override;
    od_int64			totalNr() const override;

    class			Transform1D;
    virtual Transform1D*	createTransform() const		= 0;

    ObjectSet<Transform1D>	transforms_;
    ObjectSet<int>		transforms1dstarts_;
    TypeSet<int>		nr1dtransforms_;

    bool			forward_	= true;
    int				sampling_	= 1;
    int				batchsampling_	= 1;
    const int*			batchstarts_	= nullptr;
    int				nr_		= 1;

    ArrayNDInfo*		info_		= nullptr;
    bool			parallel_	= true;

    const float_complex*	cinput_		= nullptr;
    const float*		rinput_		= nullptr;
    float_complex*		coutput_	= nullptr;
    float*			routput_	= nullptr;

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
	void		setSampling(int samplespacing);
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

	int			sz_		= -1;
	bool			forward_	= true;
	int			sampling_	= 1;
	int			nr_		= 1;
	int			batchsampling_	= 1;
	const int*		batchstarts_	= nullptr;

	const float_complex*	cinput_		= nullptr;
	const float*		rinput_		= nullptr;
	float_complex*		coutput_	= nullptr;
	float*			routput_	= nullptr;

    };

private:
    int				curdim_		= -1;
    bool			needsetup_	= true;
};
