/*****************************************************************************
 * Project: RooFit                                                           *
 * Package: RooFitModels                                                     *
 * @(#)root/roofit:$Id$
 * Authors:                                                                  *
 *   Kyle Cranmer                                                            *
 *                                                                           *
 *****************************************************************************/

/** \class RooBernstein
    \ingroup Roofit

Bernstein basis polynomials are positive-definite in the range [0,1].
In this implementation, we extend [0,1] to be the range of the parameter.
There are n+1 Bernstein basis polynomials of degree n:
\f[
 B_{i,n}(x) = \begin{pmatrix}n \\\ i \end{pmatrix} x^i \cdot (1-x)^{n-i}
\f]
Thus, by providing n coefficients that are positive-definite, there
is a natural way to have well-behaved polynomial PDFs. For any n, the n+1 polynomials
'form a partition of unity', i.e., they sum to one for all values of x.
They can be used as a basis to span the space of polynomials with degree n or less:
\f[
 PDF(x, c_0, ..., c_n) = \mathcal{N} \cdot \sum_{i=0}^{n} c_i \cdot B_{i,n}(x).
\f]
By giving n+1 coefficients in the constructor, this class constructs the n+1
polynomials of degree n, and sums them to form an element of the space of polynomials
of degree n. \f$ \mathcal{N} \f$ is a normalisation constant that takes care of the
cases where the \f$ c_i \f$ are not all equal to one.

See also
http://www.idav.ucdavis.edu/education/CAGDNotes/Bernstein-Polynomials.pdf
**/

#include "RooBernstein.h"
#include "RooAbsReal.h"
#include "RooRealVar.h"
#include "RooArgList.h"
#include "RooBatchCompute.h"

#include "TMath.h"

#include <cmath>
using namespace std;

ClassImp(RooBernstein);

/// Constructor
////////////////////////////////////////////////////////////////////////////////

RooBernstein::RooBernstein(const char* name, const char* title,
                           RooAbsRealLValue& x, const RooArgList& coefList):
  RooAbsPdf(name, title),
  _x("x", "Dependent", this, x),
  _coefList("coefficients","List of coefficients",this)
{
  for (auto *coef : coefList) {
    if (!dynamic_cast<RooAbsReal*>(coef)) {
      cout << "RooBernstein::ctor(" << GetName() << ") ERROR: coefficient " << coef->GetName()
      << " is not of type RooAbsReal" << endl ;
      R__ASSERT(0) ;
    }
    _coefList.add(*coef) ;
  }
}

////////////////////////////////////////////////////////////////////////////////

RooBernstein::RooBernstein(const RooBernstein &other, const char *name)
   : RooAbsPdf(other, name), _x("x", this, other._x), _coefList("coefList", this, other._coefList)
{
}

////////////////////////////////////////////////////////////////////////////////

/// Force use of a given normalisation range.
/// Needed for functions or PDFs (e.g. RooAddPdf) whose shape depends on the choice of normalisation.
void RooBernstein::selectNormalizationRange(const char* rangeName, bool force)
{
  if (rangeName && (force || !_refRangeName.empty())) {
     _refRangeName = rangeName;
  }
}

////////////////////////////////////////////////////////////////////////////////

double RooBernstein::evaluate() const
{
  double xmax,xmin;
  std::tie(xmin, xmax) = _x->getRange(_refRangeName.empty() ? nullptr : _refRangeName.c_str());
  double x = (_x - xmin) / (xmax - xmin); // rescale to [0,1]
  Int_t degree = _coefList.getSize() - 1; // n+1 polys of degree n

  if(degree == 0) {

    return static_cast<RooAbsReal &>(_coefList[0]).getVal();

  } else if(degree == 1) {

    double a0 = static_cast<RooAbsReal &>(_coefList[0]).getVal(); // c0
    double a1 = static_cast<RooAbsReal &>(_coefList[1]).getVal() - a0; // c1 - c0
    return a1 * x + a0;

  } else if(degree == 2) {

    double a0 = static_cast<RooAbsReal &>(_coefList[0]).getVal();  // c0
    double a1 = 2 * (static_cast<RooAbsReal &>(_coefList[1]).getVal() - a0); // 2 * (c1 - c0)
    double a2 = static_cast<RooAbsReal &>(_coefList[2]).getVal() - a1 - a0;  // c0 - 2 * c1 + c2
    return (a2 * x + a1) * x + a0;

  } else if(degree > 2) {

    double t = x;
    double s = 1 - x;

    double result = static_cast<RooAbsReal &>(_coefList[0]).getVal() * s;
    for(Int_t i = 1; i < degree; i++) {
      result = (result + t * TMath::Binomial(degree, i) 
                * static_cast<RooAbsReal &>(_coefList[i]).getVal()) * s;
      t *= x;
    }
    result += t * static_cast<RooAbsReal &>(_coefList[degree]).getVal();

    return result;
  }

  // in case list of arguments passed is empty
  return TMath::SignalingNaN();
}

////////////////////////////////////////////////////////////////////////////////
/// Compute multiple values of Bernstein distribution.
void RooBernstein::computeBatch(cudaStream_t* stream, double* output, size_t nEvents, RooFit::Detail::DataMap const& dataMap) const
{
  const int nCoef = _coefList.size();
  std::vector<double> extraArgs(nCoef+2);
  for (int i=0; i<nCoef; i++)
    extraArgs[i] = static_cast<RooAbsReal&>(_coefList[i]).getVal();
  extraArgs[nCoef] = _x.min();
  extraArgs[nCoef+1] = _x.max();

  auto dispatch = stream ? RooBatchCompute::dispatchCUDA : RooBatchCompute::dispatchCPU;
  dispatch->compute(stream, RooBatchCompute::Bernstein, output, nEvents, {dataMap.at(_x)}, extraArgs);
}

////////////////////////////////////////////////////////////////////////////////

Int_t RooBernstein::getAnalyticalIntegral(RooArgSet& allVars, RooArgSet& analVars, const char* /*rangeName*/) const
{

  if (matchArgs(allVars, analVars, _x)) return 1;
  return 0;
}

////////////////////////////////////////////////////////////////////////////////

double RooBernstein::analyticalIntegral(Int_t code, const char* rangeName) const
{
  R__ASSERT(code==1) ;

  double xmax,xmin;
  std::tie(xmin, xmax) = _x->getRange(_refRangeName.empty() ? nullptr : _refRangeName.c_str());
  const double xlo = (_x.min(rangeName) - xmin) / (xmax - xmin);
  const double xhi = (_x.max(rangeName) - xmin) / (xmax - xmin);

  Int_t degree= _coefList.getSize()-1; // n+1 polys of degree n
  double norm(0) ;

  double temp=0;
  for (int i=0; i<=degree; ++i){
    // for each of the i Bernstein basis polynomials
    // represent it in the 'power basis' (the naive polynomial basis)
    // where the integral is straight forward.
    temp = 0;
    for (int j=i; j<=degree; ++j){ // power basis≈ß
      temp += pow(-1.,j-i) * TMath::Binomial(degree, j) * TMath::Binomial(j,i) * (TMath::Power(xhi,j+1) - TMath::Power(xlo,j+1)) / (j+1);
    }
    temp *= static_cast<RooAbsReal &>(_coefList[i]).getVal(); // include coeff
    norm += temp; // add this basis's contribution to total
  }

  norm *= xmax-xmin;
  return norm;
}
