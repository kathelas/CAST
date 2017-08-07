#pragma once 

#if defined _OPENMP
  #include <omp.h>
#endif

#include <iostream>
#include <memory>
#include <string>
#include "coords_rep.h"

namespace energy
{
	/**object where fep parameters for one window are saved*/
  struct fepvar
  {
	  /**current lambda_el for appearing atoms*/
	  coords::float_type ein;
	  /**current lambda_el for disappearing atoms*/
	  coords::float_type eout;
	  /**current lambda_vdw for appearing atoms*/
	  coords::float_type vin;
	  /**current lambda_vdw for disappearing atoms*/
	  coords::float_type vout;
	  /**(lambda + dlambda)_el for appearing atoms*/
	  coords::float_type dein;
	  /**(lambda + dlambda)_el for disappearing atoms*/
	  coords::float_type deout;
	  /**(lambda + dlambda)_vdw for appearing atoms*/
	  coords::float_type dvin;
	  /**(lambda + dlambda)_vdw for disappearing atoms*/
	  coords::float_type dvout;
	/**number of current window*/
    int step;
    fepvar (void) 
      : ein(1.0), eout(1.0), vin(1.0), vout(1.0),
      dein(1.0), deout(1.0), dvin(1.0), dvout(1.0),
      step(0)
    { }
  };

  /**object in which data for one conformation is saved
  which is relevant for FEP calculation*/
  struct fepvect
  {
	  /**coulomb-energy for this conformation with lambda*/
	  coords::float_type e_c_l1;
	  /**coulomb-energy for this conformation with lambda+dlambda*/
	  coords::float_type e_c_l2;
	  /**vdw-energy for this conformation with lambda*/
	  coords::float_type e_vdw_l1;
	  /**vdw-energy for this conformation with lambda+dlambda*/
	  coords::float_type e_vdw_l2;
	  /**difference in potential energy for this conformation
	  = (e_c_l2 + e_vdw_l2) - (e_c_l1 + e_vdw_l1)*/
	  coords::float_type dE;
	  /**difference in free energy calculated for all conformations 
	  in this window until current conformation*/
	  coords::float_type dG;
	  /**exp((-1 / (k_B*T))*dE ) for this conformation*/
	  coords::float_type de_ens;
	  /**temperature*/
    coords::float_type T;
    fepvect (void) :
      e_c_l1(0.0), e_c_l2(0.0), e_vdw_l1(0.0), 
      e_vdw_l2(0.0), dE(0.0), dG(0.0), de_ens(0.0), T(0.0)
    { }
  };
  
  /**Abstract interface base class, 
  -> specialization for FF, MOPAC, terachem etc.*/
  class interface_base
  {
  protected:

    coords::Coordinates * const coords;
    bool periodic, integrity, optimizer, interactions, internal_optimizer;

  public:

    coords::float_type energy;
    coords::Cartesian_Point pb_max, pb_min, pb_dim;

    interface_base (coords::Coordinates *coord_pointer) : 
      coords(coord_pointer), periodic(false), integrity(true), 
      optimizer(false), interactions(false), energy(0.0)
    { 
      if(!coord_pointer) throw std::runtime_error("Interface without valid coordinates prohibited."); 
    }

    interface_base& operator= (interface_base const &other)
    {
      energy = other.energy;
      pb_max = other.pb_max;
      pb_min = other.pb_min;
      pb_dim = other.pb_dim;
      periodic = other.periodic;
      integrity = other.integrity;
      optimizer = other.optimizer;
      internal_optimizer = other.internal_optimizer;
      interactions = other.interactions;
      return *this;
    }

    virtual void swap (interface_base &other) = 0;
    
    /** create an copy-instance of derived via new and return pointer*/
    virtual interface_base * clone (coords::Coordinates * coord_object) const = 0;
    // create new instance of derived and move in data
    virtual interface_base * move (coords::Coordinates * coord_object) = 0;

    /** update interface information from coordinates*/
    virtual void update (bool const skip_topology = false) = 0;
    /** delete interface*/
    virtual ~interface_base (void) { }

    /** Energy function*/
    virtual coords::float_type e (void) = 0;

    /** Energy+Gradient function*/
    virtual coords::float_type g (void) = 0;

    /** Energy+Gradient function*/
    //virtual coords::float_type gi(void) = 0;

    /** Energy+Gradient+Hessian function*/
    virtual coords::float_type h (void) = 0;

    /** Optimization in the intface or interfaced program*/
    virtual coords::float_type o (void) = 0;

    // Feature getter
    bool has_periodics() const { return periodic; }
    bool has_optimizer() const { return optimizer; }
    bool has_internal_optimizer() const { return optimizer; }
    bool has_interactions() const { return interactions; }

    bool intact() const { return integrity; }

    // Output functions
    virtual void print_E (std::ostream&) const = 0;
    virtual void print_E_head (std::ostream&, bool const endline = true) const = 0;
    virtual void print_E_short (std::ostream&, bool const endline = true) const = 0;
    virtual void print_G_tinkerlike (std::ostream&, bool const aggregate = false) const = 0;
    virtual void to_stream (std::ostream&) const = 0;

  };

  interface_base* new_interface (coords::Coordinates *);
  interface_base* pre_interface (coords::Coordinates *);
}
