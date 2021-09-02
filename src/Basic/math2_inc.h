/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2008
-*/


bool Math::IsNormalNumber( mTYPE x )
{
#ifdef __win__
    return finite( x );
#else
    return std::isfinite( x );
#endif
}


mTYPE Math::ACos( mTYPE c )
{
    return (mTYPE) c >= 1 ? 0 : (c <= -1 ? mTYPE(M_PI) : acos( c ));
}


mTYPE Math::ASin( mTYPE s )
{
    return (mTYPE) s >= 1 ? mTYPE(M_PI_2)
			  : (s <= -1 ? -mTYPE(M_PI_2) : asin( s ));
}


mTYPE Math::Log( mTYPE s )
{
    return (mTYPE) s <= 0 ? mUdf(mTYPE) : log( s );
}


mTYPE Math::Log10( mTYPE s )
{
    return (mTYPE) s <= 0 ? mUdf(mTYPE) : log10( s );
}


mTYPE Math::Sqrt( mTYPE s )
{
    //A bit silly bu the space before the parantesis
    //makes it avoid the sqrt test.
    return (mTYPE) s <= 0 ? 0 : sqrt ( s );
}

mTYPE Math::toDB( mTYPE s )
{
    return (mTYPE) s <= 0 ? mUdf(mTYPE) : 20*log10( s );
}


mTYPE Math::PowerOf( mTYPE x, mTYPE y )
{
    if ( mIsUdf(x) || mIsUdf(y) )
	return x;

    if ( x == 0 )
	return (mTYPE) (y ? 0 : 1);

    const bool isneg = x < 0 ? 1 : 0;
    if ( isneg ) x = -x;

    mTYPE ret = exp( y * log(x) );
    return isneg ? -ret : ret;
}


mTYPE Math::IntPowerOf( mTYPE x, int y )
{
    if ( mIsUdf(x) )
	return mUdf(mTYPE);

    if ( x == 0 )
	return y ? (mTYPE)0 : (mTYPE)1;

    if ( x > 1.5 || x < -1.5 )
    {
	if ( y > 150 ) return mUdf(mTYPE);
	if ( y < -150 ) return (mTYPE) 0;
	if ( x > 1.99 || x < -1.99 )
	{
	    if ( y > 100 ) return mUdf(mTYPE);
	    if ( y < -100 ) return (mTYPE) 0;
	}
    }
    else if ( x < 0.5 && x > -0.5 )
    {
	if ( y > 100 ) return (mTYPE) 0;
	if ( y < -100 ) return (mTYPE) 1;
    }

    mTYPE ret = 1;
    while ( y )
    {
	if ( y > 0 )
	    { ret *= x; y--; }
	else
	    { ret /= x; y++; }
    }
    return ret;
}


mTYPE Math::BesselI0( mTYPE x )
{
    if ( mIsUdf(x) )
	return mUdf(mTYPE);

    double xx = (mTYPE)x;
    xx *= xx;
    double s = 1.;
    double ds = 1.;
    double d = 0.;
    while ( ds > s * DBL_EPSILON )
    {
	d += 2.;
	ds *= xx / ( d*d );
	s += ds;
    }

    return (mTYPE)s;
}
