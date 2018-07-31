#ifndef TRANSLATION_ROTATION_INTERNAL_COORDINATES_H
#define TRANSLATION_ROTATION_INTERNAL_COORDINATES_H

#include "PrimitiveInternalCoordinates.h"

namespace internals {

  class TRIC : public system {
  public:
    TRIC(const std::vector<coords::Representation_3D>& res_init,
      const std::vector<std::vector<std::size_t>>& res_index,
      CartesianType const& xyz_init, BondGraph const& graph) : system{ res_init, res_index, xyz_init } {
      system::create_ic_system(graph);
      delocalize_ic_system();
    }


    scon::mathmatrix<coords::float_type>& Bmat() override;//F
    scon::mathmatrix<coords::float_type>& Gmat() override;//F
    scon::mathmatrix<coords::float_type>& delocalize_ic_system();//F
    scon::mathmatrix<coords::float_type>& guess_hessian() override;
    scon::mathmatrix<coords::float_type> calc(coords::Representation_3D const& xyz) const override;//F
    scon::mathmatrix<coords::float_type> calc_diff(coords::Representation_3D const& lhs, coords::Representation_3D const& rhs) const override;//F

    scon::mathmatrix<coords::float_type> const& getDelMat()const { return del_mat; }
  protected:
    scon::mathmatrix<coords::float_type> del_mat;
  };
}
#endif