#ifndef viscolortabindexer_h
#define viscolortabindexer_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		January 2007
 RCS:		$Id: viscolortabindexer.h,v 1.3 2007-10-30 16:53:35 cvskris Exp $
________________________________________________________________________


-*/

#include "task.h"

namespace Threads { class Mutex; }

template <class T> class ValueSeries;

namespace visBase
{

class VisColorTab;

/*!\brief
Bins float data according a colortable's table-colors. Number of bins is
dependent on number of entries in the colortable's table. Undef-values are
assigned nrStep() as index, and are not present in the histogram.

*/


class ColorTabIndexer : public ParallelTask
{
public:
			ColorTabIndexer( const ValueSeries<float>& inp,
				    unsigned char* outp, int sz,
				    const VisColorTab* );
			ColorTabIndexer( const float* inp,
				    unsigned char* outp, int sz,
				    const VisColorTab* );

			~ColorTabIndexer();

    const unsigned int*	getHistogram() const;
    int			nrHistogramSteps() const;

protected:
    bool			doWork(int start,int stop,int threadid);
    int				totalNr() const;

    unsigned char*		indexcache_;
    const ValueSeries<float>*	datacache_;
    const float*		datacacheptr_;
    const visBase::VisColorTab*	colortab_;
    unsigned int*		globalhistogram_;
    int				nrhistogramsteps_;
    Threads::Mutex&		histogrammutex_;
    const int			sz_;
};


}; // Namespace


#endif
