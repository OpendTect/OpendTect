#ifndef survinfo_H
#define survinfo_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		9-4-1996
 Contents:	Features for sets of data
 RCS:		$Id: survinfo.h,v 1.4 2001-02-27 11:22:21 bert Exp $
________________________________________________________________________

-*/
 
 
#include <uidobj.h>
#include <binidselimpl.h>
class ascostream;


/*!\brief Holds survey general information.

The surveyinfo is the primary source for ranges and steps. It also provides
the transformation between inline/xline and coordinates.

*/

class SurveyInfo : public UserIDObject
{

    friend class		EdSurvey;
    friend class		EdSurveyInfo;
    friend const SurveyInfo&	SI();

public:

			SurveyInfo(const SurveyInfo&);

    int			validTransform() const	{ return xtr.valid(ytr); }
    Coord		transform(const BinID&) const;
    BinID		transform(const Coord&) const;

    const BinIDRange&	range() const		{ return range_; }
    void		setRange(const BinIDRange&);
    int			rangeUsable() const
			{ return range_.start.inl && range_.stop.inl
			      && range_.start.crl && range_.stop.crl; }
    const BinID&	step() const		{ return step_; }
    void		setStep(const BinID&);

    void		snap(BinID&,const BinID& direction) const;
			// 0 : auto; -1 round downward, 1 round upward

    void		get3Pts(Coord c[3],BinID b[2],int& xline);
    const char*		set3Pts(Coord c[3],BinID b[2],int xline);
			//!< returns error message or null on success

private:

			SurveyInfo(const char*);
    int			write(const char*) const;

    UserIDString	dirname;
    struct BCTransform	{
					BCTransform()	{ a = b = c = 0; }
			    double	det(const BCTransform&) const;
			    int		valid(const BCTransform&) const;
			    double	a, b, c;
			};
    void		setTr(BCTransform&,const char*);
    void		putTr(const BCTransform&,ascostream&,const char*) const;

    BCTransform		xtr;
    BCTransform		ytr;
    BinIDRange		range_;
    BinID		step_;

    BinID		set3binids[3];
    Coord		set3coords[3];

    static SurveyInfo*	theinst_;

};


#endif
