/***********************************************************************************
 * ngmCont2.cpp
 * 
 * Code to compute the errors on the 2-country neoclassical growth model with controls
 * 
 * 13feb2016
 * Philip Barrett, Chicago
 * 
 ***********************************************************************************/

#include "ngmCont2.hpp"
#include "sim.hpp"

// [[Rcpp::export]]
arma::rowvec integrand_ngm_cont_2( 
                arma::mat exog, arma::mat endog, arma::rowvec cont, 
                arma::rowvec exog_lead, List params, arma::mat coeffs, 
                arma::mat coeffs_cont, int n_exog, int n_endog, int n_cont, 
                int N, arma::rowvec upper, arma::rowvec lower, bool cheby=false ){

  double A = params["A"] ;
  double alpha = params["alpha"] ;
  double delta = params["delta"] ;
  double gamma = params["gamma"] ;
  
  rowvec cont_lead = endog_update( exog_lead, endog.row(0), coeffs_cont, n_exog, 
                                    n_endog, N, upper, lower, cheby ) ;
      // Create the next-period control.  Remember, the current-period state is
      // an end-of-period variable, so the control depends directly on its lag.

  rowvec integrand(2) ;
  integrand(0) = pow( cont_lead(0) / cont(0), - gamma ) * 
          ( 1 - delta + exp( exog_lead( 0 ) ) * alpha * A * pow( endog( 0, 0 ), alpha - 1 ) ) ;
  integrand(1) = pow( cont_lead(0) / cont(0), - gamma ) * 
          ( 1 - delta + exp( exog_lead( 1 ) ) * alpha * A * pow( endog( 0, 1 ), alpha - 1 ) ) ;
      // Two countries have same consumption but diefferent capital stocks and
      // technologies
  return integrand ;
}

// [[Rcpp::export]]
arma::rowvec euler_hat_ngm_cont_2( 
                  arma::mat exog, arma::mat endog, arma::rowvec cont,
                  arma::mat exog_innov_integ, 
                  List params, arma::mat coeffs, arma::mat coeffs_cont, 
                  int n_exog, int n_endog, int n_cont,
                  arma::rowvec rho, int n_integ, int N, arma::rowvec upper, 
                  arma::rowvec lower, bool cheby, arma::rowvec weights,
                  bool print_rhs=false ){
// Computes the single-period error on the neocassical growth model equilibrium 
// condition using a Monte Carlo approach.  NB: THE INNOVATIONS exog_innov_integ
// MUST ALREADY BE SCALED (IE. HAVE THE APPROPRIATE VARIANCE)
  
  double betta = params["betta"] ;
      // Extract beta
  mat exog_lead = zeros( n_integ, n_exog ) ;
      // Initalize the draws of the exogenous variables in the next period
  for( int i = 0 ; i < n_integ ; i++ ){
    exog_lead.row(i) = rho % exog.row(0) + exog_innov_integ.row(i) ;
        // Multiply the most recent exogenous draw by the appropriate rho and
        // add the innovation
  }
  
  rowvec rhs = zeros<rowvec>( n_endog ) ;
  mat err = zeros( n_integ, n_endog ) ;
      // Initialize the right hand side
      
  for( int i = 0 ; i < n_integ ; i++ ){
    err.row(i) = betta * 
              integrand_ngm_cont_2( exog, endog, cont, exog_lead.row(i), params, 
                                      coeffs, coeffs_cont, n_exog, n_endog, 
                                      n_cont, N, upper, lower, cheby ) ;
  }   // Compute the integral

  rhs = weights * err ;
  
    if( print_rhs ){
      Rcout << "err: \n" << err << std::endl ;
      Rcout << "weights:" << weights << std::endl ;
      Rcout << "rhs: " << rhs << std::endl ;
      Rcout << "rhs(0) - 1 = " << rhs(0) - 1 << std::endl ;
    }
  
  rowvec endog_hat = rhs % endog.row(0) ;
  return endog_hat ;
      // Return k * the Euler Error
  
}

// [[Rcpp::export]]
arma::rowvec con_eqns_ngm_2( 
  arma::mat exog, arma::mat endog, List params, arma::mat coeffs, 
  arma::mat coeffs_cont, int n_exog, int n_endog, int n_cont, int N, 
  arma::rowvec upper, arma::rowvec lower, bool cheby=false ){
// Computes the predicted controls.  In general can depend on the controls. 
// Happens not to in this case.

  // Extract parameters
  double A = params["A"] ;
  double alpha = params["alpha"] ;
  double delta = params["delta"] ;

  rowvec out = zeros<rowvec>(n_cont) ;
      // Initialize the output vector
  out(0) = .5 * ( ( 1 - delta ) * sum( endog.row( 1 ) ) - sum( endog.row( 0 ) ) + 
                  exp( exog( 0, 0 ) ) * A * pow( endog( 1, 0 ), alpha ) +
                  exp( exog( 0, 1 ) ) * A * pow( endog( 1, 1 ), alpha ) ) ;
  return( out ) ;
}

// [[Rcpp::export]]
arma::rowvec ngm_reg_2( 
                  arma::mat exog, arma::mat endog, arma::rowvec cont,
                  arma::mat exog_innov_integ, 
                  List params, arma::mat coeffs, arma::mat coeffs_cont, 
                  int n_exog, int n_endog, int n_cont,
                  arma::rowvec rho, int n_integ, int N, arma::rowvec upper, 
                  arma::rowvec lower, bool cheby, arma::rowvec weights, 
                  bool print_rhs=false ){
// Computes the dependent variables for the regression problem.  Aggregates both
// the states and controls.
  rowvec out = zeros<rowvec>( n_endog + n_cont ) ;
      // Initialize the output
  out.head(n_endog) = 
        euler_hat_ngm_cont_2( exog, endog, cont, exog_innov_integ, params, 
                            coeffs, coeffs_cont, n_exog, n_endog, n_cont,
                            rho, n_integ, N, upper, lower, cheby, weights,
                            print_rhs ) ;
      // The endogenous states
  out.tail(n_cont) = 
        con_eqns_ngm_2( exog, endog, params, coeffs, coeffs_cont, n_exog, 
                      n_endog, n_cont, N, upper, lower, cheby ) ;
      // The controls
  return out ;
}