#include "SimpleHelix.h"
#include "CLHEP/Matrix/Matrix.h"
#include "CLHEP/Matrix/Vector.h"

#include <iostream>
#include <iomanip>

#include <cmath>
#include <float.h>
#include <exception>

SimpleHelix::SimpleHelix( double d0, double phi0, double omega,
			  double z0, double tanLambda,
			  LCPoint3D referencePoint ) 
{
  init();

  _d0        = d0;
  _phi0      = phi0;
  _omega     = omega;
  _z0        = z0;
  _tanLambda = tanLambda;

  _reference = referencePoint;

  //  _Bz = Bz;

}

void SimpleHelix::init()
{
  _d0         = 0;
  _phi0       = 0;
  _omega      = 1;
  _z0         = 0;
  _tanLambda  = 0;
  //  _Bz         = 4;

  _reference.set(0.,0.,0.); 

  _helixStart = -DBL_MAX;
  _helixEnd   =  DBL_MAX;

}

double SimpleHelix::getCentreX() const
{
  return ( _reference.x() + ((1/_omega) - _d0) * sin(_phi0) );
}

double SimpleHelix::getCentreY() const
{
  return ( _reference.y() - ((1/_omega) - _d0) * cos(_phi0) );
}

double SimpleHelix::getWindingLength() const
{
  return 2*_pi*sqrt(1+_tanLambda*_tanLambda)/fabs(_omega);
}

LCPoint3D SimpleHelix::getPosition(double s, LCErrorMatrix* errors) const 
{
  LCPoint3D x;

  double xc = getCentreX();
  double yc = getCentreY();

  double varphi0 = _phi0 + ((_omega * _pi) / (2*fabs(_omega)));
  double w =  _omega / sqrt(1 + _tanLambda*_tanLambda);

  x.setX(xc + fabs(1/_omega) * cos( w*s - varphi0 ) );
  x.setY(yc + fabs(1/_omega) * sin( (-w*s) + varphi0) );
  x.setZ(_reference.z() + _z0 + s*_tanLambda/sqrt(1 + _tanLambda*_tanLambda) );

  return x;
}

LCVector3D SimpleHelix::getDirection(double s,  LCErrorMatrix* errors) const
{
  LCVector3D t;

  double varphi0 = _phi0 + ((_omega * _pi) / (2*fabs(_omega)));
  double w =  _omega / sqrt(1 + _tanLambda*_tanLambda);

  t.setX(-w*fabs(1/_omega) * sin( (w*s) - varphi0) );
  t.setY(-w*fabs(1/_omega) * cos( (-w*s) + varphi0) );
  t.setZ(_tanLambda/sqrt(1 + _tanLambda*_tanLambda) );

  return t.unit();
}

LCErrorMatrix SimpleHelix::getCovarianceMatrix( double s) const  
{
  return LCErrorMatrix( 6 , 0 ) ;
}

double SimpleHelix::getPathAt(const LCVector3D position ) const
{
  double sStart = (position.z() - _reference.z() - _z0)
    * sqrt(1 + _tanLambda*_tanLambda) / _tanLambda;

  if (sStart <= _helixStart) sStart = _helixStart;
  if (sStart >= _helixEnd) sStart = _helixEnd;

  double startRange = sStart - getWindingLength();
  double endRange   = sStart + getWindingLength();

  if (startRange <= _helixStart) startRange = _helixStart;
  if (endRange >= _helixEnd) endRange = _helixEnd;

  ///###  int nSteps = 100;
  int nSteps = 10;
  double stepWidth = (endRange - startRange)/((double)nSteps);

  double sOfMin = 0;
  double distMinSQ = DBL_MAX;
  double s = startRange;
  LCPoint3D x;
  for (int i = 0; i <= nSteps; i++)
    {
      x = getPosition(s);
      double distsq = (x-position).mag2() ;
// ###### vector
      if (distsq < distMinSQ)
	{
	  distMinSQ = distsq;
	  sOfMin = s;
	}
      s += stepWidth;
    }

  double minStepWidth = 0.000001;
  while (stepWidth > minStepWidth)
    {
      stepWidth /= 10;

      x = getPosition(sOfMin + stepWidth);
      double lp = (x-position).mag2();
      x = getPosition(sOfMin - stepWidth);
      double lm = (x-position).mag2();

      double upOrDown = 0;
      double lastDistance = 0;
      nSteps = 10 ;
      if (distMinSQ <= lp && distMinSQ <= lm)
	{
	  nSteps = 0 ;
	  lastDistance = distMinSQ;
	}
      else if (lm<lp)
	{
	  upOrDown = -1;
	  lastDistance = lm;
	}
      else if (lp<lm)
	{
	  upOrDown = 1;
	  lastDistance = lp;
	}
      else
	{
	  //	  cout << "Da laeuft was falsch!!! " 
	  //	       << " distMinSQ " << distMinSQ << " lm " << lm << " lp " << lp << endl;
	}

      double step = 0, l = 0, lastStep = sOfMin;
      for (int i = 1; i<=nSteps;i++)
	{
	  step = sOfMin + (upOrDown*stepWidth*(double)i);
	  x = getPosition(step);
	  l = (x-position).mag2();
	  if (l <= lastDistance) 
	    {
	      lastStep = step;
	      lastDistance = l;
	    }
	}
      sOfMin = lastStep ;
      distMinSQ = lastDistance;
    }
  return sOfMin;
}

double SimpleHelix::getIntersectionWithPlane( LCPlane3D p, 
					      bool& pointExists) const
{

  //FIXME: needs to be implemented
  pointExists = false ;
  return 0 ;
}

double SimpleHelix::getIntersectionWithCylinder(LCPoint3D center, 
					       LCVector3D axis, 
					       double radius,
					       bool & pointExists) const  
{

  //FIXME: needs to be implemented
  pointExists = false ;
  return 0 ;
}

double SimpleHelix::getStart() const
{
  return _helixStart;
}

double SimpleHelix::getEnd() const
{
  return _helixEnd;
}

bool SimpleHelix::setStart(double s)
{
  if (s <= _helixEnd)
    {
      _helixStart = s;
      return true;
    }

  return false;
}

bool SimpleHelix::setEnd(double s)
{
  if (s >= _helixStart)
    {
      _helixEnd = s;
      return true;
    }

  return false;
}

bool SimpleHelix::setStartEnd(double start, double end)
{
  if (start <= end)
    {
      _helixStart = start;
      _helixEnd = end;
      return true;
    }

  return false;
}