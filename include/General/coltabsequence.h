#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		23-3-2000
________________________________________________________________________

-*/


#include "coltab.h"
#include "enums.h"
#include "uistring.h"
#include "sharedobject.h"


namespace ColTab
{

class SequenceManager;


/*!\brief A series of color control points able to give an (interpolated) color
	  for every position [0,1].

  Standard color sequences ('Color tables') are read at program start,
  including the 'user defined' ones. Users can overrule or 'remove' the
  standard ones. Disabled sequences will not be shown in selectors.

  Sequences cannot be scaled, try the Mapper.

 */

mExpClass(General) Sequence : public SharedObject
{
public:

    typedef TypeSet<PosType>::size_type	size_type;
    typedef size_type			IdxType;
    typedef unsigned char		CompType;
    typedef std::pair<PosType,CompType>	TranspPtType;

    enum Status		{ System, Edited, Added };
			mDeclareEnumUtils(Status);

			Sequence();		//!< Empty
			Sequence(const char*);	//!< Find by name in SeqMGR
			mDeclInstanceCreatedNotifierAccess(Sequence);
			mDeclMonitorableAssignment(Sequence);

    Color		color(PosType) const; //!< 0 <= pos <= 1

    Status		status() const;
    inline bool		isSys() const		{ return status()==System; }
    mImplSimpleMonitoredGetSet(inline,disabled,setDisabled,bool,disabled_,
						    cStatusChange())
    uiString		statusDispStr() const;

    inline bool		isEmpty() const		{ return size() < 1; }
    size_type		size() const;
    PosType		position(IdxType) const;
    CompType		r(IdxType) const;
    CompType		g(IdxType) const;
    CompType		b(IdxType) const;

    size_type		transparencySize() const;
    TranspPtType	transparency(IdxType) const;
    CompType		transparencyAt(PosType) const;
    void		setTransparency(TranspPtType);
    void		changeTransparency(IdxType,TranspPtType);
    void		removeTransparencies();
    void		removeTransparencyAt(IdxType);
    bool		hasTransparency() const;

    mImplSimpleMonitoredGetSet(inline,nrSegments,setNrSegments,size_type,
				    nrsegments_,cSegmentationChange())
			/*!<nrsegments > 0 divide the ctab in equally wide
			    nrsegments == 0 no segmentation
			    nrsegments == -1 constant color between markers.*/
    bool		isSegmentized() const		{ return nrSegments(); }

    void		changeColor(IdxType,CompType,CompType,CompType);
    void		changePos(IdxType,PosType);
    IdxType		setColor(PosType, //!< Insert or change
				 CompType,CompType,CompType);
    void		removeColor(IdxType);
    void		removeAllColors();

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    mImplSimpleMonitoredGetSet(inline,undefColor,setUndefColor,
				Color,undefcolor_,cUdfColChange())
    mImplSimpleMonitoredGetSet(inline,markColor,setMarkColor,
				Color,markcolor_,cMarkColChange())

    static ChangeType	cStatusChange()		{ return 3; }
    static ChangeType	cColorChange()		{ return 4; }
    static ChangeType	cTransparencyChange()	{ return 5; }
    static ChangeType	cSegmentationChange()	{ return 6; }
    static ChangeType	cMarkColChange()	{ return 7; }
    static ChangeType	cUdfColChange()		{ return 8; }

    static const char*	sKeyValCol();
    static const char*	sKeyMarkColor();
    static const char*	sKeyUdfColor();
    static const char*	sKeyTransparency();
    static const char*	sKeyCtbl();
    static const char*	sKeyNrSegments();
    static const char*	sKeyDisabled();
    static const char*	sDefaultName(bool for_seismics=false);

protected:

			~Sequence();

    TypeSet<PosType>	x_;
    TypeSet<CompType>	r_;
    TypeSet<CompType>	g_;
    TypeSet<CompType>	b_;
    TypeSet<TranspPtType> tr_;

    size_type		nrsegments_	= 0;
    Color		undefcolor_	= Color::LightGrey();
    Color		markcolor_	= Color::DgbColor();
    bool		disabled_	= false;

    inline size_type	gtSize() const	{ return x_.size(); }

    PosType		snapToSegmentCenter(PosType) const;
    CompType		gtTransparencyAt(PosType) const;
    bool		chgColor(IdxType,CompType,CompType,CompType);
    bool		rmColor(IdxType);
    void		emitStatusChg() const;

    friend class	SequenceManager;

};


} // namespace ColTab
