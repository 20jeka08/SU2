/*!
 * \file solution_direct_poisson.cpp
 * \brief Main subrotuines for solving direct problems
 * \author F. Palacios
 * \version 6.0.0 "Falcon"
 *
 * The current SU2 release has been coordinated by the
 * SU2 International Developers Society <www.su2devsociety.org>
 * with selected contributions from the open-source community.
 *
 * The main research teams contributing to the current release are:
 *  - Prof. Juan J. Alonso's group at Stanford University.
 *  - Prof. Piero Colonna's group at Delft University of Technology.
 *  - Prof. Nicolas R. Gauger's group at Kaiserslautern University of Technology.
 *  - Prof. Alberto Guardone's group at Polytechnic University of Milan.
 *  - Prof. Rafael Palacios' group at Imperial College London.
 *  - Prof. Vincent Terrapon's group at the University of Liege.
 *  - Prof. Edwin van der Weide's group at the University of Twente.
 *  - Lab. of New Concepts in Aeronautics at Tech. Institute of Aeronautics.
 *
 * Copyright 2012-2018, Francisco D. Palacios, Thomas D. Economon,
 *                      Tim Albring, and the SU2 contributors.
 *
 * SU2 is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * SU2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with SU2. If not, see <http://www.gnu.org/licenses/>.
 */
 
 
#include "../include/solver_structure.hpp"
 
CPBIncEulerSolver::CPBIncEulerSolver(void) : CSolver() {
  /*--- Basic array initialization ---*/

  CD_Inv  = NULL; CL_Inv  = NULL; CSF_Inv = NULL;  CEff_Inv = NULL;
  CMx_Inv = NULL; CMy_Inv = NULL; CMz_Inv = NULL;
  CFx_Inv = NULL; CFy_Inv = NULL; CFz_Inv = NULL;

  CD_Mnt  = NULL; CL_Mnt  = NULL; CSF_Mnt = NULL;  CEff_Mnt = NULL;
  CMx_Mnt = NULL; CMy_Mnt = NULL; CMz_Mnt = NULL;
  CFx_Mnt = NULL; CFy_Mnt = NULL; CFz_Mnt = NULL;

  CPressure = NULL; CPressureTarget = NULL; HeatFlux = NULL; HeatFluxTarget = NULL; YPlus = NULL;
  ForceInviscid = NULL; MomentInviscid = NULL;
  ForceMomentum = NULL; MomentMomentum = NULL;

  /*--- Surface based array initialization ---*/

  Surface_CL_Inv  = NULL; Surface_CD_Inv  = NULL; Surface_CSF_Inv = NULL; Surface_CEff_Inv = NULL;
  Surface_CFx_Inv = NULL; Surface_CFy_Inv = NULL; Surface_CFz_Inv = NULL;
  Surface_CMx_Inv = NULL; Surface_CMy_Inv = NULL; Surface_CMz_Inv = NULL;

  Surface_CL_Mnt  = NULL; Surface_CD_Mnt  = NULL; Surface_CSF_Mnt = NULL; Surface_CEff_Mnt = NULL;
  Surface_CFx_Mnt = NULL; Surface_CFy_Mnt = NULL; Surface_CFz_Mnt = NULL;
  Surface_CMx_Mnt = NULL; Surface_CMy_Mnt = NULL; Surface_CMz_Mnt = NULL;

  Surface_CL  = NULL; Surface_CD  = NULL; Surface_CSF = NULL; Surface_CEff = NULL;
  Surface_CFx = NULL; Surface_CFy = NULL; Surface_CFz = NULL;
  Surface_CMx = NULL; Surface_CMy = NULL; Surface_CMz = NULL;
  
  /*--- Rotorcraft simulation array initialization ---*/
  
  CMerit_Inv = NULL;  CT_Inv = NULL;  CQ_Inv = NULL;
  
  /*--- Numerical methods array initialization ---*/
  
  iPoint_UndLapl = NULL;
  jPoint_UndLapl = NULL;
  Primitive = NULL; Primitive_i = NULL; Primitive_j = NULL;
  CharacPrimVar = NULL;
  Smatrix = NULL; Cvector = NULL;
 
  /*--- Fixed CL mode initialization (cauchy criteria) ---*/
  
  Cauchy_Value = 0;
  Cauchy_Func = 0;
  Old_Func = 0;
  New_Func = 0;
  Cauchy_Counter = 0;
  Cauchy_Serie = NULL;
  
  FluidModel = NULL;
 
}

CPBIncEulerSolver::CPBIncEulerSolver(CGeometry *geometry, CConfig *config, unsigned short iMesh) : CSolver() {
  
  unsigned long iPoint, iVertex;
  unsigned short iVar, iDim, iMarker, nLineLets;
  ifstream restart_file;
  unsigned short nZone = geometry->GetnZone();
  bool restart   = (config->GetRestart() || config->GetRestart_Flow());
  string filename = config->GetSolution_FlowFileName();
  int Unst_RestartIter;
  unsigned short iZone = config->GetiZone();
  bool dual_time = ((config->GetUnsteady_Simulation() == DT_STEPPING_1ST) ||
                    (config->GetUnsteady_Simulation() == DT_STEPPING_2ND));
  bool time_stepping = config->GetUnsteady_Simulation() == TIME_STEPPING;
  bool adjoint = (config->GetContinuous_Adjoint()) || (config->GetDiscrete_Adjoint());
  bool fsi     = config->GetFSI_Simulation();
  string filename_ = config->GetSolution_FlowFileName();

  unsigned short direct_diff = config->GetDirectDiff();

  /*--- Check for a restart file to evaluate if there is a change in the angle of attack
   before computing all the non-dimesional quantities. ---*/


  /*--- Basic array initialization ---*/

  CD_Inv  = NULL; CL_Inv  = NULL; CSF_Inv = NULL;  CEff_Inv = NULL;
  CMx_Inv = NULL; CMy_Inv = NULL; CMz_Inv = NULL;
  CFx_Inv = NULL; CFy_Inv = NULL; CFz_Inv = NULL;

  CD_Mnt  = NULL; CL_Mnt  = NULL; CSF_Mnt = NULL; CEff_Mnt = NULL;
  CMx_Mnt = NULL; CMy_Mnt = NULL; CMz_Mnt = NULL;
  CFx_Mnt = NULL; CFy_Mnt = NULL; CFz_Mnt = NULL;

  CPressure = NULL; CPressureTarget = NULL; HeatFlux = NULL; HeatFluxTarget = NULL; YPlus = NULL;
  ForceInviscid = NULL; MomentInviscid = NULL;
  ForceMomentum = NULL;  MomentMomentum = NULL;

  /*--- Surface based array initialization ---*/

  Surface_CL_Inv  = NULL; Surface_CD_Inv  = NULL; Surface_CSF_Inv = NULL; Surface_CEff_Inv = NULL;
  Surface_CFx_Inv = NULL; Surface_CFy_Inv = NULL; Surface_CFz_Inv = NULL;
  Surface_CMx_Inv = NULL; Surface_CMy_Inv = NULL; Surface_CMz_Inv = NULL;

  Surface_CL_Mnt  = NULL; Surface_CD_Mnt  = NULL; Surface_CSF_Mnt = NULL; Surface_CEff_Mnt= NULL;
  Surface_CFx_Mnt = NULL; Surface_CFy_Mnt = NULL; Surface_CFz_Mnt = NULL;
  Surface_CMx_Mnt = NULL; Surface_CMy_Mnt = NULL; Surface_CMz_Mnt = NULL;

  Surface_CL  = NULL; Surface_CD  = NULL; Surface_CSF = NULL; Surface_CEff = NULL;
  Surface_CMx = NULL; Surface_CMy = NULL; Surface_CMz = NULL;

  /*--- Rotorcraft simulation array initialization ---*/

  CMerit_Inv = NULL;  CT_Inv = NULL;  CQ_Inv = NULL;

  /*--- Numerical methods array initialization ---*/
  
  iPoint_UndLapl = NULL;
  jPoint_UndLapl = NULL;
  Primitive = NULL; Primitive_i = NULL; Primitive_j = NULL;
  CharacPrimVar = NULL;
  Smatrix = NULL; Cvector = NULL;

  /*--- Fixed CL mode initialization (cauchy criteria) ---*/

  Cauchy_Value = 0;
  Cauchy_Func = 0;
  Old_Func = 0;
  New_Func = 0;
  Cauchy_Counter = 0;
  Cauchy_Serie = NULL;
  
  /*--- Fluid model pointer initialization ---*/

  FluidModel = NULL;

  /*--- Set the gamma value ---*/
  
  Gamma = config->GetGamma();
  Gamma_Minus_One = Gamma - 1.0;
  
  /*--- Define geometry constants in the solver structure.
   * Incompressible flow, primitive variables (P, vx, vy, vz, rho) ---*/
  
  nDim = geometry->GetnDim();
  
  nVar = nDim; nPrimVar = nDim+2; nPrimVarGrad = nDim+2;

  /*--- Initialize nVarGrad for deallocation ---*/
  
  nVarGrad = nPrimVarGrad;
  
  nMarker      = config->GetnMarker_All();
  nPoint       = geometry->GetnPoint();
  nPointDomain = geometry->GetnPointDomain();
 
  /*--- Store the number of vertices on each marker for deallocation later ---*/

  nVertex = new unsigned long[nMarker];
  for (iMarker = 0; iMarker < nMarker; iMarker++) 
    nVertex[iMarker] = geometry->nVertex[iMarker];
 
  /*--- Perform the non-dimensionalization for the flow equations using the
   specified reference values. ---*/
  
  SetNondimensionalization(geometry, config, iMesh);
  
  /*--- Allocate the node variables ---*/
  
  node = new CVariable*[nPoint];
  
  /*--- Define some auxiliary vectors related to the residual ---*/
  
  Residual      = new su2double[nVar]; for (iVar = 0; iVar < nVar; iVar++) Residual[iVar]     = 0.0;
  Residual_RMS  = new su2double[nVar]; for (iVar = 0; iVar < nVar; iVar++) Residual_RMS[iVar] = 0.0;
  Residual_Max  = new su2double[nVar]; for (iVar = 0; iVar < nVar; iVar++) Residual_Max[iVar] = 0.0;
  Residual_i    = new su2double[nVar]; for (iVar = 0; iVar < nVar; iVar++) Residual_i[iVar]   = 0.0;
  Residual_j    = new su2double[nVar]; for (iVar = 0; iVar < nVar; iVar++) Residual_j[iVar]   = 0.0;
  Res_Conv      = new su2double[nVar]; for (iVar = 0; iVar < nVar; iVar++) Res_Conv[iVar]     = 0.0;
  Res_Visc      = new su2double[nVar]; for (iVar = 0; iVar < nVar; iVar++) Res_Visc[iVar]     = 0.0;
  Res_Sour      = new su2double[nVar]; for (iVar = 0; iVar < nVar; iVar++) Res_Sour[iVar]     = 0.0;
  
  /*--- Define some structures for locating max residuals ---*/
  
  Point_Max = new unsigned long[nVar];
  for (iVar = 0; iVar < nVar; iVar++) Point_Max[iVar] = 0;
  
  Point_Max_Coord = new su2double*[nVar];
  for (iVar = 0; iVar < nVar; iVar++) {
    Point_Max_Coord[iVar] = new su2double[nDim];
    for (iDim = 0; iDim < nDim; iDim++) Point_Max_Coord[iVar][iDim] = 0.0;
  }
  
  /*--- Define some auxiliary vectors related to the solution ---*/
  
  Solution   = new su2double[nVar]; for (iVar = 0; iVar < nVar; iVar++) Solution[iVar]   = 0.0;
  Solution_i = new su2double[nVar]; for (iVar = 0; iVar < nVar; iVar++) Solution_i[iVar] = 0.0;
  Solution_j = new su2double[nVar]; for (iVar = 0; iVar < nVar; iVar++) Solution_j[iVar] = 0.0;
  
  /*--- Define some auxiliary vectors related to the geometry ---*/
  
  Vector   = new su2double[nDim]; for (iDim = 0; iDim < nDim; iDim++) Vector[iDim]   = 0.0;
  Vector_i = new su2double[nDim]; for (iDim = 0; iDim < nDim; iDim++) Vector_i[iDim] = 0.0;
  Vector_j = new su2double[nDim]; for (iDim = 0; iDim < nDim; iDim++) Vector_j[iDim] = 0.0;
  
  /*--- Define some auxiliary vectors related to the primitive solution ---*/
  
  Primitive   = new su2double[nPrimVar]; for (iVar = 0; iVar < nPrimVar; iVar++) Primitive[iVar]   = 0.0;
  Primitive_i = new su2double[nPrimVar]; for (iVar = 0; iVar < nPrimVar; iVar++) Primitive_i[iVar] = 0.0;
  Primitive_j = new su2double[nPrimVar]; for (iVar = 0; iVar < nPrimVar; iVar++) Primitive_j[iVar] = 0.0;
  
  /*--- Define some auxiliary vectors related to the undivided lapalacian ---*/
  
  if (config->GetKind_ConvNumScheme_Flow() == SPACE_CENTERED) {
    iPoint_UndLapl = new su2double [nPoint];
    jPoint_UndLapl = new su2double [nPoint];
  }
  
  /*--- Initialize the solution and right-hand side vectors for storing
   the residuals and updating the solution (always needed even for
   explicit schemes). ---*/
  
  LinSysSol.Initialize(nPoint, nPointDomain, nVar, 0.0);
  LinSysRes.Initialize(nPoint, nPointDomain, nVar, 0.0);
  
  /*--- Jacobians and vector structures for implicit computations ---*/
  
  if (config->GetKind_TimeIntScheme_Flow() == EULER_IMPLICIT) {
    
    Jacobian_i = new su2double* [nVar];
    Jacobian_j = new su2double* [nVar];
    for (iVar = 0; iVar < nVar; iVar++) {
      Jacobian_i[iVar] = new su2double [nVar];
      Jacobian_j[iVar] = new su2double [nVar];
    }
    
    if (rank == MASTER_NODE) cout << "Initialize Jacobian structure (Euler). MG level: " << iMesh <<"." << endl;
    Jacobian.Initialize(nPoint, nPointDomain, nVar, nVar, true, geometry, config);
    
    if ((config->GetKind_Linear_Solver_Prec() == LINELET) ||
        (config->GetKind_Linear_Solver() == SMOOTHER_LINELET)) {
      nLineLets = Jacobian.BuildLineletPreconditioner(geometry, config);
      if (rank == MASTER_NODE) cout << "Compute linelet structure. " << nLineLets << " elements in each line (average)." << endl;
    }
    
  }
  
  else {
    if (rank == MASTER_NODE) cout << "Explicit scheme. No Jacobian structure (Euler). MG level: " << iMesh <<"." << endl;
  }
  
  /*--- Define some auxiliary vectors for computing flow variable
   gradients by least squares, S matrix := inv(R)*traspose(inv(R)),
   c vector := transpose(WA)*(Wb) ---*/
  
  if (config->GetKind_Gradient_Method() == WEIGHTED_LEAST_SQUARES) {
    
    Smatrix = new su2double* [nDim];
    for (iDim = 0; iDim < nDim; iDim++)
      Smatrix[iDim] = new su2double [nDim];
    
    Cvector = new su2double* [nPrimVarGrad];
    for (iVar = 0; iVar < nPrimVarGrad; iVar++)
      Cvector[iVar] = new su2double [nDim];
    
  }

  /*--- Store the value of the characteristic primitive variables at the boundaries ---*/

  CharacPrimVar = new su2double** [nMarker];
  for (iMarker = 0; iMarker < nMarker; iMarker++) {
    CharacPrimVar[iMarker] = new su2double* [geometry->nVertex[iMarker]];
    for (iVertex = 0; iVertex < geometry->nVertex[iMarker]; iVertex++) {
      CharacPrimVar[iMarker][iVertex] = new su2double [nPrimVar];
      for (iVar = 0; iVar < nPrimVar; iVar++) {
        CharacPrimVar[iMarker][iVertex][iVar] = 0.0;
      }
    }
  }
   
  /*--- Force definition and coefficient arrays for all of the markers ---*/
  
  CPressure = new su2double* [nMarker];
  CPressureTarget = new su2double* [nMarker];
  for (iMarker = 0; iMarker < nMarker; iMarker++) {
    CPressure[iMarker] = new su2double [geometry->nVertex[iMarker]];
    CPressureTarget[iMarker] = new su2double [geometry->nVertex[iMarker]];
    for (iVertex = 0; iVertex < geometry->nVertex[iMarker]; iVertex++) {
      CPressure[iMarker][iVertex] = 0.0;
      CPressureTarget[iMarker][iVertex] = 0.0;
    }
  }
  
  /*--- Store the value of the Total Pressure at the inlet BC ---*/
  
  Inlet_Ttotal = new su2double* [nMarker];
  for (iMarker = 0; iMarker < nMarker; iMarker++) {
    Inlet_Ttotal[iMarker] = new su2double [geometry->nVertex[iMarker]];
    for (iVertex = 0; iVertex < geometry->nVertex[iMarker]; iVertex++) {
      Inlet_Ttotal[iMarker][iVertex] = 0;
    }
  }
  
  /*--- Store the value of the Total Temperature at the inlet BC ---*/
  
  Inlet_Ptotal = new su2double* [nMarker];
  for (iMarker = 0; iMarker < nMarker; iMarker++) {
    Inlet_Ptotal[iMarker] = new su2double [geometry->nVertex[iMarker]];
    for (iVertex = 0; iVertex < geometry->nVertex[iMarker]; iVertex++) {
      Inlet_Ptotal[iMarker][iVertex] = 0;
    }
  }
  
  /*--- Store the value of the Flow direction at the inlet BC ---*/
  
  Inlet_FlowDir = new su2double** [nMarker];
  for (iMarker = 0; iMarker < nMarker; iMarker++) {
    Inlet_FlowDir[iMarker] = new su2double* [geometry->nVertex[iMarker]];
    for (iVertex = 0; iVertex < geometry->nVertex[iMarker]; iVertex++) {
      Inlet_FlowDir[iMarker][iVertex] = new su2double [nDim];
      for (iDim = 0; iDim < nDim; iDim++) {
        Inlet_FlowDir[iMarker][iVertex][iDim] = 0;
      }
    }
  }
  
  /*--- Non-dimensional coefficients ---*/

  ForceInviscid  = new su2double[nDim];
  MomentInviscid = new su2double[3];
  CD_Inv         = new su2double[nMarker];
  CL_Inv         = new su2double[nMarker];
  CSF_Inv        = new su2double[nMarker];
  CMx_Inv        = new su2double[nMarker];
  CMy_Inv        = new su2double[nMarker];
  CMz_Inv        = new su2double[nMarker];
  CEff_Inv       = new su2double[nMarker];
  CFx_Inv        = new su2double[nMarker];
  CFy_Inv        = new su2double[nMarker];
  CFz_Inv        = new su2double[nMarker];

  ForceMomentum  = new su2double[nDim];
  MomentMomentum = new su2double[3];
  CD_Mnt         = new su2double[nMarker];
  CL_Mnt         = new su2double[nMarker];
  CSF_Mnt        = new su2double[nMarker];
  CMx_Mnt        = new su2double[nMarker];
  CMy_Mnt        = new su2double[nMarker];
  CMz_Mnt        = new su2double[nMarker];
  CEff_Mnt       = new su2double[nMarker];
  CFx_Mnt        = new su2double[nMarker];
  CFy_Mnt        = new su2double[nMarker];
  CFz_Mnt        = new su2double[nMarker];

  Surface_CL_Inv   = new su2double[config->GetnMarker_Monitoring()];
  Surface_CD_Inv   = new su2double[config->GetnMarker_Monitoring()];
  Surface_CSF_Inv  = new su2double[config->GetnMarker_Monitoring()];
  Surface_CEff_Inv = new su2double[config->GetnMarker_Monitoring()];
  Surface_CFx_Inv  = new su2double[config->GetnMarker_Monitoring()];
  Surface_CFy_Inv  = new su2double[config->GetnMarker_Monitoring()];
  Surface_CFz_Inv  = new su2double[config->GetnMarker_Monitoring()];
  Surface_CMx_Inv  = new su2double[config->GetnMarker_Monitoring()];
  Surface_CMy_Inv  = new su2double[config->GetnMarker_Monitoring()];
  Surface_CMz_Inv  = new su2double[config->GetnMarker_Monitoring()];

  Surface_CL_Mnt   = new su2double[config->GetnMarker_Monitoring()];
  Surface_CD_Mnt   = new su2double[config->GetnMarker_Monitoring()];
  Surface_CSF_Mnt  = new su2double[config->GetnMarker_Monitoring()];
  Surface_CEff_Mnt = new su2double[config->GetnMarker_Monitoring()];
  Surface_CFx_Mnt  = new su2double[config->GetnMarker_Monitoring()];
  Surface_CFy_Mnt  = new su2double[config->GetnMarker_Monitoring()];
  Surface_CFz_Mnt  = new su2double[config->GetnMarker_Monitoring()];
  Surface_CMx_Mnt  = new su2double[config->GetnMarker_Monitoring()];
  Surface_CMy_Mnt  = new su2double[config->GetnMarker_Monitoring()];
  Surface_CMz_Mnt  = new su2double[config->GetnMarker_Monitoring()];

  Surface_CL   = new su2double[config->GetnMarker_Monitoring()];
  Surface_CD   = new su2double[config->GetnMarker_Monitoring()];
  Surface_CSF  = new su2double[config->GetnMarker_Monitoring()];
  Surface_CEff = new su2double[config->GetnMarker_Monitoring()];
  Surface_CFx  = new su2double[config->GetnMarker_Monitoring()];
  Surface_CFy  = new su2double[config->GetnMarker_Monitoring()];
  Surface_CFz  = new su2double[config->GetnMarker_Monitoring()];
  Surface_CMx  = new su2double[config->GetnMarker_Monitoring()];
  Surface_CMy  = new su2double[config->GetnMarker_Monitoring()];
  Surface_CMz  = new su2double[config->GetnMarker_Monitoring()];

  /*--- Rotorcraft coefficients ---*/

  CT_Inv           = new su2double[nMarker];
  CQ_Inv           = new su2double[nMarker];
  CMerit_Inv       = new su2double[nMarker];

  CT_Mnt           = new su2double[nMarker];
  CQ_Mnt           = new su2double[nMarker];
  CMerit_Mnt       = new su2double[nMarker];

  /*--- Init total coefficients ---*/

  Total_CD      = 0.0;  Total_CL           = 0.0;  Total_CSF          = 0.0;
  Total_CMx     = 0.0;  Total_CMy          = 0.0;  Total_CMz          = 0.0;
  Total_CEff    = 0.0;
  Total_CFx     = 0.0;  Total_CFy          = 0.0;  Total_CFz          = 0.0;
  Total_CT      = 0.0;  Total_CQ           = 0.0;  Total_CMerit       = 0.0;
  Total_MaxHeat = 0.0;  Total_Heat         = 0.0;  Total_ComboObj     = 0.0;
  Total_CpDiff  = 0.0;  Total_HeatFluxDiff = 0.0;  Total_Custom_ObjFunc=0.0;
  
  /*--- Coefficients for fixed lift mode. ---*/
  
  AoA_Prev = 0.0;
  Total_CL_Prev = 0.0; Total_CD_Prev = 0.0;
  Total_CMx_Prev = 0.0; Total_CMy_Prev = 0.0; Total_CMz_Prev = 0.0;

  /*--- Read farfield conditions ---*/

  Density_Inf     = config->GetDensity_FreeStreamND();
  Pressure_Inf    = config->GetPressure_FreeStreamND();
  Velocity_Inf    = config->GetVelocity_FreeStreamND();
  Temperature_Inf = config->GetTemperature_FreeStreamND();
  
  /*--- Initialize the secondary values for direct derivative approxiations ---*/
  
  switch(direct_diff){
    case NO_DERIVATIVE:
      /*--- Default ---*/
      break;
    case D_DENSITY:
      SU2_TYPE::SetDerivative(Density_Inf, 1.0);
      break;
    case D_PRESSURE:
      SU2_TYPE::SetDerivative(Pressure_Inf, 1.0);
      break;
    case D_TEMPERATURE:
      SU2_TYPE::SetDerivative(Temperature_Inf, 1.0);
      break;
    case D_MACH: case D_AOA:
    case D_SIDESLIP: case D_REYNOLDS:
    case D_TURB2LAM: case D_DESIGN:
      /*--- Already done in postprocessing of config ---*/
      break;
    default:
      break;
  }
  
  /*--- Initialize the cauchy critera array for fixed CL mode ---*/

  if (config->GetFixed_CL_Mode())
    Cauchy_Serie = new su2double [config->GetCauchy_Elems()+1];

  /*--- Initialize the solution to the far-field state everywhere. ---*/
  for (iPoint = 0; iPoint < nPoint; iPoint++)
    node[iPoint] = new CPBIncEulerVariable(Pressure_Inf, Velocity_Inf, nDim, nVar, config);

  /*--- Define solver parameters needed for execution of destructor ---*/

  if (config->GetKind_ConvNumScheme_Flow() == SPACE_CENTERED ) space_centered = true;
  else space_centered = false;

  if (config->GetKind_TimeIntScheme_Flow() == EULER_IMPLICIT) euler_implicit = true;
  else euler_implicit = false;

  if (config->GetKind_Gradient_Method() == WEIGHTED_LEAST_SQUARES) least_squares = true;
  else least_squares = false;

  /*--- Perform the MPI communication of the solution ---*/

  Set_MPI_Solution(geometry, config);

}

CPBIncEulerSolver::~CPBIncEulerSolver(void) {

  unsigned short iMarker;
  unsigned long iVertex;

  /*--- Array deallocation ---*/

  if (CD_Inv  != NULL) delete [] CD_Inv;
  if (CL_Inv  != NULL) delete [] CL_Inv;
  if (CSF_Inv != NULL) delete [] CSF_Inv;
  if (CMx_Inv != NULL) delete [] CMx_Inv;
  if (CMy_Inv != NULL) delete [] CMy_Inv;
  if (CMz_Inv != NULL) delete [] CMz_Inv;
  if (CFx_Inv != NULL) delete [] CFx_Inv;
  if (CFy_Inv != NULL) delete [] CFy_Inv;
  if (CFz_Inv != NULL) delete [] CFz_Inv;
  
  if (Surface_CL_Inv   != NULL) delete [] Surface_CL_Inv;
  if (Surface_CD_Inv   != NULL) delete [] Surface_CD_Inv;
  if (Surface_CSF_Inv  != NULL) delete [] Surface_CSF_Inv;
  if (Surface_CEff_Inv != NULL) delete [] Surface_CEff_Inv;
  if (Surface_CFx_Inv  != NULL) delete [] Surface_CFx_Inv;
  if (Surface_CFy_Inv  != NULL) delete [] Surface_CFy_Inv;
  if (Surface_CFz_Inv  != NULL) delete [] Surface_CFz_Inv;
  if (Surface_CMx_Inv  != NULL) delete [] Surface_CMx_Inv;
  if (Surface_CMy_Inv  != NULL) delete [] Surface_CMy_Inv;
  if (Surface_CMz_Inv  != NULL) delete [] Surface_CMz_Inv;

  if (CD_Mnt  != NULL) delete [] CD_Mnt;
  if (CL_Mnt  != NULL) delete [] CL_Mnt;
  if (CSF_Mnt != NULL) delete [] CSF_Mnt;
  if (CMx_Mnt != NULL) delete [] CMx_Mnt;
  if (CMy_Mnt != NULL) delete [] CMy_Mnt;
  if (CMz_Mnt != NULL) delete [] CMz_Mnt;
  if (CFx_Mnt != NULL) delete [] CFx_Mnt;
  if (CFy_Mnt != NULL) delete [] CFy_Mnt;
  if (CFz_Mnt != NULL) delete [] CFz_Mnt;
  
  if (Surface_CL_Mnt   != NULL) delete [] Surface_CL_Mnt;
  if (Surface_CD_Mnt   != NULL) delete [] Surface_CD_Mnt;
  if (Surface_CSF_Mnt  != NULL) delete [] Surface_CSF_Mnt;
  if (Surface_CEff_Mnt != NULL) delete [] Surface_CEff_Mnt;
  if (Surface_CFx_Mnt  != NULL) delete [] Surface_CFx_Mnt;
  if (Surface_CFy_Mnt  != NULL) delete [] Surface_CFy_Mnt;
  if (Surface_CFz_Mnt  != NULL) delete [] Surface_CFz_Mnt;
  if (Surface_CMx_Mnt  != NULL) delete [] Surface_CMx_Mnt;
  if (Surface_CMy_Mnt  != NULL) delete [] Surface_CMy_Mnt;
  if (Surface_CMz_Mnt  != NULL) delete [] Surface_CMz_Mnt;

  if (Surface_CL   != NULL) delete [] Surface_CL;
  if (Surface_CD   != NULL) delete [] Surface_CD;
  if (Surface_CSF  != NULL) delete [] Surface_CSF;
  if (Surface_CEff != NULL) delete [] Surface_CEff;
  if (Surface_CFx  != NULL) delete [] Surface_CFx;
  if (Surface_CFy  != NULL) delete [] Surface_CFy;
  if (Surface_CFz  != NULL) delete [] Surface_CFz;
  if (Surface_CMx  != NULL) delete [] Surface_CMx;
  if (Surface_CMy  != NULL) delete [] Surface_CMy;
  if (Surface_CMz  != NULL) delete [] Surface_CMz;
  
  if (CEff_Inv   != NULL) delete [] CEff_Inv;
  if (CMerit_Inv != NULL) delete [] CMerit_Inv;
  if (CT_Inv     != NULL) delete [] CT_Inv;
  if (CQ_Inv     != NULL) delete [] CQ_Inv;

  if (CEff_Mnt   != NULL) delete [] CEff_Mnt;
  if (CMerit_Mnt != NULL) delete [] CMerit_Mnt;
  if (CT_Mnt     != NULL) delete [] CT_Mnt;
  if (CQ_Mnt     != NULL) delete [] CQ_Mnt;

  if (ForceInviscid  != NULL) delete [] ForceInviscid;
  if (MomentInviscid != NULL) delete [] MomentInviscid;
  if (ForceMomentum  != NULL) delete [] ForceMomentum;
  if (MomentMomentum != NULL) delete [] MomentMomentum;

  if (iPoint_UndLapl != NULL) delete [] iPoint_UndLapl;
  if (jPoint_UndLapl != NULL) delete [] jPoint_UndLapl;

  if (Primitive   != NULL) delete [] Primitive;
  if (Primitive_i != NULL) delete [] Primitive_i;
  if (Primitive_j != NULL) delete [] Primitive_j;

  if (CPressure != NULL) {
    for (iMarker = 0; iMarker < nMarker; iMarker++)
      delete [] CPressure[iMarker];
    delete [] CPressure;
  }
  
  if (CPressureTarget != NULL) {
    for (iMarker = 0; iMarker < nMarker; iMarker++)
      delete [] CPressureTarget[iMarker];
    delete [] CPressureTarget;
  }

  if (CharacPrimVar != NULL) {
    for (iMarker = 0; iMarker < nMarker; iMarker++) {
      for (iVertex = 0; iVertex<nVertex[iMarker]; iVertex++)
        delete [] CharacPrimVar[iMarker][iVertex];
      delete [] CharacPrimVar[iMarker];
    }
    delete [] CharacPrimVar;
  }

  if (nVertex!=NULL) delete [] nVertex;

  if (HeatFlux != NULL) {
    for (iMarker = 0; iMarker < nMarker; iMarker++) {
      delete [] HeatFlux[iMarker];
    }
    delete [] HeatFlux;
  }
  
  if (HeatFluxTarget != NULL) {
    for (iMarker = 0; iMarker < nMarker; iMarker++) {
      delete [] HeatFluxTarget[iMarker];
    }
    delete [] HeatFluxTarget;
  }
  
  if (YPlus != NULL) {
    for (iMarker = 0; iMarker < nMarker; iMarker++) {
      delete [] YPlus[iMarker];
    }
    delete [] YPlus;
  }
  
  if (Cauchy_Serie != NULL) delete [] Cauchy_Serie;
  
  if (FluidModel != NULL) delete FluidModel;
  
  if (Inlet_Ttotal != NULL) {
    for (iMarker = 0; iMarker < nMarker; iMarker++)
      if (Inlet_Ttotal[iMarker] != NULL)
        delete [] Inlet_Ttotal[iMarker];
    delete [] Inlet_Ttotal;
  }
  
  if (Inlet_Ptotal != NULL) {
    for (iMarker = 0; iMarker < nMarker; iMarker++)
      if (Inlet_Ptotal[iMarker] != NULL)
        delete [] Inlet_Ptotal[iMarker];
    delete [] Inlet_Ptotal;
  }
  
    /*if (Inlet_FlowDir != NULL) {
    for (iMarker = 0; iMarker < nMarker; iMarker++) {
      if (Inlet_FlowDir[iMarker] != NULL) {
        for (iVertex = 0; iVertex < nVertex[iMarker]; iVertex++)
          delete [] Inlet_FlowDir[iMarker][iVertex];
        delete [] Inlet_FlowDir[iMarker];
      }
    }
    delete [] Inlet_FlowDir;
  }*/
}

void CPBIncEulerSolver::SetInitialCondition(CGeometry **geometry, CSolver ***solver_container, CConfig *config, unsigned long ExtIter) {
  
  unsigned long iPoint, Point_Fine;
  unsigned short iMesh, iChildren, iVar;
  su2double Density, Pressure, yCoord,
  Area_Children, Area_Parent, *Solution_Fine,
  *Solution, PressRef, yCoordRef;
  
  unsigned short nDim = geometry[MESH_0]->GetnDim();
  
  bool restart   = (config->GetRestart() || config->GetRestart_Flow());
  bool rans      = ((config->GetKind_Solver() == RANS) ||
               (config->GetKind_Solver() == ADJ_RANS) ||
               (config->GetKind_Solver() == DISC_ADJ_RANS));
  bool dual_time = ((config->GetUnsteady_Simulation() == DT_STEPPING_1ST) ||
                    (config->GetUnsteady_Simulation() == DT_STEPPING_2ND));
  bool gravity   = (config->GetGravityForce() == YES);
  
  /*--- Set the pressure value in simulations with gravity ---*/

  if (gravity ) {
    
    for (iMesh = 0; iMesh <= config->GetnMGLevels(); iMesh++) {
      
      for (iPoint = 0; iPoint < geometry[iMesh]->GetnPoint(); iPoint++) {
        
        /*--- Set initial boundary condition at iter 0 ---*/
        
        if ((ExtIter == 0) && (!restart)) {
          
          /*--- Update solution with the new pressure ---*/
          
          PressRef = solver_container[iMesh][FLOW_SOL]->GetPressure_Inf();
          Density = solver_container[iMesh][FLOW_SOL]->GetDensity_Inf();
          yCoordRef = 0.0;
          yCoord = geometry[iMesh]->node[iPoint]->GetCoord(nDim-1);
          Pressure = PressRef + Density*((yCoordRef-yCoord)/(config->GetFroude()*config->GetFroude()));
          solver_container[iMesh][FLOW_SOL]->node[iPoint]->SetSolution(0, Pressure);
          
        }
        
      }
      
      /*--- Set the MPI communication ---*/
      
      solver_container[iMesh][FLOW_SOL]->Set_MPI_Solution(geometry[iMesh], config);
      solver_container[iMesh][FLOW_SOL]->Set_MPI_Solution_Old(geometry[iMesh], config);
      
    }
    
  }
  
  /*--- If restart solution, then interpolate the flow solution to
   all the multigrid levels, this is important with the dual time strategy ---*/
  
  if (restart && (ExtIter == 0)) {
    
    Solution = new su2double[nVar];
    for (iMesh = 1; iMesh <= config->GetnMGLevels(); iMesh++) {
      for (iPoint = 0; iPoint < geometry[iMesh]->GetnPoint(); iPoint++) {
        Area_Parent = geometry[iMesh]->node[iPoint]->GetVolume();
        for (iVar = 0; iVar < nVar; iVar++) Solution[iVar] = 0.0;
        for (iChildren = 0; iChildren < geometry[iMesh]->node[iPoint]->GetnChildren_CV(); iChildren++) {
          Point_Fine = geometry[iMesh]->node[iPoint]->GetChildren_CV(iChildren);
          Area_Children = geometry[iMesh-1]->node[Point_Fine]->GetVolume();
          Solution_Fine = solver_container[iMesh-1][FLOW_SOL]->node[Point_Fine]->GetSolution();
          for (iVar = 0; iVar < nVar; iVar++) {
            Solution[iVar] += Solution_Fine[iVar]*Area_Children/Area_Parent;
          }
        }
        solver_container[iMesh][FLOW_SOL]->node[iPoint]->SetSolution(Solution);
      }
      solver_container[iMesh][FLOW_SOL]->Set_MPI_Solution(geometry[iMesh], config);
    }
    delete [] Solution;
    
    /*--- Interpolate the turblence variable also, if needed ---*/
    
    if (rans) {
      
      unsigned short nVar_Turb = solver_container[MESH_0][TURB_SOL]->GetnVar();
      Solution = new su2double[nVar_Turb];
      for (iMesh = 1; iMesh <= config->GetnMGLevels(); iMesh++) {
        for (iPoint = 0; iPoint < geometry[iMesh]->GetnPoint(); iPoint++) {
          Area_Parent = geometry[iMesh]->node[iPoint]->GetVolume();
          for (iVar = 0; iVar < nVar_Turb; iVar++) Solution[iVar] = 0.0;
          for (iChildren = 0; iChildren < geometry[iMesh]->node[iPoint]->GetnChildren_CV(); iChildren++) {
            Point_Fine = geometry[iMesh]->node[iPoint]->GetChildren_CV(iChildren);
            Area_Children = geometry[iMesh-1]->node[Point_Fine]->GetVolume();
            Solution_Fine = solver_container[iMesh-1][TURB_SOL]->node[Point_Fine]->GetSolution();
            for (iVar = 0; iVar < nVar_Turb; iVar++) {
              Solution[iVar] += Solution_Fine[iVar]*Area_Children/Area_Parent;
            }
          }
          solver_container[iMesh][TURB_SOL]->node[iPoint]->SetSolution(Solution);
        }
        solver_container[iMesh][TURB_SOL]->Set_MPI_Solution(geometry[iMesh], config);
        solver_container[iMesh][TURB_SOL]->Postprocessing(geometry[iMesh], solver_container[iMesh], config, iMesh);
      }
      delete [] Solution;
    }
    
  }
  
  /*--- The value of the solution for the first iteration of the dual time ---*/
  
  //if (dual_time && (ExtIter == 0 || (restart && (long)ExtIter == config->GetUnst_RestartIter()))) {
    
    ///*--- Push back the initial condition to previous solution containers
     //for a 1st-order restart or when simply intitializing to freestream. ---*/
    
    //for (iMesh = 0; iMesh <= config->GetnMGLevels(); iMesh++) {
      //for (iPoint = 0; iPoint < geometry[iMesh]->GetnPoint(); iPoint++) {
        //solver_container[iMesh][FLOW_SOL]->node[iPoint]->Set_Solution_time_n();
        //solver_container[iMesh][FLOW_SOL]->node[iPoint]->Set_Solution_time_n1();
        //if (rans) {
          //solver_container[iMesh][TURB_SOL]->node[iPoint]->Set_Solution_time_n();
          //solver_container[iMesh][TURB_SOL]->node[iPoint]->Set_Solution_time_n1();
        //}
      //}
    //}
    
    //if ((restart && (long)ExtIter == config->GetUnst_RestartIter()) &&
        //(config->GetUnsteady_Simulation() == DT_STEPPING_2ND)) {
      
      ///*--- Load an additional restart file for a 2nd-order restart ---*/
      
      //solver_container[MESH_0][FLOW_SOL]->LoadRestart(geometry, solver_container, config, SU2_TYPE::Int(config->GetUnst_RestartIter()-1), true);
      
      ///*--- Load an additional restart file for the turbulence model ---*/
      //if (rans)
        //solver_container[MESH_0][TURB_SOL]->LoadRestart(geometry, solver_container, config, SU2_TYPE::Int(config->GetUnst_RestartIter()-1), false);
      
      ///*--- Push back this new solution to time level N. ---*/
      
      //for (iMesh = 0; iMesh <= config->GetnMGLevels(); iMesh++) {
        //for (iPoint = 0; iPoint < geometry[iMesh]->GetnPoint(); iPoint++) {
          //solver_container[iMesh][FLOW_SOL]->node[iPoint]->Set_Solution_time_n();
          //if (rans) {
            //solver_container[iMesh][TURB_SOL]->node[iPoint]->Set_Solution_time_n();
          //}
        //}
      //}
    //}
  //}
}

void CPBIncEulerSolver::Preprocessing(CGeometry *geometry, CSolver **solver_container, CConfig *config, unsigned short iMesh, unsigned short iRKStep, unsigned short RunTime_EqSystem, bool Output) {
  
  unsigned long ErrorCounter = 0;

  unsigned long ExtIter = config->GetExtIter();
  bool cont_adjoint     = config->GetContinuous_Adjoint();
  bool disc_adjoint     = config->GetDiscrete_Adjoint();
  bool implicit         = (config->GetKind_TimeIntScheme_Flow() == EULER_IMPLICIT);
  bool muscl            = (config->GetMUSCL_Flow() || (cont_adjoint && config->GetKind_ConvNumScheme_AdjFlow() == ROE));
  bool limiter          = ((config->GetKind_SlopeLimit_Flow() != NO_LIMITER) && (ExtIter <= config->GetLimiterIter()) && !(disc_adjoint && config->GetFrozen_Limiter_Disc()));
  bool center           = ((config->GetKind_ConvNumScheme_Flow() == SPACE_CENTERED) || (cont_adjoint && config->GetKind_ConvNumScheme_AdjFlow() == SPACE_CENTERED));
  bool center_jst       = center && (config->GetKind_Centered_Flow() == JST);
  bool fixed_cl         = config->GetFixed_CL_Mode();
  su2double				*TestVec;
  unsigned long 		iVertex;
  unsigned short		iVar, iMarker;

  /*--- Update the angle of attack at the far-field for fixed CL calculations. ---*/
  
  if (fixed_cl) { SetFarfield_AoA(geometry, solver_container, config, iMesh, Output); }

  /*--- Set the primitive variables ---*/
  
  ErrorCounter = SetPrimitive_Variables(solver_container, config, Output);
  
  /*--- Compute Primitive gradients to be used in Mass flux computation and pressure residual ---*/
  
  if ((iMesh == MESH_0) && !Output) {
    
    /*--- Gradient computation ---*/
    
    if (config->GetKind_Gradient_Method() == GREEN_GAUSS) {
      SetPrimitive_Gradient_GG(geometry, config);
    }
    if (config->GetKind_Gradient_Method() == WEIGHTED_LEAST_SQUARES) {
      SetPrimitive_Gradient_LS(geometry, config);
    }
    
    /*--- Limiter computation ---*/
    
    if ((limiter) && (iMesh == MESH_0) && !Output) {
      SetPrimitive_Limiter(geometry, config);
    }
    
  }
  
  /*--- Artificial dissipation ---*/
  
  if (center && !Output) {
    SetMax_Eigenvalue(geometry, config);
    if ((center_jst) && (iMesh == MESH_0)) {
      SetCentered_Dissipation_Sensor(geometry, config);
      SetUndivided_Laplacian(geometry, config);
    }
  }
  
  /*--- Initialize the Jacobian matrices ---*/
  
  if (implicit && !disc_adjoint) Jacobian.SetValZero();

  /*--- Error message ---*/
  
  if (config->GetConsole_Output_Verb() == VERB_HIGH) {
#ifdef HAVE_MPI
    unsigned long MyErrorCounter = ErrorCounter; ErrorCounter = 0;
    SU2_MPI::Allreduce(&MyErrorCounter, &ErrorCounter, 1, MPI_UNSIGNED_LONG, MPI_SUM, MPI_COMM_WORLD);
#endif
    if (iMesh == MESH_0) config->SetNonphysical_Points(ErrorCounter);
  }
  
}

void CPBIncEulerSolver::Postprocessing(CGeometry *geometry, CSolver **solver_container, CConfig *config,
                                  unsigned short iMesh) { 
									  
bool RightSol = true;
unsigned long iPoint, ErrorCounter = 0;

			  
  ErrorCounter = SetPrimitive_Variables(solver_container, config, true);
  
  
  /*--- Compute gradients to be used in Rhie Chow interpolation ---*/
  if (iMesh == MESH_0) {
    
    /*--- Gradient computation ---*/
    
    if (config->GetKind_Gradient_Method() == GREEN_GAUSS) {
      SetPrimitive_Gradient_GG(geometry, config);
    }
    if (config->GetKind_Gradient_Method() == WEIGHTED_LEAST_SQUARES) {
      SetPrimitive_Gradient_LS(geometry, config);
    }
    
  }
  
}


unsigned long CPBIncEulerSolver::SetPrimitive_Variables(CSolver **solver_container, CConfig *config, bool Output) {
  
  unsigned long iPoint, ErrorCounter = 0;
  unsigned short iVar;
  su2double pressure_val;
  
  bool RightSol = true;
  
  for (iPoint = 0; iPoint < nPoint; iPoint ++) {
    
    /*--- Initialize the non-physical points vector ---*/
    
    node[iPoint]->SetNon_Physical(false);
    
    /*--- PB Incompressible flow, primitive variables nDim+2, (P, vx, vy, vz, rho) ---*/
        
    RightSol = node[iPoint]->SetPrimVar(Density_Inf, config);
        
    if (!RightSol) { node[iPoint]->SetNon_Physical(true); ErrorCounter++; }
    
    /*--- Initialize the convective, source and viscous residual vector ---*/
    
    if (!Output) LinSysRes.SetBlock_Zero(iPoint);
    
    node[iPoint]->SetMassFluxZero();
    
  }
  
  return ErrorCounter;
}


void CPBIncEulerSolver::SetMax_Eigenvalue(CGeometry *geometry, CConfig *config) {
  
  su2double *Normal, Area, Mean_Speed = 0.0, Mean_ProjVel = 0.0,
  Lambda, Mean_DensityInc,
  ProjVel, ProjVel_i, ProjVel_j, *GridVel, *GridVel_i, *GridVel_j;
  
  unsigned long iEdge, iVertex, iPoint, jPoint;
  unsigned short iDim, iMarker;

  bool grid_movement = config->GetGrid_Movement();
  
  /*--- Set maximum inviscid eigenvalue to zero, and compute sound speed ---*/
  
  for (iPoint = 0; iPoint < nPointDomain; iPoint++) {
    node[iPoint]->SetLambda(0.0);
  }
  
  /*--- Loop interior edges ---*/
  
  for (iEdge = 0; iEdge < geometry->GetnEdge(); iEdge++) {
    
    /*--- Point identification, Normal vector and area ---*/
    
    iPoint = geometry->edge[iEdge]->GetNode(0);
    jPoint = geometry->edge[iEdge]->GetNode(1);
    
    Normal = geometry->edge[iEdge]->GetNormal();
    Area = 0.0;
    for (iDim = 0; iDim < nDim; iDim++) Area += Normal[iDim]*Normal[iDim];
    Area = sqrt(Area);
    
    /*--- Mean Values ---*/

    Mean_ProjVel    = 0.5 * (node[iPoint]->GetProjVel(Normal) + node[jPoint]->GetProjVel(Normal));
    Mean_DensityInc = 0.5 * (node[iPoint]->GetDensity() + node[jPoint]->GetDensity());
    Mean_Speed = Mean_ProjVel;

    /*--- Adjustment for grid movement ---*/
    
    if (grid_movement) {
      GridVel_i = geometry->node[iPoint]->GetGridVel();
      GridVel_j = geometry->node[jPoint]->GetGridVel();
      ProjVel_i = 0.0; ProjVel_j =0.0;
      for (iDim = 0; iDim < nDim; iDim++) {
        ProjVel_i += GridVel_i[iDim]*Normal[iDim];
        ProjVel_j += GridVel_j[iDim]*Normal[iDim];
      }
      Mean_ProjVel -= 0.5 * (ProjVel_i + ProjVel_j);
    }
    
    /*--- Inviscid contribution ---*/
    
    Lambda = fabs(Mean_ProjVel) + Mean_Speed;
    if (geometry->node[iPoint]->GetDomain()) node[iPoint]->AddLambda(Lambda);
    if (geometry->node[jPoint]->GetDomain()) node[jPoint]->AddLambda(Lambda);
    
  }
  
  /*--- Loop boundary edges ---*/
  
  for (iMarker = 0; iMarker < geometry->GetnMarker(); iMarker++) {
    for (iVertex = 0; iVertex < geometry->GetnVertex(iMarker); iVertex++) {
      
      /*--- Point identification, Normal vector and area ---*/
      
      iPoint = geometry->vertex[iMarker][iVertex]->GetNode();
      Normal = geometry->vertex[iMarker][iVertex]->GetNormal();
      Area = 0.0;
      for (iDim = 0; iDim < nDim; iDim++) Area += Normal[iDim]*Normal[iDim];
      Area = sqrt(Area);
      
      /*--- Mean Values ---*/
      
      Mean_ProjVel    = node[iPoint]->GetProjVel(Normal);
      Mean_DensityInc = node[iPoint]->GetDensity();
      Mean_Speed = Mean_ProjVel;
      
      /*--- Adjustment for grid movement ---*/
      
      if (grid_movement) {
        GridVel = geometry->node[iPoint]->GetGridVel();
        ProjVel = 0.0;
        for (iDim = 0; iDim < nDim; iDim++)
          ProjVel += GridVel[iDim]*Normal[iDim];
        Mean_ProjVel -= ProjVel;
      }
      
      /*--- Inviscid contribution ---*/
      
      Lambda = fabs(2.0*Mean_ProjVel);
      if (geometry->node[iPoint]->GetDomain()) {
        node[iPoint]->AddLambda(Lambda);
      }
      
    }
  }
  
  /*--- MPI parallelization ---*/
  
  Set_MPI_MaxEigenvalue(geometry, config);
  
}



void CPBIncEulerSolver::Set_MPI_MaxEigenvalue(CGeometry *geometry, CConfig *config) {
  unsigned short iMarker, MarkerS, MarkerR, *Buffer_Receive_Neighbor = NULL, *Buffer_Send_Neighbor = NULL;
  unsigned long iVertex, iPoint, nVertexS, nVertexR, nBufferS_Vector, nBufferR_Vector;
  su2double *Buffer_Receive_Lambda = NULL, *Buffer_Send_Lambda = NULL;
  
#ifdef HAVE_MPI
  int send_to, receive_from;
  SU2_MPI::Status status;
#endif
  
  for (iMarker = 0; iMarker < nMarker; iMarker++) {
    
    if ((config->GetMarker_All_KindBC(iMarker) == SEND_RECEIVE) &&
        (config->GetMarker_All_SendRecv(iMarker) > 0)) {
      
      MarkerS = iMarker;  MarkerR = iMarker+1;
      
#ifdef HAVE_MPI
      send_to = config->GetMarker_All_SendRecv(MarkerS)-1;
      receive_from = abs(config->GetMarker_All_SendRecv(MarkerR))-1;
#endif
      
      nVertexS = geometry->nVertex[MarkerS];  nVertexR = geometry->nVertex[MarkerR];
      nBufferS_Vector = nVertexS;        nBufferR_Vector = nVertexR;
      
      /*--- Allocate Receive and send buffers  ---*/
      Buffer_Receive_Lambda = new su2double [nBufferR_Vector];
      Buffer_Send_Lambda = new su2double[nBufferS_Vector];
      Buffer_Receive_Neighbor = new unsigned short [nBufferR_Vector];
      Buffer_Send_Neighbor = new unsigned short[nBufferS_Vector];
      
      /*--- Copy the solution old that should be sended ---*/
      for (iVertex = 0; iVertex < nVertexS; iVertex++) {
        iPoint = geometry->vertex[MarkerS][iVertex]->GetNode();
        Buffer_Send_Lambda[iVertex] = node[iPoint]->GetLambda();
        Buffer_Send_Neighbor[iVertex] = geometry->node[iPoint]->GetnPoint();
      }
      
#ifdef HAVE_MPI
      
      /*--- Send/Receive information using Sendrecv ---*/
      SU2_MPI::Sendrecv(Buffer_Send_Lambda, nBufferS_Vector, MPI_DOUBLE, send_to, 0,
                        Buffer_Receive_Lambda, nBufferR_Vector, MPI_DOUBLE, receive_from, 0, MPI_COMM_WORLD, &status);
      SU2_MPI::Sendrecv(Buffer_Send_Neighbor, nBufferS_Vector, MPI_UNSIGNED_SHORT, send_to, 1,
                        Buffer_Receive_Neighbor, nBufferR_Vector, MPI_UNSIGNED_SHORT, receive_from, 1, MPI_COMM_WORLD, &status);
      
#else
      
      /*--- Receive information without MPI ---*/
      for (iVertex = 0; iVertex < nVertexR; iVertex++) {
        Buffer_Receive_Lambda[iVertex] = Buffer_Send_Lambda[iVertex];
        Buffer_Receive_Neighbor[iVertex] = Buffer_Send_Neighbor[iVertex];
      }
      
#endif
      
      /*--- Deallocate send buffer ---*/
      delete [] Buffer_Send_Lambda;
      delete [] Buffer_Send_Neighbor;
      
      /*--- Do the coordinate transformation ---*/
      for (iVertex = 0; iVertex < nVertexR; iVertex++) {
        
        /*--- Find point and its type of transformation ---*/
        iPoint = geometry->vertex[MarkerR][iVertex]->GetNode();
        node[iPoint]->SetLambda(Buffer_Receive_Lambda[iVertex]);
        geometry->node[iPoint]->SetnNeighbor(Buffer_Receive_Neighbor[iVertex]);
        
      }
      
      /*--- Deallocate receive buffer ---*/
      delete [] Buffer_Receive_Lambda;
      delete [] Buffer_Receive_Neighbor;
      
    }
    
  }
}


void CPBIncEulerSolver::Set_MPI_Undivided_Laplacian(CGeometry *geometry, CConfig *config) {
  unsigned short iVar, iMarker, iPeriodic_Index, MarkerS, MarkerR;
  unsigned long iVertex, iPoint, nVertexS, nVertexR, nBufferS_Vector, nBufferR_Vector;
  su2double rotMatrix[3][3], *angles, theta, cosTheta, sinTheta, phi, cosPhi, sinPhi, psi, cosPsi, sinPsi,
  *Buffer_Receive_Undivided_Laplacian = NULL, *Buffer_Send_Undivided_Laplacian = NULL;
  
#ifdef HAVE_MPI
  int send_to, receive_from;
  SU2_MPI::Status status;
#endif
  
  for (iMarker = 0; iMarker < nMarker; iMarker++) {
    
    if ((config->GetMarker_All_KindBC(iMarker) == SEND_RECEIVE) &&
        (config->GetMarker_All_SendRecv(iMarker) > 0)) {
      
      MarkerS = iMarker;  MarkerR = iMarker+1;
      
#ifdef HAVE_MPI
      send_to = config->GetMarker_All_SendRecv(MarkerS)-1;
      receive_from = abs(config->GetMarker_All_SendRecv(MarkerR))-1;
#endif
      
      nVertexS = geometry->nVertex[MarkerS];  nVertexR = geometry->nVertex[MarkerR];
      nBufferS_Vector = nVertexS*nVar;        nBufferR_Vector = nVertexR*nVar;
      
      /*--- Allocate Receive and send buffers  ---*/
      Buffer_Receive_Undivided_Laplacian = new su2double [nBufferR_Vector];
      Buffer_Send_Undivided_Laplacian = new su2double[nBufferS_Vector];
      
      /*--- Copy the solution old that should be sended ---*/
      for (iVertex = 0; iVertex < nVertexS; iVertex++) {
        iPoint = geometry->vertex[MarkerS][iVertex]->GetNode();
        for (iVar = 0; iVar < nVar; iVar++)
          Buffer_Send_Undivided_Laplacian[iVar*nVertexS+iVertex] = node[iPoint]->GetUndivided_Laplacian(iVar);
      }
      
#ifdef HAVE_MPI
      
      /*--- Send/Receive information using Sendrecv ---*/
      SU2_MPI::Sendrecv(Buffer_Send_Undivided_Laplacian, nBufferS_Vector, MPI_DOUBLE, send_to, 0,
                        Buffer_Receive_Undivided_Laplacian, nBufferR_Vector, MPI_DOUBLE, receive_from, 0, MPI_COMM_WORLD, &status);
      
#else
      
      /*--- Receive information without MPI ---*/
      for (iVertex = 0; iVertex < nVertexR; iVertex++) {
        for (iVar = 0; iVar < nVar; iVar++)
          Buffer_Receive_Undivided_Laplacian[iVar*nVertexR+iVertex] = Buffer_Send_Undivided_Laplacian[iVar*nVertexR+iVertex];
      }
      
#endif
      
      /*--- Deallocate send buffer ---*/
      delete [] Buffer_Send_Undivided_Laplacian;
      
      /*--- Do the coordinate transformation ---*/
      for (iVertex = 0; iVertex < nVertexR; iVertex++) {
        
        /*--- Find point and its type of transformation ---*/
        iPoint = geometry->vertex[MarkerR][iVertex]->GetNode();
        iPeriodic_Index = geometry->vertex[MarkerR][iVertex]->GetRotation_Type();
        
        /*--- Retrieve the supplied periodic information. ---*/
        angles = config->GetPeriodicRotation(iPeriodic_Index);
        
        /*--- Store angles separately for clarity. ---*/
        theta    = angles[0];   phi    = angles[1];     psi    = angles[2];
        cosTheta = cos(theta);  cosPhi = cos(phi);      cosPsi = cos(psi);
        sinTheta = sin(theta);  sinPhi = sin(phi);      sinPsi = sin(psi);
        
        /*--- Compute the rotation matrix. Note that the implicit
         ordering is rotation about the x-axis, y-axis,
         then z-axis. Note that this is the transpose of the matrix
         used during the preprocessing stage. ---*/
        rotMatrix[0][0] = cosPhi*cosPsi;    rotMatrix[1][0] = sinTheta*sinPhi*cosPsi - cosTheta*sinPsi;     rotMatrix[2][0] = cosTheta*sinPhi*cosPsi + sinTheta*sinPsi;
        rotMatrix[0][1] = cosPhi*sinPsi;    rotMatrix[1][1] = sinTheta*sinPhi*sinPsi + cosTheta*cosPsi;     rotMatrix[2][1] = cosTheta*sinPhi*sinPsi - sinTheta*cosPsi;
        rotMatrix[0][2] = -sinPhi;          rotMatrix[1][2] = sinTheta*cosPhi;                              rotMatrix[2][2] = cosTheta*cosPhi;
        
        /*--- Copy conserved variables before performing transformation. ---*/
        for (iVar = 0; iVar < nVar; iVar++)
          Solution[iVar] = Buffer_Receive_Undivided_Laplacian[iVar*nVertexR+iVertex];
        
        /*--- Rotate the momentum components. ---*/
        if (nDim == 2) {
          Solution[1] = rotMatrix[0][0]*Buffer_Receive_Undivided_Laplacian[1*nVertexR+iVertex] +
          rotMatrix[0][1]*Buffer_Receive_Undivided_Laplacian[2*nVertexR+iVertex];
          Solution[2] = rotMatrix[1][0]*Buffer_Receive_Undivided_Laplacian[1*nVertexR+iVertex] +
          rotMatrix[1][1]*Buffer_Receive_Undivided_Laplacian[2*nVertexR+iVertex];
        }
        else {
          Solution[1] = rotMatrix[0][0]*Buffer_Receive_Undivided_Laplacian[1*nVertexR+iVertex] +
          rotMatrix[0][1]*Buffer_Receive_Undivided_Laplacian[2*nVertexR+iVertex] +
          rotMatrix[0][2]*Buffer_Receive_Undivided_Laplacian[3*nVertexR+iVertex];
          Solution[2] = rotMatrix[1][0]*Buffer_Receive_Undivided_Laplacian[1*nVertexR+iVertex] +
          rotMatrix[1][1]*Buffer_Receive_Undivided_Laplacian[2*nVertexR+iVertex] +
          rotMatrix[1][2]*Buffer_Receive_Undivided_Laplacian[3*nVertexR+iVertex];
          Solution[3] = rotMatrix[2][0]*Buffer_Receive_Undivided_Laplacian[1*nVertexR+iVertex] +
          rotMatrix[2][1]*Buffer_Receive_Undivided_Laplacian[2*nVertexR+iVertex] +
          rotMatrix[2][2]*Buffer_Receive_Undivided_Laplacian[3*nVertexR+iVertex];
        }
        
        /*--- Copy transformed conserved variables back into buffer. ---*/
        for (iVar = 0; iVar < nVar; iVar++)
          node[iPoint]->SetUndivided_Laplacian(iVar, Solution[iVar]);
        
      }
      
      /*--- Deallocate receive buffer ---*/
      delete [] Buffer_Receive_Undivided_Laplacian;
      
    }
    
  }
  
}

void CPBIncEulerSolver::SetUndivided_Laplacian(CGeometry *geometry, CConfig *config) {
  
  unsigned long iPoint, jPoint, iEdge;
  su2double *Diff;
  unsigned short iVar;
  bool boundary_i, boundary_j;
  
  Diff = new su2double[nVar];
  
  for (iPoint = 0; iPoint < nPointDomain; iPoint++)
    node[iPoint]->SetUnd_LaplZero();
  
  for (iEdge = 0; iEdge < geometry->GetnEdge(); iEdge++) {
    
    iPoint = geometry->edge[iEdge]->GetNode(0);
    jPoint = geometry->edge[iEdge]->GetNode(1);
    
    /*--- Solution differences ---*/
    
    for (iVar = 0; iVar < nVar; iVar++)
      Diff[iVar] = node[iPoint]->GetSolution(iVar) - node[jPoint]->GetSolution(iVar);
    
    boundary_i = geometry->node[iPoint]->GetPhysicalBoundary();
    boundary_j = geometry->node[jPoint]->GetPhysicalBoundary();
    
    /*--- Both points inside the domain, or both in the boundary ---*/
    
    if ((!boundary_i && !boundary_j) || (boundary_i && boundary_j)) {
      if (geometry->node[iPoint]->GetDomain()) node[iPoint]->SubtractUnd_Lapl(Diff);
      if (geometry->node[jPoint]->GetDomain()) node[jPoint]->AddUnd_Lapl(Diff);
    }
    
    /*--- iPoint inside the domain, jPoint on the boundary ---*/
    
    if (!boundary_i && boundary_j)
      if (geometry->node[iPoint]->GetDomain()) node[iPoint]->SubtractUnd_Lapl(Diff);
    
    /*--- jPoint inside the domain, iPoint on the boundary ---*/
    
    if (boundary_i && !boundary_j)
      if (geometry->node[jPoint]->GetDomain()) node[jPoint]->AddUnd_Lapl(Diff);
    
  }
  
  /*--- MPI parallelization ---*/
  
  Set_MPI_Undivided_Laplacian(geometry, config);
  
  delete [] Diff;
  
}



void CPBIncEulerSolver::LoadRestart(CGeometry **geometry, CSolver ***solver, CConfig *config, int val_iter, bool val_update_geo) {
  
  /*--- Restart the solution from file information ---*/
  unsigned short iDim, iVar, iMesh, iMeshFine;
  unsigned long iPoint, index, iChildren, Point_Fine;
  unsigned short turb_model = config->GetKind_Turb_Model();
  su2double Area_Children, Area_Parent, *Coord, *Solution_Fine;
  bool grid_movement  = config->GetGrid_Movement();
  bool static_fsi = ((config->GetUnsteady_Simulation() == STEADY) &&
                     (config->GetFSI_Simulation()));
  bool dual_time = ((config->GetUnsteady_Simulation() == DT_STEPPING_1ST) ||
                    (config->GetUnsteady_Simulation() == DT_STEPPING_2ND));
  bool steady_restart = config->GetSteadyRestart();
  bool time_stepping = config->GetUnsteady_Simulation() == TIME_STEPPING;
  string UnstExt, text_line;
  ifstream restart_file;
  
  unsigned short iZone = config->GetiZone();
  unsigned short nZone = config->GetnZone();

  string restart_filename = config->GetSolution_FlowFileName();

  Coord = new su2double [nDim];
  for (iDim = 0; iDim < nDim; iDim++)
    Coord[iDim] = 0.0;
  
  int counter = 0;
  long iPoint_Local = 0; unsigned long iPoint_Global = 0;
  unsigned long iPoint_Global_Local = 0;
  unsigned short rbuf_NotMatching = 0, sbuf_NotMatching = 0;

  /*--- Skip coordinates ---*/

  unsigned short skipVars = geometry[MESH_0]->GetnDim();
  
  /*--- Multizone problems require the number of the zone to be appended. ---*/

  if (nZone > 1)
  restart_filename = config->GetMultizone_FileName(restart_filename, iZone);

  /*--- Modify file name for an unsteady restart ---*/
  
  if (dual_time || time_stepping)
    restart_filename = config->GetUnsteady_FileName(restart_filename, val_iter);

  /*--- Read the restart data from either an ASCII or binary SU2 file. ---*/

  if (config->GetRead_Binary_Restart()) {
    Read_SU2_Restart_Binary(geometry[MESH_0], config, restart_filename);
  } else {
    Read_SU2_Restart_ASCII(geometry[MESH_0], config, restart_filename);
  }

  /*--- Load data from the restart into correct containers. ---*/

  counter = 0;
  for (iPoint_Global = 0; iPoint_Global < geometry[MESH_0]->GetGlobal_nPointDomain(); iPoint_Global++ ) {

    /*--- Retrieve local index. If this node from the restart file lives
     on the current processor, we will load and instantiate the vars. ---*/

    iPoint_Local = geometry[MESH_0]->GetGlobal_to_Local_Point(iPoint_Global);

    if (iPoint_Local > -1) {

      /*--- We need to store this point's data, so jump to the correct
       offset in the buffer of data from the restart file and load it. ---*/

      index = counter*Restart_Vars[1] + skipVars;
      node[iPoint_Local]->SetPressure_val(Restart_Data[index]);
      for (iVar = 1; iVar < nVar; iVar++) Solution[iVar-1] = Restart_Data[index+iVar];
      node[iPoint_Local]->SetSolution(Solution);
      iPoint_Global_Local++;

      /*--- For dynamic meshes, read in and store the
       grid coordinates and grid velocities for each node. ---*/

      if (grid_movement) {

        /*--- First, remove any variables for the turbulence model that
         appear in the restart file before the grid velocities. ---*/

        if (turb_model == SA || turb_model == SA_NEG) {
          index++;
        } else if (turb_model == SST) {
          index+=2;
        }

        /*--- Read in the next 2 or 3 variables which are the grid velocities ---*/
        /*--- If we are restarting the solution from a previously computed static calculation (no grid movement) ---*/
        /*--- the grid velocities are set to 0. This is useful for FSI computations ---*/

        su2double GridVel[3] = {0.0,0.0,0.0};
        if (!steady_restart) {

          /*--- Rewind the index to retrieve the Coords. ---*/
          index = counter*Restart_Vars[1];
          for (iDim = 0; iDim < nDim; iDim++) { Coord[iDim] = Restart_Data[index+iDim]; }

          /*--- Move the index forward to get the grid velocities. ---*/
          index = counter*Restart_Vars[1] + skipVars + nVar;
          for (iDim = 0; iDim < nDim; iDim++) { GridVel[iDim] = Restart_Data[index+iDim]; }
        }

        for (iDim = 0; iDim < nDim; iDim++) {
          geometry[MESH_0]->node[iPoint_Local]->SetCoord(iDim, Coord[iDim]);
          geometry[MESH_0]->node[iPoint_Local]->SetGridVel(iDim, GridVel[iDim]);
        }
      }
      

      /*--- For static FSI problems, grid_movement is 0 but we need to read in and store the
       grid coordinates for each node (but not the grid velocities, as there are none). ---*/

      if (static_fsi && val_update_geo) {
       /*--- Rewind the index to retrieve the Coords. ---*/
        index = counter*Restart_Vars[1];
        for (iDim = 0; iDim < nDim; iDim++) { Coord[iDim] = Restart_Data[index+iDim];}

        for (iDim = 0; iDim < nDim; iDim++) {
          geometry[MESH_0]->node[iPoint_Local]->SetCoord(iDim, Coord[iDim]);
        }
      }

      /*--- Increment the overall counter for how many points have been loaded. ---*/
      counter++;
      
    }
  }

  /*--- Detect a wrong solution file ---*/

  if (iPoint_Global_Local < nPointDomain) { sbuf_NotMatching = 1; }

#ifndef HAVE_MPI
  rbuf_NotMatching = sbuf_NotMatching;
#else
  SU2_MPI::Allreduce(&sbuf_NotMatching, &rbuf_NotMatching, 1, MPI_UNSIGNED_SHORT, MPI_SUM, MPI_COMM_WORLD);
#endif
  if (rbuf_NotMatching != 0) {
    SU2_MPI::Error(string("The solution file ") + restart_filename + string(" doesn't match with the mesh file!\n") +
                   string("It could be empty lines at the end of the file."), CURRENT_FUNCTION);
  }
  
  /*--- Communicate the loaded solution on the fine grid before we transfer
   it down to the coarse levels. We alo call the preprocessing routine
   on the fine level in order to have all necessary quantities updated,
   especially if this is a turbulent simulation (eddy viscosity). ---*/
  
  solver[MESH_0][FLOW_SOL]->Set_MPI_Solution(geometry[MESH_0], config);
  solver[MESH_0][FLOW_SOL]->Preprocessing(geometry[MESH_0], solver[MESH_0], config, MESH_0, NO_RK_ITER, RUNTIME_FLOW_SYS, false);

  /*--- Interpolate the solution down to the coarse multigrid levels ---*/
  
  for (iMesh = 1; iMesh <= config->GetnMGLevels(); iMesh++) {
    for (iPoint = 0; iPoint < geometry[iMesh]->GetnPoint(); iPoint++) {
      Area_Parent = geometry[iMesh]->node[iPoint]->GetVolume();
      for (iVar = 0; iVar < nVar; iVar++) Solution[iVar] = 0.0;
      for (iChildren = 0; iChildren < geometry[iMesh]->node[iPoint]->GetnChildren_CV(); iChildren++) {
        Point_Fine = geometry[iMesh]->node[iPoint]->GetChildren_CV(iChildren);
        Area_Children = geometry[iMesh-1]->node[Point_Fine]->GetVolume();
        Solution_Fine = solver[iMesh-1][FLOW_SOL]->node[Point_Fine]->GetSolution();
        for (iVar = 0; iVar < nVar; iVar++) {
          Solution[iVar] += Solution_Fine[iVar]*Area_Children/Area_Parent;
        }
      }
      solver[iMesh][FLOW_SOL]->node[iPoint]->SetSolution(Solution);
    }
    solver[iMesh][FLOW_SOL]->Set_MPI_Solution(geometry[iMesh], config);
    solver[iMesh][FLOW_SOL]->Preprocessing(geometry[iMesh], solver[iMesh], config, iMesh, NO_RK_ITER, RUNTIME_FLOW_SYS, false);

  }
  
  /*--- Update the geometry for flows on dynamic meshes ---*/
  
  if (grid_movement) {
    
    /*--- Communicate the new coordinates and grid velocities at the halos ---*/
    
    geometry[MESH_0]->Set_MPI_Coord(config);
    geometry[MESH_0]->Set_MPI_GridVel(config);
    
    /*--- Recompute the edges and  dual mesh control volumes in the
     domain and on the boundaries. ---*/
    
    geometry[MESH_0]->SetCoord_CG();
    geometry[MESH_0]->SetControlVolume(config, UPDATE);
    geometry[MESH_0]->SetBoundControlVolume(config, UPDATE);
    
    /*--- Update the multigrid structure after setting up the finest grid,
     including computing the grid velocities on the coarser levels. ---*/
    
    for (iMesh = 1; iMesh <= config->GetnMGLevels(); iMesh++) {
      iMeshFine = iMesh-1;
      geometry[iMesh]->SetControlVolume(config, geometry[iMeshFine], UPDATE);
      geometry[iMesh]->SetBoundControlVolume(config, geometry[iMeshFine],UPDATE);
      geometry[iMesh]->SetCoord(geometry[iMeshFine]);
      geometry[iMesh]->SetRestricted_GridVelocity(geometry[iMeshFine], config);
    }
  }
  
  /*--- Update the geometry for flows on static FSI problems with moving meshes ---*/
  
  if (static_fsi && val_update_geo) {
    
    /*--- Communicate the new coordinates and grid velocities at the halos ---*/
    
    geometry[MESH_0]->Set_MPI_Coord(config);
    
    /*--- Recompute the edges and  dual mesh control volumes in the
     domain and on the boundaries. ---*/
    
    geometry[MESH_0]->SetCoord_CG();
    geometry[MESH_0]->SetControlVolume(config, UPDATE);
    geometry[MESH_0]->SetBoundControlVolume(config, UPDATE);
    
    /*--- Update the multigrid structure after setting up the finest grid,
     including computing the grid velocities on the coarser levels. ---*/
    
    for (iMesh = 1; iMesh <= config->GetnMGLevels(); iMesh++) {
      iMeshFine = iMesh-1;
      geometry[iMesh]->SetControlVolume(config, geometry[iMeshFine], UPDATE);
      geometry[iMesh]->SetBoundControlVolume(config, geometry[iMeshFine],UPDATE);
      geometry[iMesh]->SetCoord(geometry[iMeshFine]);
    }
  }
  
  delete [] Coord;

  /*--- Delete the class memory that is used to load the restart. ---*/

  if (Restart_Vars != NULL) delete [] Restart_Vars;
  if (Restart_Data != NULL) delete [] Restart_Data;
  Restart_Vars = NULL; Restart_Data = NULL;
  
}





void CPBIncEulerSolver::SetCentered_Dissipation_Sensor(CGeometry *geometry, CConfig *config) {
  
  unsigned long iEdge, iPoint, jPoint;
  su2double Pressure_i = 0.0, Pressure_j = 0.0;
  bool boundary_i, boundary_j;
  
  /*--- Reset variables to store the undivided pressure ---*/
  
  for (iPoint = 0; iPoint < nPointDomain; iPoint++) {
    iPoint_UndLapl[iPoint] = 0.0;
    jPoint_UndLapl[iPoint] = 0.0;
  }
  
  /*--- Evaluate the pressure sensor ---*/
  
  for (iEdge = 0; iEdge < geometry->GetnEdge(); iEdge++) {
    
    iPoint = geometry->edge[iEdge]->GetNode(0);
    jPoint = geometry->edge[iEdge]->GetNode(1);
    
    /*--- Get the pressure, or density for incompressible solvers ---*/

    Pressure_i = node[iPoint]->GetDensity();
    Pressure_j = node[jPoint]->GetDensity();

    boundary_i = geometry->node[iPoint]->GetPhysicalBoundary();
    boundary_j = geometry->node[jPoint]->GetPhysicalBoundary();
    
    /*--- Both points inside the domain, or both on the boundary ---*/
    
    if ((!boundary_i && !boundary_j) || (boundary_i && boundary_j)) {
      
      if (geometry->node[iPoint]->GetDomain()) {
        iPoint_UndLapl[iPoint] += (Pressure_j - Pressure_i);
        jPoint_UndLapl[iPoint] += (Pressure_i + Pressure_j);
      }
      
      if (geometry->node[jPoint]->GetDomain()) {
        iPoint_UndLapl[jPoint] += (Pressure_i - Pressure_j);
        jPoint_UndLapl[jPoint] += (Pressure_i + Pressure_j);
      }
      
    }
    
    /*--- iPoint inside the domain, jPoint on the boundary ---*/
    
    if (!boundary_i && boundary_j)
      if (geometry->node[iPoint]->GetDomain()) {
        iPoint_UndLapl[iPoint] += (Pressure_j - Pressure_i);
        jPoint_UndLapl[iPoint] += (Pressure_i + Pressure_j);
      }
    
    /*--- jPoint inside the domain, iPoint on the boundary ---*/
    
    if (boundary_i && !boundary_j)
      if (geometry->node[jPoint]->GetDomain()) {
        iPoint_UndLapl[jPoint] += (Pressure_i - Pressure_j);
        jPoint_UndLapl[jPoint] += (Pressure_i + Pressure_j);
      }
    
  }
  
  /*--- Set pressure switch for each point ---*/
  
  for (iPoint = 0; iPoint < nPointDomain; iPoint++)
    node[iPoint]->SetSensor(fabs(iPoint_UndLapl[iPoint]) / jPoint_UndLapl[iPoint]);
  
  /*--- MPI parallelization ---*/
  
  Set_MPI_Sensor(geometry, config);
  
}



void CPBIncEulerSolver::SetNondimensionalization(CGeometry *geometry, CConfig *config, unsigned short iMesh) {
 su2double Temperature_FreeStream = 0.0,  ModVel_FreeStream = 0.0,Energy_FreeStream = 0.0,
  ModVel_FreeStreamND = 0.0, Omega_FreeStream = 0.0, Omega_FreeStreamND = 0.0, Viscosity_FreeStream = 0.0,
  Density_FreeStream = 0.0, Pressure_FreeStream = 0.0, Pressure_Thermodynamic = 0.0, Tke_FreeStream = 0.0,
  Length_Ref = 0.0, Density_Ref = 0.0, Pressure_Ref = 0.0, Temperature_Ref = 0.0, Velocity_Ref = 0.0, Time_Ref = 0.0,
  Gas_Constant_Ref = 0.0, Omega_Ref = 0.0, Force_Ref = 0.0, Viscosity_Ref = 0.0, Conductivity_Ref = 0.0, Heat_Flux_Ref = 0.0, Energy_Ref= 0.0, Pressure_FreeStreamND = 0.0, Pressure_ThermodynamicND = 0.0, Density_FreeStreamND = 0.0,
  Temperature_FreeStreamND = 0.0, Gas_ConstantND = 0.0, Specific_Heat_CpND = 0.0, Specific_Heat_CvND = 0.0, Thermal_Expansion_CoeffND = 0.0,
  Velocity_FreeStreamND[3] = {0.0, 0.0, 0.0}, Viscosity_FreeStreamND = 0.0,
  Tke_FreeStreamND = 0.0, Energy_FreeStreamND = 0.0,
  Total_UnstTimeND = 0.0, Delta_UnstTimeND = 0.0;
  
  unsigned short iDim;
  
  /*--- Local variables ---*/
  
  su2double Mach     = config->GetMach();
  su2double Reynolds = config->GetReynolds();
  
  bool unsteady      = (config->GetUnsteady_Simulation() != NO);
  bool viscous       = config->GetViscous();
  bool grid_movement = config->GetGrid_Movement();
  bool turbulent     = ((config->GetKind_Solver() == RANS) ||
                        (config->GetKind_Solver() == DISC_ADJ_RANS));
  bool tkeNeeded     = ((turbulent) && (config->GetKind_Turb_Model() == SST));
  bool energy        = config->GetEnergy_Equation();
  bool boussinesq    = (config->GetKind_DensityModel() == BOUSSINESQ);

  /*--- Compute dimensional free-stream values. ---*/

  Density_FreeStream     = config->GetInc_Density_Init();     config->SetDensity_FreeStream(Density_FreeStream);
  Temperature_FreeStream = config->GetInc_Temperature_Init(); config->SetTemperature_FreeStream(Temperature_FreeStream);
  Pressure_FreeStream    = 0.0; config->SetPressure_FreeStream(Pressure_FreeStream);

  ModVel_FreeStream   = 0.0;
  for (iDim = 0; iDim < nDim; iDim++) {
    ModVel_FreeStream += config->GetInc_Velocity_Init()[iDim]*config->GetInc_Velocity_Init()[iDim];
    config->SetVelocity_FreeStream(config->GetInc_Velocity_Init()[iDim],iDim);
  }
  ModVel_FreeStream = sqrt(ModVel_FreeStream); config->SetModVel_FreeStream(ModVel_FreeStream);

  /*--- Depending on the density model chosen, select a fluid model. ---*/

  switch (config->GetKind_FluidModel()) {

    case CONSTANT_DENSITY:

      FluidModel = new CConstantDensity(Density_FreeStream, config->GetSpecific_Heat_Cp());
      FluidModel->SetTDState_T(Temperature_FreeStream);
      break;

    case INC_IDEAL_GAS:

      config->SetGas_Constant(UNIVERSAL_GAS_CONSTANT/(config->GetMolecular_Weight()/1000.0));
      Pressure_Thermodynamic = Density_FreeStream*Temperature_FreeStream*config->GetGas_Constant();
      FluidModel = new CIncIdealGas(config->GetSpecific_Heat_Cp(), config->GetGas_Constant(), Pressure_Thermodynamic);
      FluidModel->SetTDState_T(Temperature_FreeStream);
      Pressure_Thermodynamic = FluidModel->GetPressure();
      config->SetPressure_Thermodynamic(Pressure_Thermodynamic);
      break;

    default:

      SU2_MPI::Error("Fluid model not implemented for incompressible solver.", CURRENT_FUNCTION);
      break;
  }

  if (viscous) {

    /*--- The dimensional viscosity is needed to determine the free-stream conditions.
      To accomplish this, simply set the non-dimensional coefficients to the
      dimensional ones. This will be overruled later.---*/

    config->SetMu_RefND(config->GetMu_Ref());
    config->SetMu_Temperature_RefND(config->GetMu_Temperature_Ref());
    config->SetMu_SND(config->GetMu_S());
    config->SetMu_ConstantND(config->GetMu_Constant());

    /*--- Use the fluid model to compute the dimensional viscosity/conductivity. ---*/

    FluidModel->SetLaminarViscosityModel(config);
    Viscosity_FreeStream = FluidModel->GetLaminarViscosity();
    config->SetViscosity_FreeStream(Viscosity_FreeStream);

    Reynolds = Density_FreeStream*ModVel_FreeStream/Viscosity_FreeStream; config->SetReynolds(Reynolds);

    /*--- Turbulence kinetic energy ---*/

    Tke_FreeStream  = 3.0/2.0*(ModVel_FreeStream*ModVel_FreeStream*config->GetTurbulenceIntensity_FreeStream()*config->GetTurbulenceIntensity_FreeStream());

  }

  /*--- The non-dim. scheme for incompressible flows uses the following ref. values:
     Reference length      = 1 m (fixed by default, grid in meters)
     Reference density     = liquid density or freestream (input)
     Reference velocity    = liquid velocity or freestream (input)
     Reference temperature = liquid temperature or freestream (input)
     Reference pressure    = Reference density * Reference velocity * Reference velocity
     Reference viscosity   = Reference Density * Reference velocity * Reference length
     This is the same non-dim. scheme as in the compressible solver.
     Note that the Re and Re Length are not used as part of initialization. ---*/

  if (config->GetRef_Inc_NonDim() == DIMENSIONAL) {
    Density_Ref     = 1.0;
    Velocity_Ref    = 1.0;
    Temperature_Ref = 1.0;
    Pressure_Ref    = 1.0;
  }
  else if (config->GetRef_Inc_NonDim() == INITIAL_VALUES) {
    Density_Ref     = Density_FreeStream;
    Velocity_Ref    = ModVel_FreeStream;
    Temperature_Ref = Temperature_FreeStream;
    Pressure_Ref    = Density_Ref*Velocity_Ref*Velocity_Ref;
  } 
  else if (config->GetRef_Inc_NonDim() == REFERENCE_VALUES) {
    Density_Ref     = config->GetInc_Density_Ref();
    Velocity_Ref    = config->GetInc_Velocity_Ref();
    Temperature_Ref = config->GetInc_Temperature_Ref();
    Pressure_Ref    = Density_Ref*Velocity_Ref*Velocity_Ref;
  }
  config->SetDensity_Ref(Density_Ref);
  config->SetVelocity_Ref(Velocity_Ref);
  config->SetTemperature_Ref(Temperature_Ref);
  config->SetPressure_Ref(Pressure_Ref);

  /*--- More derived reference values ---*/
  
  Length_Ref       = 1.0;                                                config->SetLength_Ref(Length_Ref);
  Time_Ref         = Length_Ref/Velocity_Ref;                            config->SetTime_Ref(Time_Ref);
  Omega_Ref        = Velocity_Ref/Length_Ref;                            config->SetOmega_Ref(Omega_Ref);
  Force_Ref        = Velocity_Ref*Velocity_Ref/Length_Ref;               config->SetForce_Ref(Force_Ref);
  Heat_Flux_Ref    = Density_Ref*Velocity_Ref*Velocity_Ref*Velocity_Ref; config->SetHeat_Flux_Ref(Heat_Flux_Ref);
  Gas_Constant_Ref = Velocity_Ref*Velocity_Ref/Temperature_Ref;          config->SetGas_Constant_Ref(Gas_Constant_Ref);
  Viscosity_Ref    = Density_Ref*Velocity_Ref*Length_Ref;                config->SetViscosity_Ref(Viscosity_Ref);
  Conductivity_Ref = Viscosity_Ref*Gas_Constant_Ref;                     config->SetConductivity_Ref(Conductivity_Ref);

  /*--- Get the freestream energy. Only useful if energy equation is active. ---*/

  Energy_FreeStream = FluidModel->GetStaticEnergy() + 0.5*ModVel_FreeStream*ModVel_FreeStream;
  config->SetEnergy_FreeStream(Energy_FreeStream);
  if (tkeNeeded) { Energy_FreeStream += Tke_FreeStream; }; config->SetEnergy_FreeStream(Energy_FreeStream);

  /*--- Compute Mach number ---*/

  if (config->GetKind_FluidModel() == CONSTANT_DENSITY) {
    Mach = ModVel_FreeStream / sqrt(config->GetBulk_Modulus()/Density_FreeStream);
  } else {
    Mach = 0.0;
  }
  config->SetMach(Mach);

  /*--- Divide by reference values, to compute the non-dimensional free-stream values ---*/
  
  Pressure_FreeStreamND = Pressure_FreeStream/config->GetPressure_Ref(); config->SetPressure_FreeStreamND(Pressure_FreeStreamND);
  Pressure_ThermodynamicND = Pressure_Thermodynamic/config->GetPressure_Ref(); config->SetPressure_ThermodynamicND(Pressure_ThermodynamicND);
  Density_FreeStreamND  = Density_FreeStream/config->GetDensity_Ref();   config->SetDensity_FreeStreamND(Density_FreeStreamND);
  
  for (iDim = 0; iDim < nDim; iDim++) {
    Velocity_FreeStreamND[iDim] = config->GetVelocity_FreeStream()[iDim]/Velocity_Ref; config->SetVelocity_FreeStreamND(Velocity_FreeStreamND[iDim], iDim);
  }

  Temperature_FreeStreamND = Temperature_FreeStream/config->GetTemperature_Ref(); config->SetTemperature_FreeStreamND(Temperature_FreeStreamND);
  Gas_ConstantND      = config->GetGas_Constant()/Gas_Constant_Ref;    config->SetGas_ConstantND(Gas_ConstantND);
  Specific_Heat_CpND  = config->GetSpecific_Heat_Cp()/Gas_Constant_Ref; config->SetSpecific_Heat_CpND(Specific_Heat_CpND);
  
  /*--- We assume that Cp = Cv for our incompressible fluids. ---*/
  Specific_Heat_CvND  = config->GetSpecific_Heat_Cp()/Gas_Constant_Ref; config->SetSpecific_Heat_CvND(Specific_Heat_CvND);
  
  Thermal_Expansion_CoeffND = config->GetThermal_Expansion_Coeff()*config->GetTemperature_Ref(); config->SetThermal_Expansion_CoeffND(Thermal_Expansion_CoeffND);

  ModVel_FreeStreamND = 0.0;
  for (iDim = 0; iDim < nDim; iDim++) ModVel_FreeStreamND += Velocity_FreeStreamND[iDim]*Velocity_FreeStreamND[iDim];
  ModVel_FreeStreamND    = sqrt(ModVel_FreeStreamND); config->SetModVel_FreeStreamND(ModVel_FreeStreamND);
  
  Viscosity_FreeStreamND = Viscosity_FreeStream / Viscosity_Ref;   config->SetViscosity_FreeStreamND(Viscosity_FreeStreamND);
  
  Tke_FreeStream  = 3.0/2.0*(ModVel_FreeStream*ModVel_FreeStream*config->GetTurbulenceIntensity_FreeStream()*config->GetTurbulenceIntensity_FreeStream());
  config->SetTke_FreeStream(Tke_FreeStream);
  
  Tke_FreeStreamND  = 3.0/2.0*(ModVel_FreeStreamND*ModVel_FreeStreamND*config->GetTurbulenceIntensity_FreeStream()*config->GetTurbulenceIntensity_FreeStream());
  config->SetTke_FreeStreamND(Tke_FreeStreamND);
  
  Omega_FreeStream = Density_FreeStream*Tke_FreeStream/(Viscosity_FreeStream*config->GetTurb2LamViscRatio_FreeStream());
  config->SetOmega_FreeStream(Omega_FreeStream);
  
  Omega_FreeStreamND = Density_FreeStreamND*Tke_FreeStreamND/(Viscosity_FreeStreamND*config->GetTurb2LamViscRatio_FreeStream());
  config->SetOmega_FreeStreamND(Omega_FreeStreamND);
 
  /*--- Delete the original (dimensional) FluidModel object. No fluid is used for inscompressible cases. ---*/
  
  delete FluidModel;

  switch (config->GetKind_FluidModel()) {
      
    case CONSTANT_DENSITY:
      FluidModel = new CConstantDensity(Density_FreeStreamND, Specific_Heat_CpND);
      FluidModel->SetTDState_T(Temperature_FreeStreamND);
      break;

    case INC_IDEAL_GAS:
      FluidModel = new CIncIdealGas(Specific_Heat_CpND, Gas_ConstantND, Pressure_ThermodynamicND);
      FluidModel->SetTDState_T(Temperature_FreeStreamND);
      break;
      
  }
  
  Energy_FreeStreamND = FluidModel->GetStaticEnergy() + 0.5*ModVel_FreeStreamND*ModVel_FreeStreamND;
  
  if (viscous) {
    
    /*--- Constant viscosity model ---*/

    config->SetMu_ConstantND(config->GetMu_Constant()/Viscosity_Ref);
    
    /*--- Sutherland's model ---*/    
    config->SetMu_RefND(config->GetMu_Ref()/Viscosity_Ref);
    config->SetMu_SND(config->GetMu_S()/config->GetTemperature_Ref());
    config->SetMu_Temperature_RefND(config->GetMu_Temperature_Ref()/config->GetTemperature_Ref());
    
    /*--- Constant thermal conductivity model ---*/

    config->SetKt_ConstantND(config->GetKt_Constant()/Conductivity_Ref);
    
    /*--- Set up the transport property models. ---*/

    FluidModel->SetLaminarViscosityModel(config);
    FluidModel->SetThermalConductivityModel(config);
    
  }

  if (tkeNeeded) { Energy_FreeStreamND += Tke_FreeStreamND; };  config->SetEnergy_FreeStreamND(Energy_FreeStreamND);
  
  Energy_Ref = Energy_FreeStream/Energy_FreeStreamND; config->SetEnergy_Ref(Energy_Ref);
  
  Total_UnstTimeND = config->GetTotal_UnstTime() / Time_Ref;    config->SetTotal_UnstTimeND(Total_UnstTimeND);
  Delta_UnstTimeND = config->GetDelta_UnstTime() / Time_Ref;    config->SetDelta_UnstTimeND(Delta_UnstTimeND);
  
  /*--- Write output to the console if this is the master node and first domain ---*/
  
  if ((rank == MASTER_NODE) && (iMesh == MESH_0)) {
    
    cout.precision(6);

    if (config->GetRef_Inc_NonDim() == DIMENSIONAL) {
      cout << "Incompressible flow: rho_ref, vel_ref, temp_ref, p_ref" << endl;
      cout << "are set to 1.0 in order to perform a dimensional calculation." << endl;
      if (grid_movement) cout << "Force coefficients computed using MACH_MOTION." << endl;
      else cout << "Force coefficients computed using initial values." << endl;
    }
    else if (config->GetRef_Inc_NonDim() == INITIAL_VALUES) {
      cout << "Incompressible flow: rho_ref, vel_ref, and temp_ref" << endl;
      cout << "are based on the initial values, p_ref = rho_ref*vel_ref^2." << endl;
      if (grid_movement) cout << "Force coefficients computed using MACH_MOTION." << endl;
      else cout << "Force coefficients computed using initial values." << endl;
    } 
    else if (config->GetRef_Inc_NonDim() == REFERENCE_VALUES) {
      cout << "Incompressible flow: rho_ref, vel_ref, and temp_ref" << endl;
      cout << "are user-provided reference values, p_ref = rho_ref*vel_ref^2." << endl;
      if (grid_movement) cout << "Force coefficients computed using MACH_MOTION." << endl;
      else cout << "Force coefficients computed using reference values." << endl;
    }
    cout << "The reference area for force coeffs. is " << config->GetRefArea() << " m^2." << endl;
    cout << "The reference length for force coeffs. is " << config->GetRefLength() << " m." << endl;

    cout << "The pressure is decomposed into thermodynamic and dynamic components." << endl;
    cout << "The initial value of the dynamic pressure is 0." << endl;

    cout << "Mach number: "<< config->GetMach();
    if (config->GetKind_FluidModel() == CONSTANT_DENSITY) {
      cout << ", computed using the Bulk modulus." << endl;
    } else {
      cout << ", computed using fluid speed of sound." << endl;
    }

    cout << "For external flows, the initial state is imposed at the far-field." << endl;
    cout << "Angle of attack (deg): "<< config->GetAoA() << ", computed using the initial velocity." << endl;
    cout << "Side slip angle (deg): "<< config->GetAoS() << ", computed using the initial velocity." << endl;

    if (viscous) { 
      cout << "Reynolds number per meter: " << config->GetReynolds() << ", computed using initial values."<< endl;
      cout << "Reynolds number is a byproduct of inputs only (not used internally)." << endl;
    }
    cout << "SI units only. The grid should be dimensional (meters)." << endl;
    
    switch (config->GetKind_DensityModel()) {
      
      case CONSTANT:
        if (energy) cout << "Energy equation is active and decoupled." << endl;
        else cout << "No energy equation." << endl;
        break;

      case BOUSSINESQ:
        if (energy) cout << "Energy equation is active and coupled through Boussinesq approx." << endl;
        break;

      case VARIABLE:
        if (energy) cout << "Energy equation is active and coupled for variable density." << endl;
        break;

    }

    cout <<"-- Input conditions:"<< endl;
    
    switch (config->GetKind_FluidModel()) {
      
      case CONSTANT_DENSITY:
        cout << "Fluid Model: CONSTANT_DENSITY "<< endl;
        if (energy) {
          cout << "Specific heat at constant pressure (Cp): " << config->GetSpecific_Heat_Cp() << " N.m/kg.K." << endl;
        }
        if (boussinesq) cout << "Thermal expansion coefficient: " << config->GetThermal_Expansion_Coeff() << " K^-1." << endl;
        cout << "Thermodynamic pressure not required." << endl;
        break;
        
      case INC_IDEAL_GAS:
        cout << "Fluid Model: INC_IDEAL_GAS "<< endl;
        cout << "Variable density incompressible flow using ideal gas law." << endl;
        cout << "Density is a function of temperature (constant thermodynamic pressure)." << endl;
        cout << "Specific heat at constant pressure (Cp): " << config->GetSpecific_Heat_Cp() << " N.m/kg.K." << endl;
        cout << "Molecular weight : "<< config->GetMolecular_Weight() << " g/mol" << endl;
        cout << "Specific gas constant: " << config->GetGas_Constant() << " N.m/kg.K." << endl;
        cout << "Thermodynamic pressure: " << config->GetPressure_Thermodynamic();
        if (config->GetSystemMeasurements() == SI) cout << " Pa." << endl;
        else if (config->GetSystemMeasurements() == US) cout << " psf." << endl;
        break;
      
    }
    if (viscous) {
      switch (config->GetKind_ViscosityModel()) {

        case CONSTANT_VISCOSITY:
          cout << "Viscosity Model: CONSTANT_VISCOSITY  "<< endl;
          cout << "Constant Laminar Viscosity: " << config->GetMu_Constant();
          if (config->GetSystemMeasurements() == SI) cout << " N.s/m^2." << endl;
          else if (config->GetSystemMeasurements() == US) cout << " lbf.s/ft^2." << endl;
          cout << "Laminar Viscosity (non-dim): " << config->GetMu_ConstantND()<< endl;
          break;

        case SUTHERLAND:
          cout << "Viscosity Model: SUTHERLAND "<< endl;
          cout << "Ref. Laminar Viscosity: " << config->GetMu_Ref();
          if (config->GetSystemMeasurements() == SI) cout << " N.s/m^2." << endl;
          else if (config->GetSystemMeasurements() == US) cout << " lbf.s/ft^2." << endl;
          cout << "Ref. Temperature: " << config->GetMu_Temperature_Ref();
          if (config->GetSystemMeasurements() == SI) cout << " K." << endl;
          else if (config->GetSystemMeasurements() == US) cout << " R." << endl;
          cout << "Sutherland Constant: "<< config->GetMu_S();
          if (config->GetSystemMeasurements() == SI) cout << " K." << endl;
          else if (config->GetSystemMeasurements() == US) cout << " R." << endl;
          cout << "Laminar Viscosity (non-dim): " << config->GetMu_ConstantND()<< endl;
          cout << "Ref. Temperature (non-dim): " << config->GetMu_Temperature_RefND()<< endl;
          cout << "Sutherland constant (non-dim): "<< config->GetMu_SND()<< endl;
          break;

      }

      if (energy) {
        switch (config->GetKind_ConductivityModel()) {

          case CONSTANT_PRANDTL:
            cout << "Conductivity Model: CONSTANT_PRANDTL  "<< endl;
            cout << "Prandtl (Laminar): " << config->GetPrandtl_Lam()<< endl;
            cout << "Prandtl (Turbulent): " << config->GetPrandtl_Turb()<< endl;
            break;

          case CONSTANT_CONDUCTIVITY:
            cout << "Conductivity Model: CONSTANT_CONDUCTIVITY "<< endl;
            cout << "Molecular Conductivity: " << config->GetKt_Constant()<< " W/m^2.K." << endl;
            cout << "Molecular Conductivity (non-dim): " << config->GetKt_ConstantND()<< endl;
            break;

        }
      }

    }
    
    if (config->GetKind_FluidModel() == CONSTANT_DENSITY) {
      cout << "Bulk modulus: " << config->GetBulk_Modulus();
      if (config->GetSystemMeasurements() == SI) cout << " Pa." << endl;
      else if (config->GetSystemMeasurements() == US) cout << " psf." << endl;
    }

    cout << "Initial dynamic pressure: " << config->GetPressure_FreeStream();
    if (config->GetSystemMeasurements() == SI) cout << " Pa." << endl;
    else if (config->GetSystemMeasurements() == US) cout << " psf." << endl;
    
    cout << "Initial total pressure: " << config->GetPressure_FreeStream() + 0.5*config->GetDensity_FreeStream()*config->GetModVel_FreeStream()*config->GetModVel_FreeStream();
    if (config->GetSystemMeasurements() == SI) cout << " Pa." << endl;
    else if (config->GetSystemMeasurements() == US) cout << " psf." << endl;

    if (energy) {
      cout << "Initial temperature: " << config->GetTemperature_FreeStream();
      if (config->GetSystemMeasurements() == SI) cout << " K." << endl;
      else if (config->GetSystemMeasurements() == US) cout << " R." << endl;
    }

    cout << "Initial density: " << config->GetDensity_FreeStream();
    if (config->GetSystemMeasurements() == SI) cout << " kg/m^3." << endl;
    else if (config->GetSystemMeasurements() == US) cout << " slug/ft^3." << endl;
    
    if (nDim == 2) {
      cout << "Initial velocity: (" << config->GetVelocity_FreeStream()[0] << ", ";
      cout << config->GetVelocity_FreeStream()[1] << ")";
    }
    if (nDim == 3) {
      cout << "Initial velocity: (" << config->GetVelocity_FreeStream()[0] << ", ";
      cout << config->GetVelocity_FreeStream()[1] << ", " << config->GetVelocity_FreeStream()[2] << ")";
    }
    if (config->GetSystemMeasurements() == SI) cout << " m/s. ";
    else if (config->GetSystemMeasurements() == US) cout << " ft/s. ";
    
    cout << "Magnitude: "  << config->GetModVel_FreeStream();
    if (config->GetSystemMeasurements() == SI) cout << " m/s." << endl;
    else if (config->GetSystemMeasurements() == US) cout << " ft/s." << endl;

    if (viscous) {
      cout << "Initial laminar viscosity: " << config->GetViscosity_FreeStream();
      if (config->GetSystemMeasurements() == SI) cout << " N.s/m^2." << endl;
      else if (config->GetSystemMeasurements() == US) cout << " lbf.s/ft^2." << endl;
      if (turbulent) {
        cout << "Initial turb. kinetic energy per unit mass: " << config->GetTke_FreeStream();
        if (config->GetSystemMeasurements() == SI) cout << " m^2/s^2." << endl;
        else if (config->GetSystemMeasurements() == US) cout << " ft^2/s^2." << endl;
        cout << "Initial specific dissipation: " << config->GetOmega_FreeStream();
        if (config->GetSystemMeasurements() == SI) cout << " 1/s." << endl;
        else if (config->GetSystemMeasurements() == US) cout << " 1/s." << endl;
      }
    }
    
    if (unsteady) { cout << "Total time: " << config->GetTotal_UnstTime() << " s. Time step: " << config->GetDelta_UnstTime() << " s." << endl; }
    
    /*--- Print out reference values. ---*/
    
    cout <<"-- Reference values:"<< endl;

    if (config->GetKind_FluidModel() != CONSTANT_DENSITY) {
      cout << "Reference specific gas constant: " << config->GetGas_Constant_Ref();
      if (config->GetSystemMeasurements() == SI) cout << " N.m/kg.K." << endl;
      else if (config->GetSystemMeasurements() == US) cout << " lbf.ft/slug.R." << endl;
    } else {
      if (energy) {
        cout << "Reference specific heat: " << config->GetGas_Constant_Ref();
        if (config->GetSystemMeasurements() == SI) cout << " N.m/kg.K." << endl;
        else if (config->GetSystemMeasurements() == US) cout << " lbf.ft/slug.R." << endl;
      }
    }

    cout << "Reference pressure: " << config->GetPressure_Ref();
    if (config->GetSystemMeasurements() == SI) cout << " Pa." << endl;
    else if (config->GetSystemMeasurements() == US) cout << " psf." << endl;
    
    if (energy) {
      cout << "Reference temperature: " << config->GetTemperature_Ref();
      if (config->GetSystemMeasurements() == SI) cout << " K." << endl;
      else if (config->GetSystemMeasurements() == US) cout << " R." << endl;
    }

    cout << "Reference density: " << config->GetDensity_Ref();
    if (config->GetSystemMeasurements() == SI) cout << " kg/m^3." << endl;
    else if (config->GetSystemMeasurements() == US) cout << " slug/ft^3." << endl;
    
    cout << "Reference velocity: " << config->GetVelocity_Ref();
    if (config->GetSystemMeasurements() == SI) cout << " m/s." << endl;
    else if (config->GetSystemMeasurements() == US) cout << " ft/s." << endl;

    cout << "Reference length: " << config->GetLength_Ref();
    if (config->GetSystemMeasurements() == SI) cout << " m." << endl;
    else if (config->GetSystemMeasurements() == US) cout << " in." << endl;

    if (viscous) {
      cout << "Reference viscosity: " << config->GetViscosity_Ref();
      if (config->GetSystemMeasurements() == SI) cout << " N.s/m^2." << endl;
      else if (config->GetSystemMeasurements() == US) cout << " lbf.s/ft^2." << endl;
    }
    
    if (unsteady) cout << "Reference time: " << config->GetTime_Ref() <<" s." << endl;
    
    /*--- Print out resulting non-dim values here. ---*/
    
    cout << "-- Resulting non-dimensional state:" << endl;
    cout << "Mach number (non-dim): " << config->GetMach() << endl;
    if (viscous) {
      cout << "Reynolds number (per m): " << config->GetReynolds() << endl;
    }
    
    if (config->GetKind_FluidModel() != CONSTANT_DENSITY) {
      cout << "Specific gas constant (non-dim): " << config->GetGas_ConstantND() << endl;
      cout << "Initial thermodynamic pressure (non-dim): " << config->GetPressure_ThermodynamicND() << endl;
    } else {
      if (energy) {
        cout << "Specific heat at constant pressure (non-dim): " << config->GetSpecific_Heat_CpND() << endl;
        if (boussinesq) cout << "Thermal expansion coefficient (non-dim.): " << config->GetThermal_Expansion_CoeffND() << " K^-1." << endl;
      }
    }
    
    if (energy) cout << "Initial temperature (non-dim): " << config->GetTemperature_FreeStreamND() << endl;
    cout << "Initial pressure (non-dim): " << config->GetPressure_FreeStreamND() << endl;
    cout << "Initial density (non-dim): " << config->GetDensity_FreeStreamND() << endl;
    
    if (nDim == 2) {
      cout << "Initial velocity (non-dim): (" << config->GetVelocity_FreeStreamND()[0] << ", ";
      cout << config->GetVelocity_FreeStreamND()[1] << "). ";
    } else {
      cout << "Initial velocity (non-dim): (" << config->GetVelocity_FreeStreamND()[0] << ", ";
      cout << config->GetVelocity_FreeStreamND()[1] << ", " << config->GetVelocity_FreeStreamND()[2] << "). ";
    }
    cout << "Magnitude: "   << config->GetModVel_FreeStreamND() << endl;
    
    if (viscous) {
      cout << "Initial viscosity (non-dim): " << config->GetViscosity_FreeStreamND() << endl;
      if (turbulent) {
        cout << "Initial turb. kinetic energy (non-dim): " << config->GetTke_FreeStreamND() << endl;
        cout << "Initial specific dissipation (non-dim): " << config->GetOmega_FreeStreamND() << endl;
      }
    }
    
    if (unsteady) {
      cout << "Total time (non-dim): " << config->GetTotal_UnstTimeND() << endl;
      cout << "Time step (non-dim): " << config->GetDelta_UnstTimeND() << endl;
    }
    
    cout << endl;
    
  }
  
}


void CPBIncEulerSolver::Centered_Residual(CGeometry *geometry, CSolver **solver_container, CNumerics *numerics,
                                     CConfig *config, unsigned short iMesh, unsigned short iRKStep) {

  unsigned long iEdge, iPoint, jPoint;

  bool implicit      = (config->GetKind_TimeIntScheme_Flow() == EULER_IMPLICIT);
  bool jst_scheme  = ((config->GetKind_Centered_Flow() == JST) && (iMesh == MESH_0));
  bool grid_movement = config->GetGrid_Movement();

  for (iEdge = 0; iEdge < geometry->GetnEdge(); iEdge++) {

    /*--- Points in edge, set normal vectors, and number of neighbors ---*/

    iPoint = geometry->edge[iEdge]->GetNode(0); jPoint = geometry->edge[iEdge]->GetNode(1);
    numerics->SetNormal(geometry->edge[iEdge]->GetNormal());
    numerics->SetNeighbor(geometry->node[iPoint]->GetnNeighbor(), geometry->node[jPoint]->GetnNeighbor());

    /*--- Set primitive variables w/o reconstruction ---*/

    numerics->SetPrimitive(node[iPoint]->GetPrimitive(), node[jPoint]->GetPrimitive());

    /*--- Set the largest convective eigenvalue ---*/

    numerics->SetLambda(node[iPoint]->GetLambda(), node[jPoint]->GetLambda());

    /*--- Set undivided laplacian and pressure-based sensor ---*/

    /*if (jst_scheme) {
      numerics->SetUndivided_Laplacian(node[iPoint]->GetUndivided_Laplacian(), node[jPoint]->GetUndivided_Laplacian());
      numerics->SetSensor(node[iPoint]->GetSensor(), node[jPoint]->GetSensor());
    }*/

    /*--- Grid movement ---*/

    if (grid_movement) {
      numerics->SetGridVel(geometry->node[iPoint]->GetGridVel(), geometry->node[jPoint]->GetGridVel());
    }

    /*--- Compute residuals, and Jacobians ---*/

    numerics->ComputeResidual(Res_Conv, Jacobian_i, Jacobian_j, config);

    /*--- Update convective and artificial dissipation residuals ---*/

    LinSysRes.AddBlock(iPoint, Res_Conv);
    LinSysRes.SubtractBlock(jPoint, Res_Conv);

    /*--- Store implicit contributions from the reisdual calculation. ---*/

    if (implicit) {
      Jacobian.AddBlock(iPoint, iPoint, Jacobian_i);
      Jacobian.AddBlock(iPoint, jPoint, Jacobian_j);
      Jacobian.SubtractBlock(jPoint, iPoint, Jacobian_i);
      Jacobian.SubtractBlock(jPoint, jPoint, Jacobian_j);
    }
  }

}



void CPBIncEulerSolver::Upwind_Residual(CGeometry *geometry, CSolver **solver_container, CNumerics *numerics,
                                   CConfig *config, unsigned short iMesh) {
  
  su2double **Gradient_i, **Gradient_j, Project_Grad_i, Project_Grad_j,
  *V_i, *V_j, *S_i, *S_j, *Limiter_i = NULL, *Limiter_j = NULL, Non_Physical = 1.0;
  
  unsigned long iEdge, iPoint, jPoint, counter_local = 0, counter_global = 0;
  unsigned short iDim, iVar;
  
  unsigned long ExtIter = config->GetExtIter();
  bool implicit         = (config->GetKind_TimeIntScheme_Flow() == EULER_IMPLICIT);
  bool muscl            = (config->GetMUSCL_Flow() && (iMesh == MESH_0));
  bool disc_adjoint     = config->GetDiscrete_Adjoint();
  bool limiter          = ((config->GetKind_SlopeLimit_Flow() != NO_LIMITER) && (ExtIter <= config->GetLimiterIter()) && !(disc_adjoint && config->GetFrozen_Limiter_Disc()));
  bool grid_movement    = config->GetGrid_Movement();

  /*--- Loop over all the edges ---*/
  
  for (iEdge = 0; iEdge < geometry->GetnEdge(); iEdge++) {
    
    /*--- Points in edge and normal vectors ---*/
    
    iPoint = geometry->edge[iEdge]->GetNode(0); jPoint = geometry->edge[iEdge]->GetNode(1);
    numerics->SetNormal(geometry->edge[iEdge]->GetNormal());
    
    /*--- Grid movement ---*/
    
    if (grid_movement)
      numerics->SetGridVel(geometry->node[iPoint]->GetGridVel(), geometry->node[jPoint]->GetGridVel());
    
    /*--- Get primitive variables ---*/
    
    V_i = node[iPoint]->GetPrimitive(); V_j = node[jPoint]->GetPrimitive();
    S_i = node[iPoint]->GetSecondary(); S_j = node[jPoint]->GetSecondary();

    /*--- High order reconstruction using MUSCL strategy ---*/
    
    if (muscl) {
      
      for (iDim = 0; iDim < nDim; iDim++) {
        Vector_i[iDim] = 0.5*(geometry->node[jPoint]->GetCoord(iDim) - geometry->node[iPoint]->GetCoord(iDim));
        Vector_j[iDim] = 0.5*(geometry->node[iPoint]->GetCoord(iDim) - geometry->node[jPoint]->GetCoord(iDim));
      }
      
      Gradient_i = node[iPoint]->GetGradient_Primitive();
      Gradient_j = node[jPoint]->GetGradient_Primitive();
      if (limiter) {
        Limiter_i = node[iPoint]->GetLimiter_Primitive();
        Limiter_j = node[jPoint]->GetLimiter_Primitive();
      }
      
      for (iVar = 0; iVar < nPrimVarGrad; iVar++) {
        Project_Grad_i = 0.0; Project_Grad_j = 0.0;
        Non_Physical = node[iPoint]->GetNon_Physical()*node[jPoint]->GetNon_Physical();
        for (iDim = 0; iDim < nDim; iDim++) {
          Project_Grad_i += Vector_i[iDim]*Gradient_i[iVar][iDim]*Non_Physical;
          Project_Grad_j += Vector_j[iDim]*Gradient_j[iVar][iDim]*Non_Physical;
        }
        if (limiter) {
          Primitive_i[iVar] = V_i[iVar] + Limiter_i[iVar]*Project_Grad_i;
          Primitive_j[iVar] = V_j[iVar] + Limiter_j[iVar]*Project_Grad_j;
        }
        else {
          Primitive_i[iVar] = V_i[iVar] + Project_Grad_i;
          Primitive_j[iVar] = V_j[iVar] + Project_Grad_j;
        }
      }

      numerics->SetPrimitive(Primitive_i, Primitive_j);
      
    } else {
      
      /*--- Set conservative variables without reconstruction ---*/      
      numerics->SetPrimitive(V_i, V_j);
      numerics->SetSecondary(S_i, S_j);
      
    }
    
    /*--- Compute the residual ---*/
    numerics->ComputeResidual(Res_Conv, Jacobian_i, Jacobian_j, config);

    /*--- Update residual value ---*/
    
    LinSysRes.AddBlock(iPoint, Res_Conv);
    LinSysRes.SubtractBlock(jPoint, Res_Conv);
   
    
    /*--- Set implicit Jacobians ---*/
    
    if (implicit) {
      Jacobian.AddBlock(iPoint, iPoint, Jacobian_i);
      Jacobian.AddBlock(iPoint, jPoint, Jacobian_j);
      Jacobian.SubtractBlock(jPoint, iPoint, Jacobian_i);
      Jacobian.SubtractBlock(jPoint, jPoint, Jacobian_j);
    }
  }
  
  /*--- Warning message about non-physical reconstructions. ---*/
  
  if (config->GetConsole_Output_Verb() == VERB_HIGH) {
#ifdef HAVE_MPI
    SU2_MPI::Reduce(&counter_local, &counter_global, 1, MPI_UNSIGNED_LONG, MPI_SUM, MASTER_NODE, MPI_COMM_WORLD);
#else
    counter_global = counter_local;
#endif
    if (iMesh == MESH_0) config->SetNonphysical_Reconstr(counter_global);
  }

}



void CPBIncEulerSolver::Source_Residual(CGeometry *geometry, CSolver **solver_container, CNumerics *numerics, CNumerics *second_numerics,
                                   CConfig *config, unsigned short iMesh) {

  unsigned short iVar,iDim;
  unsigned long iEdge, iPoint, jPoint;



  bool implicit         = (config->GetKind_TimeIntScheme_Flow() == EULER_IMPLICIT);
  bool rotating_frame   = config->GetRotating_Frame();
  bool axisymmetric     = config->GetAxisymmetric();
  bool gravity          = (config->GetGravityForce() == YES);

  /*--- Initialize the source residual to zero ---*/
  for (iVar = 0; iVar < nVar; iVar++) Residual[iVar] = 0.0;
  

  if (rotating_frame) {

    /*--- Loop over all points ---*/

    for (iPoint = 0; iPoint < nPointDomain; iPoint++) {

      /*--- Load the conservative variables ---*/

      numerics->SetConservative(node[iPoint]->GetSolution(),
                                node[iPoint]->GetSolution());

      /*--- Load the volume of the dual mesh cell ---*/

      numerics->SetVolume(geometry->node[iPoint]->GetVolume());

      /*--- Compute the rotating frame source residual ---*/

      numerics->ComputeResidual(Residual, Jacobian_i, config);

      /*--- Add the source residual to the total ---*/

      LinSysRes.AddBlock(iPoint, Residual);

      /*--- Add the implicit Jacobian contribution ---*/

      if (implicit) Jacobian.AddBlock(iPoint, iPoint, Jacobian_i);

    }
  }

  if (axisymmetric) {

    /*--- Zero out Jacobian structure ---*/
    if (implicit) {
      for (iVar = 0; iVar < nVar; iVar ++)
        for (unsigned short jVar = 0; jVar < nVar; jVar ++)
          Jacobian_i[iVar][jVar] = 0.0;
    }

    /*--- loop over points ---*/

    for (iPoint = 0; iPoint < nPointDomain; iPoint++) {

      /*--- Set solution  ---*/

      numerics->SetConservative(node[iPoint]->GetSolution(),
                                node[iPoint]->GetSolution());

      /*--- Set incompressible density  ---*/

      numerics->SetDensity(node[iPoint]->GetDensity(),
                           node[iPoint]->GetDensity());

      /*--- Set control volume ---*/

      numerics->SetVolume(geometry->node[iPoint]->GetVolume());

      /*--- Set y coordinate ---*/

      numerics->SetCoord(geometry->node[iPoint]->GetCoord(),
                         geometry->node[iPoint]->GetCoord());

      /*--- Compute Source term Residual ---*/

      numerics->ComputeResidual(Residual, Jacobian_i, config);

      /*--- Add Residual ---*/

      LinSysRes.AddBlock(iPoint, Residual);

      /*--- Implicit part ---*/

      if (implicit) Jacobian.AddBlock(iPoint, iPoint, Jacobian_i);

    }
  }

  if (gravity) {

    /*--- loop over points ---*/

    for (iPoint = 0; iPoint < nPointDomain; iPoint++) {

      /*--- Set solution  ---*/

      numerics->SetConservative(node[iPoint]->GetSolution(),
                                node[iPoint]->GetSolution());

      /*--- Set incompressible density  ---*/

      numerics->SetDensity(node[iPoint]->GetDensity(),
                           node[iPoint]->GetDensity());

      /*--- Set control volume ---*/

      numerics->SetVolume(geometry->node[iPoint]->GetVolume());

      /*--- Compute Source term Residual ---*/

      numerics->ComputeResidual(Residual, config);

      /*--- Add Residual ---*/

      LinSysRes.AddBlock(iPoint, Residual);

    }

  }
}



void CPBIncEulerSolver::SetPrimitive_Gradient_GG(CGeometry *geometry, CConfig *config) {
  unsigned long iPoint, jPoint, iEdge, iVertex;
  unsigned short iDim, iVar, iMarker;
  su2double *PrimVar_Vertex, *PrimVar_i, *PrimVar_j, PrimVar_Average,
  Partial_Gradient, Partial_Res, *Normal;

  /*--- Incompressible flow, primitive variables nDim+2, (P, vx, vy, vz, rho) ---*/

  PrimVar_Vertex = new su2double [nPrimVarGrad];
  PrimVar_i = new su2double [nPrimVarGrad];
  PrimVar_j = new su2double [nPrimVarGrad];

  /*--- Set Gradient_Primitive to zero ---*/
  for (iPoint = 0; iPoint < nPointDomain; iPoint++)
    node[iPoint]->SetGradient_PrimitiveZero(nPrimVarGrad);
 
  /*--- Loop interior edges ---*/
  for (iEdge = 0; iEdge < geometry->GetnEdge(); iEdge++) {
    iPoint = geometry->edge[iEdge]->GetNode(0);
    jPoint = geometry->edge[iEdge]->GetNode(1);

    for (iVar = 0; iVar < nPrimVarGrad; iVar++) {
      PrimVar_i[iVar] = node[iPoint]->GetPrimitive(iVar);
      PrimVar_j[iVar] = node[jPoint]->GetPrimitive(iVar);
    }

    Normal = geometry->edge[iEdge]->GetNormal();
    for (iVar = 0; iVar < nPrimVarGrad; iVar++) {
      PrimVar_Average =  0.5 * ( PrimVar_i[iVar] + PrimVar_j[iVar] );
      for (iDim = 0; iDim < nDim; iDim++) {
        Partial_Res = PrimVar_Average*Normal[iDim];
        if (geometry->node[iPoint]->GetDomain())
          node[iPoint]->AddGradient_Primitive(iVar, iDim, Partial_Res);
        if (geometry->node[jPoint]->GetDomain())
          node[jPoint]->SubtractGradient_Primitive(iVar, iDim, Partial_Res);
      }
    }
  }

  /*--- Loop boundary edges ---*/
  for (iMarker = 0; iMarker < geometry->GetnMarker(); iMarker++) {
    if (config->GetMarker_All_KindBC(iMarker) != INTERNAL_BOUNDARY &&
        config->GetMarker_All_KindBC(iMarker) != PERIODIC_BOUNDARY)
    for (iVertex = 0; iVertex < geometry->GetnVertex(iMarker); iVertex++) {
      iPoint = geometry->vertex[iMarker][iVertex]->GetNode();
      if (geometry->node[iPoint]->GetDomain()) {

        for (iVar = 0; iVar < nPrimVarGrad; iVar++)
          PrimVar_Vertex[iVar] = node[iPoint]->GetPrimitive(iVar);

        Normal = geometry->vertex[iMarker][iVertex]->GetNormal();
        for (iVar = 0; iVar < nPrimVarGrad; iVar++)
          for (iDim = 0; iDim < nDim; iDim++) {
            Partial_Res = PrimVar_Vertex[iVar]*Normal[iDim];
            node[iPoint]->SubtractGradient_Primitive(iVar, iDim, Partial_Res);
          }
      }
    }
  }

  /*--- Update gradient value ---*/
  for (iPoint = 0; iPoint < nPointDomain; iPoint++) {
    for (iVar = 0; iVar < nPrimVarGrad; iVar++) {
      for (iDim = 0; iDim < nDim; iDim++) {
        Partial_Gradient = node[iPoint]->GetGradient_Primitive(iVar, iDim) / (geometry->node[iPoint]->GetVolume());
        node[iPoint]->SetGradient_Primitive(iVar, iDim, Partial_Gradient);
      }
    }
  }

  delete [] PrimVar_Vertex;
  delete [] PrimVar_i;
  delete [] PrimVar_j;

  //Set_MPI_Primitive_Gradient(geometry, config);

}

void CPBIncEulerSolver::SetPrimitive_Gradient_LS(CGeometry *geometry, CConfig *config) {

  unsigned short iVar, iDim, jDim, iNeigh;
  unsigned long iPoint, jPoint;
  su2double *PrimVar_i, *PrimVar_j, *Coord_i, *Coord_j, r11, r12, r13, r22, r23, r23_a,
  r23_b, r33, weight, product, z11, z12, z13, z22, z23, z33, detR2;
  bool singular;

  /*--- Incompressible flow, primitive variables nDim+3, (P, vx, vy, vz, rho, beta) ---*/

  /*--- Loop over points of the grid ---*/

  for (iPoint = 0; iPoint < nPointDomain; iPoint++) {

    /*--- Set the value of the singular ---*/
    singular = false;

    /*--- Get coordinates ---*/

    Coord_i = geometry->node[iPoint]->GetCoord();

    /*--- Get primitives from CVariable ---*/

    PrimVar_i = node[iPoint]->GetPrimitive();

    /*--- Inizialization of variables ---*/

    for (iVar = 0; iVar < nPrimVarGrad; iVar++)
      for (iDim = 0; iDim < nDim; iDim++)
        Cvector[iVar][iDim] = 0.0;

    r11 = 0.0; r12 = 0.0;   r13 = 0.0;    r22 = 0.0;
    r23 = 0.0; r23_a = 0.0; r23_b = 0.0;  r33 = 0.0;

    AD::StartPreacc();
    AD::SetPreaccIn(PrimVar_i, nPrimVarGrad);
    AD::SetPreaccIn(Coord_i, nDim);

    for (iNeigh = 0; iNeigh < geometry->node[iPoint]->GetnPoint(); iNeigh++) {
      jPoint = geometry->node[iPoint]->GetPoint(iNeigh);
      Coord_j = geometry->node[jPoint]->GetCoord();

      PrimVar_j = node[jPoint]->GetPrimitive();

      //AD::SetPreaccIn(Coord_j, nDim);
      //AD::SetPreaccIn(PrimVar_j, nPrimVarGrad);

      weight = 0.0;
      for (iDim = 0; iDim < nDim; iDim++)
        weight += (Coord_j[iDim]-Coord_i[iDim])*(Coord_j[iDim]-Coord_i[iDim]);

      /*--- Sumations for entries of upper triangular matrix R ---*/

      if (weight != 0.0) {

        r11 += (Coord_j[0]-Coord_i[0])*(Coord_j[0]-Coord_i[0])/weight;
        r12 += (Coord_j[0]-Coord_i[0])*(Coord_j[1]-Coord_i[1])/weight;
        r22 += (Coord_j[1]-Coord_i[1])*(Coord_j[1]-Coord_i[1])/weight;

        if (nDim == 3) {
          r13 += (Coord_j[0]-Coord_i[0])*(Coord_j[2]-Coord_i[2])/weight;
          r23_a += (Coord_j[1]-Coord_i[1])*(Coord_j[2]-Coord_i[2])/weight;
          r23_b += (Coord_j[0]-Coord_i[0])*(Coord_j[2]-Coord_i[2])/weight;
          r33 += (Coord_j[2]-Coord_i[2])*(Coord_j[2]-Coord_i[2])/weight;
        }

        /*--- Entries of c:= transpose(A)*b ---*/

        for (iVar = 0; iVar < nPrimVarGrad; iVar++)
          for (iDim = 0; iDim < nDim; iDim++)
            Cvector[iVar][iDim] += (Coord_j[iDim]-Coord_i[iDim])*(PrimVar_j[iVar]-PrimVar_i[iVar])/weight;

      }

    }

    /*--- Entries of upper triangular matrix R ---*/

    if (r11 >= 0.0) r11 = sqrt(r11); else r11 = 0.0;
    if (r11 != 0.0) r12 = r12/r11; else r12 = 0.0;
    if (r22-r12*r12 >= 0.0) r22 = sqrt(r22-r12*r12); else r22 = 0.0;

    if (nDim == 3) {
      if (r11 != 0.0) r13 = r13/r11; else r13 = 0.0;
      if ((r22 != 0.0) && (r11*r22 != 0.0)) r23 = r23_a/r22 - r23_b*r12/(r11*r22); else r23 = 0.0;
      if (r33-r23*r23-r13*r13 >= 0.0) r33 = sqrt(r33-r23*r23-r13*r13); else r33 = 0.0;
    }

    /*--- Compute determinant ---*/

    if (nDim == 2) detR2 = (r11*r22)*(r11*r22);
    else detR2 = (r11*r22*r33)*(r11*r22*r33);

    /*--- Detect singular matrices ---*/

    if (abs(detR2) <= EPS) { detR2 = 1.0; singular = true; }

    /*--- S matrix := inv(R)*traspose(inv(R)) ---*/

    if (singular) {
      for (iDim = 0; iDim < nDim; iDim++)
        for (jDim = 0; jDim < nDim; jDim++)
          Smatrix[iDim][jDim] = 0.0;
    }
    else {
      if (nDim == 2) {
        Smatrix[0][0] = (r12*r12+r22*r22)/detR2;
        Smatrix[0][1] = -r11*r12/detR2;
        Smatrix[1][0] = Smatrix[0][1];
        Smatrix[1][1] = r11*r11/detR2;
      }
      else {
        z11 = r22*r33; z12 = -r12*r33; z13 = r12*r23-r13*r22;
        z22 = r11*r33; z23 = -r11*r23; z33 = r11*r22;
        Smatrix[0][0] = (z11*z11+z12*z12+z13*z13)/detR2;
        Smatrix[0][1] = (z12*z22+z13*z23)/detR2;
        Smatrix[0][2] = (z13*z33)/detR2;
        Smatrix[1][0] = Smatrix[0][1];
        Smatrix[1][1] = (z22*z22+z23*z23)/detR2;
        Smatrix[1][2] = (z23*z33)/detR2;
        Smatrix[2][0] = Smatrix[0][2];
        Smatrix[2][1] = Smatrix[1][2];
        Smatrix[2][2] = (z33*z33)/detR2;
      }
    }

    /*--- Computation of the gradient: S*c ---*/
    for (iVar = 0; iVar < nPrimVarGrad; iVar++) {
      for (iDim = 0; iDim < nDim; iDim++) {
        product = 0.0;
        for (jDim = 0; jDim < nDim; jDim++) {
          product += Smatrix[iDim][jDim]*Cvector[iVar][jDim];
        }

        node[iPoint]->SetGradient_Primitive(iVar, iDim, product);
      }
    }

    //AD::SetPreaccOut(node[iPoint]->GetGradient_Primitive(), nPrimVarGrad, nDim);
    //AD::EndPreacc();
  }

  Set_MPI_Primitive_Gradient(geometry, config);

}


void CPBIncEulerSolver::SetUniformInlet(CConfig* config, unsigned short iMarker) {
  
  if (config->GetMarker_All_KindBC(iMarker) == INLET_FLOW) {
    
    string Marker_Tag   = config->GetMarker_All_TagBound(iMarker);
    su2double p_total   = config->GetInlet_Ptotal(Marker_Tag);
    su2double t_total   = config->GetInlet_Ttotal(Marker_Tag);
    su2double* flow_dir = config->GetInlet_FlowDir(Marker_Tag);
    
    for(unsigned long iVertex=0; iVertex < nVertex[iMarker]; iVertex++){
      Inlet_Ttotal[iMarker][iVertex] = t_total;
      Inlet_Ptotal[iMarker][iVertex] = p_total;
      for (unsigned short iDim = 0; iDim < nDim; iDim++)
        Inlet_FlowDir[iMarker][iVertex][iDim] = flow_dir[iDim];
    }
    
  } else {
    
    /*--- For now, non-inlets just get set to zero. In the future, we
     can do more customization for other boundary types here. ---*/
    
    for(unsigned long iVertex=0; iVertex < nVertex[iMarker]; iVertex++){
      Inlet_Ttotal[iMarker][iVertex] = 0.0;
      Inlet_Ptotal[iMarker][iVertex] = 0.0;
      for (unsigned short iDim = 0; iDim < nDim; iDim++)
        Inlet_FlowDir[iMarker][iVertex][iDim] = 0.0;
    }
  }
  
}


void CPBIncEulerSolver::Pressure_Forces(CGeometry *geometry, CConfig *config) {

  unsigned long iVertex, iPoint;
  unsigned short iDim, iMarker, Boundary, Monitoring, iMarker_Monitoring;
  su2double Pressure = 0.0, *Normal = NULL, MomentDist[3] = {0.0,0.0,0.0}, *Coord,
  factor, RefVel2 = 0.0, RefDensity = 0.0, RefPressure,
  Force[3] = {0.0,0.0,0.0};
  su2double MomentX_Force[3] = {0.0,0.0,0.0}, MomentY_Force[3] = {0.0,0.0,0.0}, MomentZ_Force[3] = {0.0,0.0,0.0};
  su2double AxiFactor;

  bool axisymmetric = config->GetAxisymmetric();

  string Marker_Tag, Monitoring_Tag;

#ifdef HAVE_MPI
  su2double MyAllBound_CD_Inv, MyAllBound_CL_Inv, MyAllBound_CSF_Inv, MyAllBound_CMx_Inv, MyAllBound_CMy_Inv, MyAllBound_CMz_Inv, MyAllBound_CoPx_Inv, MyAllBound_CoPy_Inv, MyAllBound_CoPz_Inv, MyAllBound_CFx_Inv, MyAllBound_CFy_Inv, MyAllBound_CFz_Inv, MyAllBound_CT_Inv, MyAllBound_CQ_Inv, *MySurface_CL_Inv = NULL, *MySurface_CD_Inv = NULL, *MySurface_CSF_Inv = NULL, *MySurface_CEff_Inv = NULL, *MySurface_CFx_Inv = NULL, *MySurface_CFy_Inv = NULL, *MySurface_CFz_Inv = NULL, *MySurface_CMx_Inv = NULL, *MySurface_CMy_Inv = NULL, *MySurface_CMz_Inv = NULL;
#endif

  su2double Alpha     = config->GetAoA()*PI_NUMBER/180.0;
  su2double Beta      = config->GetAoS()*PI_NUMBER/180.0;
  su2double RefArea   = config->GetRefArea();
  su2double RefLength = config->GetRefLength();

  su2double *Origin = NULL;
  if (config->GetnMarker_Monitoring() != 0){
    Origin = config->GetRefOriginMoment(0);
  }

  /*--- Evaluate reference values for non-dimensionalization.
   For dimensional or non-dim based on initial values, use
   the far-field state (inf). For a custom non-dim based
   on user-provided reference values, use the ref values
   to compute the forces. ---*/

  if ((config->GetRef_Inc_NonDim() == DIMENSIONAL) || 
      (config->GetRef_Inc_NonDim() == INITIAL_VALUES)) {
    RefDensity  = Density_Inf;
    RefVel2 = 0.0;
    for (iDim = 0; iDim < nDim; iDim++)
      RefVel2  += Velocity_Inf[iDim]*Velocity_Inf[iDim];
  }
  else if (config->GetRef_Inc_NonDim() == REFERENCE_VALUES) {
    RefDensity = config->GetInc_Density_Ref();
    RefVel2    = config->GetInc_Velocity_Ref()*config->GetInc_Velocity_Ref();
  }

  /*--- Reference pressure is always the far-field value. ---*/

  RefPressure = Pressure_Inf;

  /*--- Compute factor for force coefficients. ---*/

  factor = 1.0 / (0.5*RefDensity*RefArea*RefVel2);

  /*-- Variables initialization ---*/

  Total_CD   = 0.0; Total_CL  = 0.0; Total_CSF = 0.0; Total_CEff = 0.0;
  Total_CMx  = 0.0; Total_CMy = 0.0; Total_CMz = 0.0;
  Total_CoPx = 0.0; Total_CoPy = 0.0;  Total_CoPz = 0.0;
  Total_CFx  = 0.0; Total_CFy = 0.0; Total_CFz = 0.0;
  Total_CT   = 0.0; Total_CQ  = 0.0; Total_CMerit = 0.0;
  Total_Heat = 0.0; Total_MaxHeat = 0.0;

  AllBound_CD_Inv   = 0.0; AllBound_CL_Inv  = 0.0;  AllBound_CSF_Inv    = 0.0;
  AllBound_CMx_Inv  = 0.0; AllBound_CMy_Inv = 0.0;  AllBound_CMz_Inv    = 0.0;
  AllBound_CoPx_Inv = 0.0; AllBound_CoPy_Inv = 0.0; AllBound_CoPz_Inv = 0.0;
  AllBound_CFx_Inv  = 0.0; AllBound_CFy_Inv = 0.0;  AllBound_CFz_Inv    = 0.0;
  AllBound_CT_Inv   = 0.0; AllBound_CQ_Inv  = 0.0;  AllBound_CMerit_Inv = 0.0;
  AllBound_CEff_Inv = 0.0;

  for (iMarker_Monitoring = 0; iMarker_Monitoring < config->GetnMarker_Monitoring(); iMarker_Monitoring++) {
    Surface_CL_Inv[iMarker_Monitoring]  = 0.0; Surface_CD_Inv[iMarker_Monitoring]   = 0.0;
    Surface_CSF_Inv[iMarker_Monitoring] = 0.0; Surface_CEff_Inv[iMarker_Monitoring] = 0.0;
    Surface_CFx_Inv[iMarker_Monitoring] = 0.0; Surface_CFy_Inv[iMarker_Monitoring]  = 0.0;
    Surface_CFz_Inv[iMarker_Monitoring] = 0.0; Surface_CMx_Inv[iMarker_Monitoring]  = 0.0;
    Surface_CMy_Inv[iMarker_Monitoring] = 0.0; Surface_CMz_Inv[iMarker_Monitoring]  = 0.0;
    
    Surface_CL[iMarker_Monitoring]  = 0.0; Surface_CD[iMarker_Monitoring]   = 0.0;
    Surface_CSF[iMarker_Monitoring] = 0.0; Surface_CEff[iMarker_Monitoring] = 0.0;
    Surface_CFx[iMarker_Monitoring] = 0.0; Surface_CFy[iMarker_Monitoring]  = 0.0;
    Surface_CFz[iMarker_Monitoring] = 0.0; Surface_CMx[iMarker_Monitoring]  = 0.0;
    Surface_CMy[iMarker_Monitoring] = 0.0; Surface_CMz[iMarker_Monitoring]  = 0.0;
  }

  /*--- Loop over the Euler and Navier-Stokes markers ---*/

  for (iMarker = 0; iMarker < nMarker; iMarker++) {

    Boundary   = config->GetMarker_All_KindBC(iMarker);
    Monitoring = config->GetMarker_All_Monitoring(iMarker);

    /*--- Obtain the origin for the moment computation for a particular marker ---*/

    if (Monitoring == YES) {
      for (iMarker_Monitoring = 0; iMarker_Monitoring < config->GetnMarker_Monitoring(); iMarker_Monitoring++) {
        Monitoring_Tag = config->GetMarker_Monitoring_TagBound(iMarker_Monitoring);
        Marker_Tag = config->GetMarker_All_TagBound(iMarker);
        if (Marker_Tag == Monitoring_Tag)
          Origin = config->GetRefOriginMoment(iMarker_Monitoring);
      }
    }

    if ((Boundary == EULER_WALL) || (Boundary == HEAT_FLUX) ||
        (Boundary == ISOTHERMAL) || (Boundary == NEARFIELD_BOUNDARY) ||
        (Boundary == INLET_FLOW) || (Boundary == OUTLET_FLOW) ||
        (Boundary == ACTDISK_INLET) || (Boundary == ACTDISK_OUTLET)||
        (Boundary == ENGINE_INFLOW) || (Boundary == ENGINE_EXHAUST)) {

      /*--- Forces initialization at each Marker ---*/

      CD_Inv[iMarker]   = 0.0; CL_Inv[iMarker]  = 0.0;  CSF_Inv[iMarker]    = 0.0;
      CMx_Inv[iMarker]  = 0.0; CMy_Inv[iMarker] = 0.0;  CMz_Inv[iMarker]    = 0.0;
      CoPx_Inv[iMarker] = 0.0; CoPy_Inv[iMarker] = 0.0; CoPz_Inv[iMarker] = 0.0;
      CFx_Inv[iMarker]  = 0.0; CFy_Inv[iMarker] = 0.0;  CFz_Inv[iMarker]    = 0.0;
      CT_Inv[iMarker]   = 0.0; CQ_Inv[iMarker]  = 0.0;  CMerit_Inv[iMarker] = 0.0;
      CEff_Inv[iMarker] = 0.0;

      for (iDim = 0; iDim < nDim; iDim++) ForceInviscid[iDim] = 0.0;
      MomentInviscid[0] = 0.0; MomentInviscid[1] = 0.0; MomentInviscid[2] = 0.0;
      MomentX_Force[0] = 0.0; MomentX_Force[1] = 0.0; MomentX_Force[2] = 0.0;
      MomentY_Force[0] = 0.0; MomentY_Force[1] = 0.0; MomentY_Force[2] = 0.0;
      MomentZ_Force[0] = 0.0; MomentZ_Force[1] = 0.0; MomentZ_Force[2] = 0.0;

      /*--- Loop over the vertices to compute the forces ---*/

      for (iVertex = 0; iVertex < geometry->GetnVertex(iMarker); iVertex++) {

        iPoint = geometry->vertex[iMarker][iVertex]->GetNode();

        Pressure = node[iPoint]->GetPressure();

        CPressure[iMarker][iVertex] = (Pressure - RefPressure)*factor*RefArea;

        /*--- Note that the pressure coefficient is computed at the
         halo cells (for visualization purposes), but not the forces ---*/

        if ( (geometry->node[iPoint]->GetDomain()) && (Monitoring == YES) ) {

          Normal = geometry->vertex[iMarker][iVertex]->GetNormal();
          Coord = geometry->node[iPoint]->GetCoord();

          for (iDim = 0; iDim < nDim; iDim++) {
            MomentDist[iDim] = Coord[iDim] - Origin[iDim];
          }

          /*--- Axisymmetric simulations ---*/

          if (axisymmetric) AxiFactor = 2.0*PI_NUMBER*geometry->node[iPoint]->GetCoord(1);
          else AxiFactor = 1.0;

          /*--- Force computation, note the minus sign due to the
           orientation of the normal (outward) ---*/

          for (iDim = 0; iDim < nDim; iDim++) {
            Force[iDim] = -(Pressure - Pressure_Inf) * Normal[iDim] * factor * AxiFactor;
            ForceInviscid[iDim] += Force[iDim];
          }

          /*--- Moment with respect to the reference axis ---*/

          if (nDim == 3) {
            MomentInviscid[0] += (Force[2]*MomentDist[1]-Force[1]*MomentDist[2])/RefLength;
            MomentX_Force[1]  += (-Force[1]*Coord[2]);
            MomentX_Force[2]  += (Force[2]*Coord[1]);

            MomentInviscid[1] += (Force[0]*MomentDist[2]-Force[2]*MomentDist[0])/RefLength;
            MomentY_Force[2]  += (-Force[2]*Coord[0]);
            MomentY_Force[0]  += (Force[0]*Coord[2]);
          }
          MomentInviscid[2] += (Force[1]*MomentDist[0]-Force[0]*MomentDist[1])/RefLength;
          MomentZ_Force[0]  += (-Force[0]*Coord[1]);
          MomentZ_Force[1]  += (Force[1]*Coord[0]);
        }

      }

      /*--- Project forces and store the non-dimensional coefficients ---*/

      if (Monitoring == YES) {

        if (Boundary != NEARFIELD_BOUNDARY) {
          if (nDim == 2) {
            CD_Inv[iMarker]  =  ForceInviscid[0]*cos(Alpha) + ForceInviscid[1]*sin(Alpha);
            CL_Inv[iMarker]  = -ForceInviscid[0]*sin(Alpha) + ForceInviscid[1]*cos(Alpha);
            CEff_Inv[iMarker]   = CL_Inv[iMarker] / (CD_Inv[iMarker]+EPS);
            CMz_Inv[iMarker]    = MomentInviscid[2];
            CoPx_Inv[iMarker]   = MomentZ_Force[1];
            CoPy_Inv[iMarker]   = -MomentZ_Force[0];
            CFx_Inv[iMarker]    = ForceInviscid[0];
            CFy_Inv[iMarker]    = ForceInviscid[1];
            CT_Inv[iMarker]     = -CFx_Inv[iMarker];
            CQ_Inv[iMarker]     = -CMz_Inv[iMarker];
            CMerit_Inv[iMarker] = CT_Inv[iMarker] / (CQ_Inv[iMarker] + EPS);
          }
          if (nDim == 3) {
            CD_Inv[iMarker]      =  ForceInviscid[0]*cos(Alpha)*cos(Beta) + ForceInviscid[1]*sin(Beta) + ForceInviscid[2]*sin(Alpha)*cos(Beta);
            CL_Inv[iMarker]      = -ForceInviscid[0]*sin(Alpha) + ForceInviscid[2]*cos(Alpha);
            CSF_Inv[iMarker] = -ForceInviscid[0]*sin(Beta)*cos(Alpha) + ForceInviscid[1]*cos(Beta) - ForceInviscid[2]*sin(Beta)*sin(Alpha);
            CEff_Inv[iMarker]       = CL_Inv[iMarker] / (CD_Inv[iMarker] + EPS);
            CMx_Inv[iMarker]        = MomentInviscid[0];
            CMy_Inv[iMarker]        = MomentInviscid[1];
            CMz_Inv[iMarker]        = MomentInviscid[2];
            CoPx_Inv[iMarker]    = -MomentY_Force[0];
            CoPz_Inv[iMarker]    = MomentY_Force[2];
            CFx_Inv[iMarker]        = ForceInviscid[0];
            CFy_Inv[iMarker]        = ForceInviscid[1];
            CFz_Inv[iMarker]        = ForceInviscid[2];
            CT_Inv[iMarker]         = -CFz_Inv[iMarker];
            CQ_Inv[iMarker]         = -CMz_Inv[iMarker];
            CMerit_Inv[iMarker]     = CT_Inv[iMarker] / (CQ_Inv[iMarker] + EPS);
          }

          AllBound_CD_Inv     += CD_Inv[iMarker];
          AllBound_CL_Inv     += CL_Inv[iMarker];
          AllBound_CSF_Inv    += CSF_Inv[iMarker];
          AllBound_CEff_Inv    = AllBound_CL_Inv / (AllBound_CD_Inv + EPS);
          AllBound_CMx_Inv    += CMx_Inv[iMarker];
          AllBound_CMy_Inv    += CMy_Inv[iMarker];
          AllBound_CMz_Inv    += CMz_Inv[iMarker];
          AllBound_CoPx_Inv   += CoPx_Inv[iMarker];
          AllBound_CoPy_Inv   += CoPy_Inv[iMarker];
          AllBound_CoPz_Inv   += CoPz_Inv[iMarker];
          AllBound_CFx_Inv    += CFx_Inv[iMarker];
          AllBound_CFy_Inv    += CFy_Inv[iMarker];
          AllBound_CFz_Inv    += CFz_Inv[iMarker];
          AllBound_CT_Inv     += CT_Inv[iMarker];
          AllBound_CQ_Inv     += CQ_Inv[iMarker];
          AllBound_CMerit_Inv  = AllBound_CT_Inv / (AllBound_CQ_Inv + EPS);

          /*--- Compute the coefficients per surface ---*/

          for (iMarker_Monitoring = 0; iMarker_Monitoring < config->GetnMarker_Monitoring(); iMarker_Monitoring++) {
            Monitoring_Tag = config->GetMarker_Monitoring_TagBound(iMarker_Monitoring);
            Marker_Tag = config->GetMarker_All_TagBound(iMarker);
            if (Marker_Tag == Monitoring_Tag) {
              Surface_CL_Inv[iMarker_Monitoring]   += CL_Inv[iMarker];
              Surface_CD_Inv[iMarker_Monitoring]   += CD_Inv[iMarker];
              Surface_CSF_Inv[iMarker_Monitoring]  += CSF_Inv[iMarker];
              Surface_CEff_Inv[iMarker_Monitoring]  = CL_Inv[iMarker] / (CD_Inv[iMarker] + EPS);
              Surface_CFx_Inv[iMarker_Monitoring]  += CFx_Inv[iMarker];
              Surface_CFy_Inv[iMarker_Monitoring]  += CFy_Inv[iMarker];
              Surface_CFz_Inv[iMarker_Monitoring]  += CFz_Inv[iMarker];
              Surface_CMx_Inv[iMarker_Monitoring]  += CMx_Inv[iMarker];
              Surface_CMy_Inv[iMarker_Monitoring]  += CMy_Inv[iMarker];
              Surface_CMz_Inv[iMarker_Monitoring]  += CMz_Inv[iMarker];
            }
          }

        }

      }

    }
  }

#ifdef HAVE_MPI

  /*--- Add AllBound information using all the nodes ---*/

  MyAllBound_CD_Inv        = AllBound_CD_Inv;        AllBound_CD_Inv = 0.0;
  MyAllBound_CL_Inv        = AllBound_CL_Inv;        AllBound_CL_Inv = 0.0;
  MyAllBound_CSF_Inv   = AllBound_CSF_Inv;   AllBound_CSF_Inv = 0.0;
  AllBound_CEff_Inv = 0.0;
  MyAllBound_CMx_Inv          = AllBound_CMx_Inv;          AllBound_CMx_Inv = 0.0;
  MyAllBound_CMy_Inv          = AllBound_CMy_Inv;          AllBound_CMy_Inv = 0.0;
  MyAllBound_CMz_Inv          = AllBound_CMz_Inv;          AllBound_CMz_Inv = 0.0;
  MyAllBound_CoPx_Inv          = AllBound_CoPx_Inv;          AllBound_CoPx_Inv = 0.0;
  MyAllBound_CoPy_Inv          = AllBound_CoPy_Inv;          AllBound_CoPy_Inv = 0.0;
  MyAllBound_CoPz_Inv          = AllBound_CoPz_Inv;          AllBound_CoPz_Inv = 0.0;
  MyAllBound_CFx_Inv          = AllBound_CFx_Inv;          AllBound_CFx_Inv = 0.0;
  MyAllBound_CFy_Inv          = AllBound_CFy_Inv;          AllBound_CFy_Inv = 0.0;
  MyAllBound_CFz_Inv          = AllBound_CFz_Inv;          AllBound_CFz_Inv = 0.0;
  MyAllBound_CT_Inv           = AllBound_CT_Inv;           AllBound_CT_Inv = 0.0;
  MyAllBound_CQ_Inv           = AllBound_CQ_Inv;           AllBound_CQ_Inv = 0.0;
  AllBound_CMerit_Inv = 0.0;

  SU2_MPI::Allreduce(&MyAllBound_CD_Inv, &AllBound_CD_Inv, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(&MyAllBound_CL_Inv, &AllBound_CL_Inv, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(&MyAllBound_CSF_Inv, &AllBound_CSF_Inv, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  AllBound_CEff_Inv = AllBound_CL_Inv / (AllBound_CD_Inv + EPS);
  SU2_MPI::Allreduce(&MyAllBound_CMx_Inv, &AllBound_CMx_Inv, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(&MyAllBound_CMy_Inv, &AllBound_CMy_Inv, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(&MyAllBound_CMz_Inv, &AllBound_CMz_Inv, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(&MyAllBound_CoPx_Inv, &AllBound_CoPx_Inv, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(&MyAllBound_CoPy_Inv, &AllBound_CoPy_Inv, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(&MyAllBound_CoPz_Inv, &AllBound_CoPz_Inv, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(&MyAllBound_CFx_Inv, &AllBound_CFx_Inv, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(&MyAllBound_CFy_Inv, &AllBound_CFy_Inv, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(&MyAllBound_CFz_Inv, &AllBound_CFz_Inv, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(&MyAllBound_CT_Inv, &AllBound_CT_Inv, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(&MyAllBound_CQ_Inv, &AllBound_CQ_Inv, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  AllBound_CMerit_Inv = AllBound_CT_Inv / (AllBound_CQ_Inv + EPS);

  /*--- Add the forces on the surfaces using all the nodes ---*/

  MySurface_CL_Inv      = new su2double[config->GetnMarker_Monitoring()];
  MySurface_CD_Inv      = new su2double[config->GetnMarker_Monitoring()];
  MySurface_CSF_Inv = new su2double[config->GetnMarker_Monitoring()];
  MySurface_CEff_Inv       = new su2double[config->GetnMarker_Monitoring()];
  MySurface_CFx_Inv        = new su2double[config->GetnMarker_Monitoring()];
  MySurface_CFy_Inv        = new su2double[config->GetnMarker_Monitoring()];
  MySurface_CFz_Inv        = new su2double[config->GetnMarker_Monitoring()];
  MySurface_CMx_Inv        = new su2double[config->GetnMarker_Monitoring()];
  MySurface_CMy_Inv        = new su2double[config->GetnMarker_Monitoring()];
  MySurface_CMz_Inv        = new su2double[config->GetnMarker_Monitoring()];

  for (iMarker_Monitoring = 0; iMarker_Monitoring < config->GetnMarker_Monitoring(); iMarker_Monitoring++) {
    MySurface_CL_Inv[iMarker_Monitoring]      = Surface_CL_Inv[iMarker_Monitoring];
    MySurface_CD_Inv[iMarker_Monitoring]      = Surface_CD_Inv[iMarker_Monitoring];
    MySurface_CSF_Inv[iMarker_Monitoring] = Surface_CSF_Inv[iMarker_Monitoring];
    MySurface_CEff_Inv[iMarker_Monitoring]       = Surface_CEff_Inv[iMarker_Monitoring];
    MySurface_CFx_Inv[iMarker_Monitoring]        = Surface_CFx_Inv[iMarker_Monitoring];
    MySurface_CFy_Inv[iMarker_Monitoring]        = Surface_CFy_Inv[iMarker_Monitoring];
    MySurface_CFz_Inv[iMarker_Monitoring]        = Surface_CFz_Inv[iMarker_Monitoring];
    MySurface_CMx_Inv[iMarker_Monitoring]        = Surface_CMx_Inv[iMarker_Monitoring];
    MySurface_CMy_Inv[iMarker_Monitoring]        = Surface_CMy_Inv[iMarker_Monitoring];
    MySurface_CMz_Inv[iMarker_Monitoring]        = Surface_CMz_Inv[iMarker_Monitoring];

    Surface_CL_Inv[iMarker_Monitoring]      = 0.0;
    Surface_CD_Inv[iMarker_Monitoring]      = 0.0;
    Surface_CSF_Inv[iMarker_Monitoring] = 0.0;
    Surface_CEff_Inv[iMarker_Monitoring]       = 0.0;
    Surface_CFx_Inv[iMarker_Monitoring]        = 0.0;
    Surface_CFy_Inv[iMarker_Monitoring]        = 0.0;
    Surface_CFz_Inv[iMarker_Monitoring]        = 0.0;
    Surface_CMx_Inv[iMarker_Monitoring]        = 0.0;
    Surface_CMy_Inv[iMarker_Monitoring]        = 0.0;
    Surface_CMz_Inv[iMarker_Monitoring]        = 0.0;
  }

  SU2_MPI::Allreduce(MySurface_CL_Inv, Surface_CL_Inv, config->GetnMarker_Monitoring(), MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(MySurface_CD_Inv, Surface_CD_Inv, config->GetnMarker_Monitoring(), MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(MySurface_CSF_Inv, Surface_CSF_Inv, config->GetnMarker_Monitoring(), MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  for (iMarker_Monitoring = 0; iMarker_Monitoring < config->GetnMarker_Monitoring(); iMarker_Monitoring++)
    Surface_CEff_Inv[iMarker_Monitoring] = Surface_CL_Inv[iMarker_Monitoring] / (Surface_CD_Inv[iMarker_Monitoring] + EPS);
  SU2_MPI::Allreduce(MySurface_CFx_Inv, Surface_CFx_Inv, config->GetnMarker_Monitoring(), MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(MySurface_CFy_Inv, Surface_CFy_Inv, config->GetnMarker_Monitoring(), MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(MySurface_CFz_Inv, Surface_CFz_Inv, config->GetnMarker_Monitoring(), MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(MySurface_CMx_Inv, Surface_CMx_Inv, config->GetnMarker_Monitoring(), MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(MySurface_CMy_Inv, Surface_CMy_Inv, config->GetnMarker_Monitoring(), MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(MySurface_CMz_Inv, Surface_CMz_Inv, config->GetnMarker_Monitoring(), MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

  delete [] MySurface_CL_Inv; delete [] MySurface_CD_Inv; delete [] MySurface_CSF_Inv;
  delete [] MySurface_CEff_Inv;  delete [] MySurface_CFx_Inv;   delete [] MySurface_CFy_Inv;
  delete [] MySurface_CFz_Inv;   delete [] MySurface_CMx_Inv;   delete [] MySurface_CMy_Inv;
  delete [] MySurface_CMz_Inv;

#endif

  /*--- Update the total coefficients (note that all the nodes have the same value) ---*/

  Total_CD            = AllBound_CD_Inv;
  Total_CL            = AllBound_CL_Inv;
  Total_CSF           = AllBound_CSF_Inv;
  Total_CEff          = Total_CL / (Total_CD + EPS);
  Total_CMx           = AllBound_CMx_Inv;
  Total_CMy           = AllBound_CMy_Inv;
  Total_CMz           = AllBound_CMz_Inv;
  Total_CoPx          = AllBound_CoPx_Inv;
  Total_CoPy          = AllBound_CoPy_Inv;
  Total_CoPz          = AllBound_CoPz_Inv;
  Total_CFx           = AllBound_CFx_Inv;
  Total_CFy           = AllBound_CFy_Inv;
  Total_CFz           = AllBound_CFz_Inv;
  Total_CT            = AllBound_CT_Inv;
  Total_CQ            = AllBound_CQ_Inv;
  Total_CMerit        = Total_CT / (Total_CQ + EPS);

  /*--- Update the total coefficients per surface (note that all the nodes have the same value)---*/

  for (iMarker_Monitoring = 0; iMarker_Monitoring < config->GetnMarker_Monitoring(); iMarker_Monitoring++) {
    Surface_CL[iMarker_Monitoring]      = Surface_CL_Inv[iMarker_Monitoring];
    Surface_CD[iMarker_Monitoring]      = Surface_CD_Inv[iMarker_Monitoring];
    Surface_CSF[iMarker_Monitoring] = Surface_CSF_Inv[iMarker_Monitoring];
    Surface_CEff[iMarker_Monitoring]       = Surface_CL_Inv[iMarker_Monitoring] / (Surface_CD_Inv[iMarker_Monitoring] + EPS);
    Surface_CFx[iMarker_Monitoring]        = Surface_CFx_Inv[iMarker_Monitoring];
    Surface_CFy[iMarker_Monitoring]        = Surface_CFy_Inv[iMarker_Monitoring];
    Surface_CFz[iMarker_Monitoring]        = Surface_CFz_Inv[iMarker_Monitoring];
    Surface_CMx[iMarker_Monitoring]        = Surface_CMx_Inv[iMarker_Monitoring];
    Surface_CMy[iMarker_Monitoring]        = Surface_CMy_Inv[iMarker_Monitoring];
    Surface_CMz[iMarker_Monitoring]        = Surface_CMz_Inv[iMarker_Monitoring];
  }

}

void CPBIncEulerSolver::Momentum_Forces(CGeometry *geometry, CConfig *config) {

  unsigned long iVertex, iPoint;
  unsigned short iDim, iMarker, Boundary, Monitoring, iMarker_Monitoring;
  su2double *Normal = NULL, MomentDist[3] = {0.0,0.0,0.0}, *Coord, Area,
  factor, RefVel2 = 0.0, RefDensity = 0.0,
  Force[3] = {0.0,0.0,0.0}, Velocity[3], MassFlow, Density;
  string Marker_Tag, Monitoring_Tag;
  su2double MomentX_Force[3] = {0.0,0.0,0.0}, MomentY_Force[3] = {0.0,0.0,0.0}, MomentZ_Force[3] = {0.0,0.0,0.0};
  su2double AxiFactor;

#ifdef HAVE_MPI
  su2double MyAllBound_CD_Mnt, MyAllBound_CL_Mnt, MyAllBound_CSF_Mnt,
  MyAllBound_CMx_Mnt, MyAllBound_CMy_Mnt, MyAllBound_CMz_Mnt,
  MyAllBound_CoPx_Mnt, MyAllBound_CoPy_Mnt, MyAllBound_CoPz_Mnt,
  MyAllBound_CFx_Mnt, MyAllBound_CFy_Mnt, MyAllBound_CFz_Mnt, MyAllBound_CT_Mnt,
  MyAllBound_CQ_Mnt,
  *MySurface_CL_Mnt = NULL, *MySurface_CD_Mnt = NULL, *MySurface_CSF_Mnt = NULL,
  *MySurface_CEff_Mnt = NULL, *MySurface_CFx_Mnt = NULL, *MySurface_CFy_Mnt = NULL,
  *MySurface_CFz_Mnt = NULL,
  *MySurface_CMx_Mnt = NULL, *MySurface_CMy_Mnt = NULL,  *MySurface_CMz_Mnt = NULL;
#endif

  su2double Alpha     = config->GetAoA()*PI_NUMBER/180.0;
  su2double Beta      = config->GetAoS()*PI_NUMBER/180.0;
  su2double RefArea   = config->GetRefArea();
  su2double RefLength = config->GetRefLength();
  su2double *Origin = NULL;
  if (config->GetnMarker_Monitoring() != 0){
    Origin = config->GetRefOriginMoment(0);
  }
  bool axisymmetric          = config->GetAxisymmetric();

  /*--- Evaluate reference values for non-dimensionalization.
   For dimensional or non-dim based on initial values, use
   the far-field state (inf). For a custom non-dim based
   on user-provided reference values, use the ref values
   to compute the forces. ---*/

  if ((config->GetRef_Inc_NonDim() == DIMENSIONAL) || 
      (config->GetRef_Inc_NonDim() == INITIAL_VALUES)) {
    RefDensity  = Density_Inf;
    RefVel2 = 0.0;
    for (iDim = 0; iDim < nDim; iDim++)
      RefVel2  += Velocity_Inf[iDim]*Velocity_Inf[iDim];
  }
  else if (config->GetRef_Inc_NonDim() == REFERENCE_VALUES) {
    RefDensity = config->GetInc_Density_Ref();
    RefVel2    = config->GetInc_Velocity_Ref()*config->GetInc_Velocity_Ref();
  }

  /*--- Compute factor for force coefficients. ---*/

  factor = 1.0 / (0.5*RefDensity*RefArea*RefVel2);

  /*-- Variables initialization ---*/

  AllBound_CD_Mnt = 0.0;        AllBound_CL_Mnt = 0.0; AllBound_CSF_Mnt = 0.0;
  AllBound_CMx_Mnt = 0.0;          AllBound_CMy_Mnt = 0.0;   AllBound_CMz_Mnt = 0.0;
  AllBound_CoPx_Mnt = 0.0;          AllBound_CoPy_Mnt = 0.0;   AllBound_CoPz_Mnt = 0.0;
  AllBound_CFx_Mnt = 0.0;          AllBound_CFy_Mnt = 0.0;   AllBound_CFz_Mnt = 0.0;
  AllBound_CT_Mnt = 0.0;           AllBound_CQ_Mnt = 0.0;    AllBound_CMerit_Mnt = 0.0;
  AllBound_CEff_Mnt = 0.0;

  for (iMarker_Monitoring = 0; iMarker_Monitoring < config->GetnMarker_Monitoring(); iMarker_Monitoring++) {
    Surface_CL_Mnt[iMarker_Monitoring]      = 0.0; Surface_CD_Mnt[iMarker_Monitoring]      = 0.0;
    Surface_CSF_Mnt[iMarker_Monitoring] = 0.0; Surface_CEff_Mnt[iMarker_Monitoring]       = 0.0;
    Surface_CFx_Mnt[iMarker_Monitoring]        = 0.0; Surface_CFy_Mnt[iMarker_Monitoring]        = 0.0;
    Surface_CFz_Mnt[iMarker_Monitoring]        = 0.0;
    Surface_CMx_Mnt[iMarker_Monitoring]        = 0.0; Surface_CMy_Mnt[iMarker_Monitoring]        = 0.0; Surface_CMz_Mnt[iMarker_Monitoring]        = 0.0;
  }

  /*--- Loop over the Inlet / Outlet Markers  ---*/

  for (iMarker = 0; iMarker < nMarker; iMarker++) {

    Boundary   = config->GetMarker_All_KindBC(iMarker);
    Monitoring = config->GetMarker_All_Monitoring(iMarker);

    /*--- Obtain the origin for the moment computation for a particular marker ---*/

    if (Monitoring == YES) {
      for (iMarker_Monitoring = 0; iMarker_Monitoring < config->GetnMarker_Monitoring(); iMarker_Monitoring++) {
        Monitoring_Tag = config->GetMarker_Monitoring_TagBound(iMarker_Monitoring);
        Marker_Tag = config->GetMarker_All_TagBound(iMarker);
        if (Marker_Tag == Monitoring_Tag)
          Origin = config->GetRefOriginMoment(iMarker_Monitoring);
      }
    }

    if ((Boundary == INLET_FLOW) || (Boundary == OUTLET_FLOW) ||
        (Boundary == ACTDISK_INLET) || (Boundary == ACTDISK_OUTLET)||
        (Boundary == ENGINE_INFLOW) || (Boundary == ENGINE_EXHAUST)) {

      /*--- Forces initialization at each Marker ---*/

      CD_Mnt[iMarker] = 0.0;        CL_Mnt[iMarker] = 0.0; CSF_Mnt[iMarker] = 0.0;
      CMx_Mnt[iMarker] = 0.0;          CMy_Mnt[iMarker] = 0.0;   CMz_Mnt[iMarker] = 0.0;
      CFx_Mnt[iMarker] = 0.0;          CFy_Mnt[iMarker] = 0.0;   CFz_Mnt[iMarker] = 0.0;
      CoPx_Mnt[iMarker] = 0.0;         CoPy_Mnt[iMarker] = 0.0;  CoPz_Mnt[iMarker] = 0.0;
      CT_Mnt[iMarker] = 0.0;           CQ_Mnt[iMarker] = 0.0;    CMerit_Mnt[iMarker] = 0.0;
      CEff_Mnt[iMarker] = 0.0;

      for (iDim = 0; iDim < nDim; iDim++) ForceMomentum[iDim] = 0.0;
      MomentMomentum[0] = 0.0; MomentMomentum[1] = 0.0; MomentMomentum[2] = 0.0;
      MomentX_Force[0] = 0.0; MomentX_Force[1] = 0.0; MomentX_Force[2] = 0.0;
      MomentY_Force[0] = 0.0; MomentY_Force[1] = 0.0; MomentY_Force[2] = 0.0;
      MomentZ_Force[0] = 0.0; MomentZ_Force[1] = 0.0; MomentZ_Force[2] = 0.0;

      /*--- Loop over the vertices to compute the forces ---*/

      for (iVertex = 0; iVertex < geometry->GetnVertex(iMarker); iVertex++) {

        iPoint = geometry->vertex[iMarker][iVertex]->GetNode();

        /*--- Note that the pressure coefficient is computed at the
         halo cells (for visualization purposes), but not the forces ---*/

        if ( (geometry->node[iPoint]->GetDomain()) && (Monitoring == YES) ) {

          Normal = geometry->vertex[iMarker][iVertex]->GetNormal();
          Coord = geometry->node[iPoint]->GetCoord();
          Density   = node[iPoint]->GetDensity();

          Area = 0.0;
          for (iDim = 0; iDim < nDim; iDim++)
            Area += Normal[iDim]*Normal[iDim];
          Area = sqrt(Area);

          MassFlow = 0.0;
          for (iDim = 0; iDim < nDim; iDim++) {
            Velocity[iDim]   = node[iPoint]->GetVelocity(iDim);
            MomentDist[iDim] = Coord[iDim] - Origin[iDim];
            MassFlow -= Normal[iDim]*Velocity[iDim]*Density;
          }

          /*--- Axisymmetric simulations ---*/

          if (axisymmetric) AxiFactor = 2.0*PI_NUMBER*geometry->node[iPoint]->GetCoord(1);
          else AxiFactor = 1.0;

          /*--- Force computation, note the minus sign due to the
           orientation of the normal (outward) ---*/

          for (iDim = 0; iDim < nDim; iDim++) {
            Force[iDim] = MassFlow * Velocity[iDim] * factor * AxiFactor;
            ForceMomentum[iDim] += Force[iDim];
          }

          /*--- Moment with respect to the reference axis ---*/

          if (iDim == 3) {
            MomentMomentum[0] += (Force[2]*MomentDist[1]-Force[1]*MomentDist[2])/RefLength;
            MomentX_Force[1]  += (-Force[1]*Coord[2]);
            MomentX_Force[2]  += (Force[2]*Coord[1]);

            MomentMomentum[1] += (Force[0]*MomentDist[2]-Force[2]*MomentDist[0])/RefLength;
            MomentY_Force[2]  += (-Force[2]*Coord[0]);
            MomentY_Force[0]  += (Force[0]*Coord[2]);
          }
          MomentMomentum[2] += (Force[1]*MomentDist[0]-Force[0]*MomentDist[1])/RefLength;
          MomentZ_Force[0]  += (-Force[0]*Coord[1]);
          MomentZ_Force[1]  += (Force[1]*Coord[0]);

        }

      }

      /*--- Project forces and store the non-dimensional coefficients ---*/

      if (Monitoring == YES) {

        if (nDim == 2) {
          CD_Mnt[iMarker]  =  ForceMomentum[0]*cos(Alpha) + ForceMomentum[1]*sin(Alpha);
          CL_Mnt[iMarker]  = -ForceMomentum[0]*sin(Alpha) + ForceMomentum[1]*cos(Alpha);
          CEff_Mnt[iMarker]   = CL_Mnt[iMarker] / (CD_Mnt[iMarker]+EPS);
          CMz_Mnt[iMarker]    = MomentInviscid[2];
          CFx_Mnt[iMarker]    = ForceMomentum[0];
          CFy_Mnt[iMarker]    = ForceMomentum[1];
          CoPx_Mnt[iMarker]   = MomentZ_Force[1];
          CoPy_Mnt[iMarker]   = -MomentZ_Force[0];
          CT_Mnt[iMarker]     = -CFx_Mnt[iMarker];
          CQ_Mnt[iMarker]     = -CMz_Mnt[iMarker];
          CMerit_Mnt[iMarker] = CT_Mnt[iMarker] / (CQ_Mnt[iMarker] + EPS);
        }
        if (nDim == 3) {
          CD_Mnt[iMarker]      =  ForceMomentum[0]*cos(Alpha)*cos(Beta) + ForceMomentum[1]*sin(Beta) + ForceMomentum[2]*sin(Alpha)*cos(Beta);
          CL_Mnt[iMarker]      = -ForceMomentum[0]*sin(Alpha) + ForceMomentum[2]*cos(Alpha);
          CSF_Mnt[iMarker] = -ForceMomentum[0]*sin(Beta)*cos(Alpha) + ForceMomentum[1]*cos(Beta) - ForceMomentum[2]*sin(Beta)*sin(Alpha);
          CEff_Mnt[iMarker]       = CL_Mnt[iMarker] / (CD_Mnt[iMarker] + EPS);
          CMx_Mnt[iMarker]        = MomentInviscid[0];
          CMy_Mnt[iMarker]        = MomentInviscid[1];
          CMz_Mnt[iMarker]        = MomentInviscid[2];
          CFx_Mnt[iMarker]        = ForceMomentum[0];
          CFy_Mnt[iMarker]        = ForceMomentum[1];
          CFz_Mnt[iMarker]        = ForceMomentum[2];
          CoPx_Mnt[iMarker]       = -MomentY_Force[0];
          CoPz_Mnt[iMarker]       =  MomentY_Force[2];
          CT_Mnt[iMarker]         = -CFz_Mnt[iMarker];
          CQ_Mnt[iMarker]         = -CMz_Mnt[iMarker];
          CMerit_Mnt[iMarker]     = CT_Mnt[iMarker] / (CQ_Mnt[iMarker] + EPS);
        }

        AllBound_CD_Mnt        += CD_Mnt[iMarker];
        AllBound_CL_Mnt        += CL_Mnt[iMarker];
        AllBound_CSF_Mnt   += CSF_Mnt[iMarker];
        AllBound_CEff_Mnt          = AllBound_CL_Mnt / (AllBound_CD_Mnt + EPS);
        AllBound_CMx_Mnt          += CMx_Mnt[iMarker];
        AllBound_CMy_Mnt          += CMy_Mnt[iMarker];
        AllBound_CMz_Mnt          += CMz_Mnt[iMarker];
        AllBound_CFx_Mnt          += CFx_Mnt[iMarker];
        AllBound_CFy_Mnt          += CFy_Mnt[iMarker];
        AllBound_CFz_Mnt          += CFz_Mnt[iMarker];
        AllBound_CoPx_Mnt         += CoPx_Mnt[iMarker];
        AllBound_CoPy_Mnt         += CoPy_Mnt[iMarker];
        AllBound_CoPz_Mnt         += CoPz_Mnt[iMarker];
        AllBound_CT_Mnt           += CT_Mnt[iMarker];
        AllBound_CQ_Mnt           += CQ_Mnt[iMarker];
        AllBound_CMerit_Mnt        += AllBound_CT_Mnt / (AllBound_CQ_Mnt + EPS);

        /*--- Compute the coefficients per surface ---*/

        for (iMarker_Monitoring = 0; iMarker_Monitoring < config->GetnMarker_Monitoring(); iMarker_Monitoring++) {
          Monitoring_Tag = config->GetMarker_Monitoring_TagBound(iMarker_Monitoring);
          Marker_Tag = config->GetMarker_All_TagBound(iMarker);
          if (Marker_Tag == Monitoring_Tag) {
            Surface_CL_Mnt[iMarker_Monitoring]      += CL_Mnt[iMarker];
            Surface_CD_Mnt[iMarker_Monitoring]      += CD_Mnt[iMarker];
            Surface_CSF_Mnt[iMarker_Monitoring] += CSF_Mnt[iMarker];
            Surface_CEff_Mnt[iMarker_Monitoring]        = CL_Mnt[iMarker] / (CD_Mnt[iMarker] + EPS);
            Surface_CFx_Mnt[iMarker_Monitoring]        += CFx_Mnt[iMarker];
            Surface_CFy_Mnt[iMarker_Monitoring]        += CFy_Mnt[iMarker];
            Surface_CFz_Mnt[iMarker_Monitoring]        += CFz_Mnt[iMarker];
            Surface_CMx_Mnt[iMarker_Monitoring]        += CMx_Mnt[iMarker];
            Surface_CMy_Mnt[iMarker_Monitoring]        += CMy_Mnt[iMarker];
            Surface_CMz_Mnt[iMarker_Monitoring]        += CMz_Mnt[iMarker];
          }
        }

      }


    }
  }

#ifdef HAVE_MPI

  /*--- Add AllBound information using all the nodes ---*/

  MyAllBound_CD_Mnt        = AllBound_CD_Mnt;        AllBound_CD_Mnt = 0.0;
  MyAllBound_CL_Mnt        = AllBound_CL_Mnt;        AllBound_CL_Mnt = 0.0;
  MyAllBound_CSF_Mnt   = AllBound_CSF_Mnt;   AllBound_CSF_Mnt = 0.0;
  AllBound_CEff_Mnt = 0.0;
  MyAllBound_CMx_Mnt          = AllBound_CMx_Mnt;          AllBound_CMx_Mnt = 0.0;
  MyAllBound_CMy_Mnt          = AllBound_CMy_Mnt;          AllBound_CMy_Mnt = 0.0;
  MyAllBound_CMz_Mnt          = AllBound_CMz_Mnt;          AllBound_CMz_Mnt = 0.0;
  MyAllBound_CFx_Mnt          = AllBound_CFx_Mnt;          AllBound_CFx_Mnt = 0.0;
  MyAllBound_CFy_Mnt          = AllBound_CFy_Mnt;          AllBound_CFy_Mnt = 0.0;
  MyAllBound_CFz_Mnt          = AllBound_CFz_Mnt;          AllBound_CFz_Mnt = 0.0;
  MyAllBound_CoPx_Mnt         = AllBound_CoPx_Mnt;         AllBound_CoPx_Mnt = 0.0;
  MyAllBound_CoPy_Mnt         = AllBound_CoPy_Mnt;         AllBound_CoPy_Mnt = 0.0;
  MyAllBound_CoPz_Mnt         = AllBound_CoPz_Mnt;         AllBound_CoPz_Mnt = 0.0;
  MyAllBound_CT_Mnt           = AllBound_CT_Mnt;           AllBound_CT_Mnt = 0.0;
  MyAllBound_CQ_Mnt           = AllBound_CQ_Mnt;           AllBound_CQ_Mnt = 0.0;
  AllBound_CMerit_Mnt = 0.0;

  SU2_MPI::Allreduce(&MyAllBound_CD_Mnt, &AllBound_CD_Mnt, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(&MyAllBound_CL_Mnt, &AllBound_CL_Mnt, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(&MyAllBound_CSF_Mnt, &AllBound_CSF_Mnt, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  AllBound_CEff_Mnt = AllBound_CL_Mnt / (AllBound_CD_Mnt + EPS);
  SU2_MPI::Allreduce(&MyAllBound_CMx_Mnt, &AllBound_CMx_Mnt, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(&MyAllBound_CMy_Mnt, &AllBound_CMy_Mnt, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(&MyAllBound_CMz_Mnt, &AllBound_CMz_Mnt, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(&MyAllBound_CFx_Mnt, &AllBound_CFx_Mnt, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(&MyAllBound_CFy_Mnt, &AllBound_CFy_Mnt, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(&MyAllBound_CFz_Mnt, &AllBound_CFz_Mnt, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(&MyAllBound_CoPx_Mnt, &AllBound_CoPx_Mnt, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(&MyAllBound_CoPy_Mnt, &AllBound_CoPy_Mnt, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(&MyAllBound_CoPz_Mnt, &AllBound_CoPz_Mnt, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(&MyAllBound_CT_Mnt, &AllBound_CT_Mnt, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(&MyAllBound_CQ_Mnt, &AllBound_CQ_Mnt, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  AllBound_CMerit_Mnt = AllBound_CT_Mnt / (AllBound_CQ_Mnt + EPS);

  /*--- Add the forces on the surfaces using all the nodes ---*/

  MySurface_CL_Mnt      = new su2double[config->GetnMarker_Monitoring()];
  MySurface_CD_Mnt      = new su2double[config->GetnMarker_Monitoring()];
  MySurface_CSF_Mnt = new su2double[config->GetnMarker_Monitoring()];
  MySurface_CEff_Mnt       = new su2double[config->GetnMarker_Monitoring()];
  MySurface_CFx_Mnt        = new su2double[config->GetnMarker_Monitoring()];
  MySurface_CFy_Mnt        = new su2double[config->GetnMarker_Monitoring()];
  MySurface_CFz_Mnt        = new su2double[config->GetnMarker_Monitoring()];
  MySurface_CMx_Mnt        = new su2double[config->GetnMarker_Monitoring()];
  MySurface_CMy_Mnt        = new su2double[config->GetnMarker_Monitoring()];
  MySurface_CMz_Mnt        = new su2double[config->GetnMarker_Monitoring()];

  for (iMarker_Monitoring = 0; iMarker_Monitoring < config->GetnMarker_Monitoring(); iMarker_Monitoring++) {
    MySurface_CL_Mnt[iMarker_Monitoring]      = Surface_CL_Mnt[iMarker_Monitoring];
    MySurface_CD_Mnt[iMarker_Monitoring]      = Surface_CD_Mnt[iMarker_Monitoring];
    MySurface_CSF_Mnt[iMarker_Monitoring] = Surface_CSF_Mnt[iMarker_Monitoring];
    MySurface_CEff_Mnt[iMarker_Monitoring]       = Surface_CEff_Mnt[iMarker_Monitoring];
    MySurface_CFx_Mnt[iMarker_Monitoring]        = Surface_CFx_Mnt[iMarker_Monitoring];
    MySurface_CFy_Mnt[iMarker_Monitoring]        = Surface_CFy_Mnt[iMarker_Monitoring];
    MySurface_CFz_Mnt[iMarker_Monitoring]        = Surface_CFz_Mnt[iMarker_Monitoring];
    MySurface_CMx_Mnt[iMarker_Monitoring]        = Surface_CMx_Mnt[iMarker_Monitoring];
    MySurface_CMy_Mnt[iMarker_Monitoring]        = Surface_CMy_Mnt[iMarker_Monitoring];
    MySurface_CMz_Mnt[iMarker_Monitoring]        = Surface_CMz_Mnt[iMarker_Monitoring];

    Surface_CL_Mnt[iMarker_Monitoring]      = 0.0;
    Surface_CD_Mnt[iMarker_Monitoring]      = 0.0;
    Surface_CSF_Mnt[iMarker_Monitoring] = 0.0;
    Surface_CEff_Mnt[iMarker_Monitoring]       = 0.0;
    Surface_CFx_Mnt[iMarker_Monitoring]        = 0.0;
    Surface_CFy_Mnt[iMarker_Monitoring]        = 0.0;
    Surface_CFz_Mnt[iMarker_Monitoring]        = 0.0;
    Surface_CMx_Mnt[iMarker_Monitoring]        = 0.0;
    Surface_CMy_Mnt[iMarker_Monitoring]        = 0.0;
    Surface_CMz_Mnt[iMarker_Monitoring]        = 0.0;
  }

  SU2_MPI::Allreduce(MySurface_CL_Mnt, Surface_CL_Mnt, config->GetnMarker_Monitoring(), MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(MySurface_CD_Mnt, Surface_CD_Mnt, config->GetnMarker_Monitoring(), MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(MySurface_CSF_Mnt, Surface_CSF_Mnt, config->GetnMarker_Monitoring(), MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  for (iMarker_Monitoring = 0; iMarker_Monitoring < config->GetnMarker_Monitoring(); iMarker_Monitoring++)
    Surface_CEff_Mnt[iMarker_Monitoring] = Surface_CL_Mnt[iMarker_Monitoring] / (Surface_CD_Mnt[iMarker_Monitoring] + EPS);
  SU2_MPI::Allreduce(MySurface_CFx_Mnt, Surface_CFx_Mnt, config->GetnMarker_Monitoring(), MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(MySurface_CFy_Mnt, Surface_CFy_Mnt, config->GetnMarker_Monitoring(), MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(MySurface_CFz_Mnt, Surface_CFz_Mnt, config->GetnMarker_Monitoring(), MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(MySurface_CMx_Mnt, Surface_CMx_Mnt, config->GetnMarker_Monitoring(), MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(MySurface_CMy_Mnt, Surface_CMy_Mnt, config->GetnMarker_Monitoring(), MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(MySurface_CMz_Mnt, Surface_CMz_Mnt, config->GetnMarker_Monitoring(), MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

  delete [] MySurface_CL_Mnt; delete [] MySurface_CD_Mnt; delete [] MySurface_CSF_Mnt;
  delete [] MySurface_CEff_Mnt;  delete [] MySurface_CFx_Mnt;   delete [] MySurface_CFy_Mnt;
  delete [] MySurface_CFz_Mnt;
  delete [] MySurface_CMx_Mnt;   delete [] MySurface_CMy_Mnt;  delete [] MySurface_CMz_Mnt;

#endif

  /*--- Update the total coefficients (note that all the nodes have the same value) ---*/

  Total_CD            += AllBound_CD_Mnt;
  Total_CL            += AllBound_CL_Mnt;
  Total_CSF           += AllBound_CSF_Mnt;
  Total_CEff          = Total_CL / (Total_CD + EPS);
  Total_CMx           += AllBound_CMx_Mnt;
  Total_CMy           += AllBound_CMy_Mnt;
  Total_CMz           += AllBound_CMz_Mnt;
  Total_CFx           += AllBound_CFx_Mnt;
  Total_CFy           += AllBound_CFy_Mnt;
  Total_CFz           += AllBound_CFz_Mnt;
  Total_CoPx          += AllBound_CoPx_Mnt;
  Total_CoPy          += AllBound_CoPy_Mnt;
  Total_CoPz          += AllBound_CoPz_Mnt;
  Total_CT            += AllBound_CT_Mnt;
  Total_CQ            += AllBound_CQ_Mnt;
  Total_CMerit        = Total_CT / (Total_CQ + EPS);

  /*--- Update the total coefficients per surface (note that all the nodes have the same value)---*/

  for (iMarker_Monitoring = 0; iMarker_Monitoring < config->GetnMarker_Monitoring(); iMarker_Monitoring++) {
    Surface_CL[iMarker_Monitoring]   += Surface_CL_Mnt[iMarker_Monitoring];
    Surface_CD[iMarker_Monitoring]   += Surface_CD_Mnt[iMarker_Monitoring];
    Surface_CSF[iMarker_Monitoring]  += Surface_CSF_Mnt[iMarker_Monitoring];
    Surface_CEff[iMarker_Monitoring] += Surface_CL_Mnt[iMarker_Monitoring] / (Surface_CD_Mnt[iMarker_Monitoring] + EPS);
    Surface_CFx[iMarker_Monitoring]  += Surface_CFx_Mnt[iMarker_Monitoring];
    Surface_CFy[iMarker_Monitoring]  += Surface_CFy_Mnt[iMarker_Monitoring];
    Surface_CFz[iMarker_Monitoring]  += Surface_CFz_Mnt[iMarker_Monitoring];
    Surface_CMx[iMarker_Monitoring]  += Surface_CMx_Mnt[iMarker_Monitoring];
    Surface_CMy[iMarker_Monitoring]  += Surface_CMy_Mnt[iMarker_Monitoring];
    Surface_CMz[iMarker_Monitoring]  += Surface_CMz_Mnt[iMarker_Monitoring];
  }

}



void CPBIncEulerSolver::ExplicitEuler_Iteration(CGeometry *geometry, CSolver **solver_container, CConfig *config) {
  
  su2double *local_Residual, *local_Res_TruncError, Vol, Delta, Res,alfa,Mom_Coeff[3];
  unsigned short iVar, jVar;
  unsigned long iPoint;
  
  bool adjoint = config->GetContinuous_Adjoint();
  
  for (iVar = 0; iVar < nVar; iVar++) {
    SetRes_RMS(iVar, 0.0);
    SetRes_Max(iVar, 0.0, 0);
  }
  alfa = 0.9;
  /*--- Update the solution ---*/
  
  for (iPoint = 0; iPoint < nPointDomain; iPoint++) {
    Vol = geometry->node[iPoint]->GetVolume();
    Delta = node[iPoint]->GetDelta_Time() / Vol;
    
    local_Residual = LinSysRes.GetBlock(iPoint);
    
    if (!adjoint) {
      for (iVar = 0; iVar < nVar; iVar ++ ) {
        Res = local_Residual[iVar];
        node[iPoint]->AddSolution(iVar, -alfa*Res*Delta);
        AddRes_RMS(iVar, Res*Res);
        AddRes_Max(iVar, fabs(Res), geometry->node[iPoint]->GetGlobalIndex(), geometry->node[iPoint]->GetCoord());
        Mom_Coeff[iVar] = 1.0/(Delta*alfa);
      }
      node[iPoint]->Set_Mom_Coeff(Mom_Coeff);
    }
       
  }
  SetIterLinSolver(1);
  /*--- MPI solution ---*/
  Set_MPI_Solution(geometry, config);
  
  /*--- Compute the root mean square residual ---*/
  
  SetResidual_RMS(geometry, config);
  
}


void CPBIncEulerSolver::ImplicitEuler_Iteration(CGeometry *geometry, CSolver **solver_container, CConfig *config) {

  unsigned short iVar;
  unsigned long iPoint, total_index, IterLinSol = 0;
  su2double Delta, *local_Res_TruncError, Vol,Mom_Coeff[3];

  bool adjoint = config->GetContinuous_Adjoint();

  /*--- Set maximum residual to zero ---*/

  for (iVar = 0; iVar < nVar; iVar++) {
    SetRes_RMS(iVar, 0.0);
    SetRes_Max(iVar, 0.0, 0);
  }

  /*--- Build implicit system ---*/

  for (iPoint = 0; iPoint < nPointDomain; iPoint++) {


    /*--- Read the volume ---*/

    Vol = geometry->node[iPoint]->GetVolume();

    /*--- Modify matrix diagonal to assure diagonal dominance ---*/

    if (node[iPoint]->GetDelta_Time() != 0.0) {
      Delta = Vol / node[iPoint]->GetDelta_Time();
      Jacobian.AddVal2Diag(iPoint, Delta);
    } else {
      Jacobian.SetVal2Diag(iPoint, 1.0);
      for (iVar = 0; iVar < nVar; iVar++) {
        total_index = iPoint*nVar + iVar;
        LinSysRes[total_index] = 0.0;
        local_Res_TruncError[iVar] = 0.0;
      }
    }

    /*--- Right hand side of the system (-Residual) and initial guess (x = 0) ---*/

    for (iVar = 0; iVar < nVar; iVar++) {
      total_index = iPoint*nVar + iVar;
      LinSysRes[total_index] = - (LinSysRes[total_index] );//+ local_Res_TruncError[iVar]);
      LinSysSol[total_index] = 0.0;
      AddRes_RMS(iVar, LinSysRes[total_index]*LinSysRes[total_index]);
      AddRes_Max(iVar, fabs(LinSysRes[total_index]), geometry->node[iPoint]->GetGlobalIndex(), geometry->node[iPoint]->GetCoord());
    }

  }
  
  /*--- Initialize residual and solution at the ghost points ---*/

  for (iPoint = nPointDomain; iPoint < nPoint; iPoint++) {
    for (iVar = 0; iVar < nVar; iVar++) {
      total_index = iPoint*nVar + iVar;
      LinSysRes[total_index] = 0.0;
      LinSysSol[total_index] = 0.0;
    }
  }

  /*--- Solve or smooth the linear system ---*/

  CSysSolve system;
  IterLinSol = system.Solve(Jacobian, LinSysRes, LinSysSol, geometry, config);

  /*--- The the number of iterations of the linear solver ---*/

  SetIterLinSolver(IterLinSol);

  /*--- Update solution (system written in terms of increments) ---*/

  if (!adjoint) {
    for (iPoint = 0; iPoint < nPointDomain; iPoint++) {
      for (iVar = 0; iVar < nVar; iVar++) {
        node[iPoint]->AddSolution(iVar, 0.5*LinSysSol[iPoint*nVar+iVar]);
        Mom_Coeff[iVar] = Jacobian.GetBlock(iPoint,iPoint,iVar,iVar);
      }
      node[iPoint]->Set_Mom_Coeff(Mom_Coeff);
    }
  }

  /*--- MPI solution ---*/

  Set_MPI_Solution(geometry, config);

  /*--- Compute the root mean square residual ---*/

  SetResidual_RMS(geometry, config);

}



void CPBIncEulerSolver::SetTime_Step(CGeometry *geometry, CSolver **solver_container, CConfig *config,
                                unsigned short iMesh, unsigned long Iteration) {

  su2double *Normal, Area, Vol, Mean_SoundSpeed = 0.0, Mean_ProjVel = 0.0,
  Mean_BetaInc2, Lambda, Local_Delta_Time, Mean_DensityInc,
  Global_Delta_Time = 1E6, Global_Delta_UnstTimeND, ProjVel, ProjVel_i, ProjVel_j;

  unsigned long iEdge, iVertex, iPoint, jPoint;
  unsigned short iDim, iMarker;

  bool implicit      = (config->GetKind_TimeIntScheme_Flow() == EULER_IMPLICIT);
  bool grid_movement = config->GetGrid_Movement();
  bool time_steping  = config->GetUnsteady_Simulation() == TIME_STEPPING;
  bool dual_time     = ((config->GetUnsteady_Simulation() == DT_STEPPING_1ST) ||
                    (config->GetUnsteady_Simulation() == DT_STEPPING_2ND));

  Min_Delta_Time = 1.E6; Max_Delta_Time = 0.0;

  /*--- Set maximum inviscid eigenvalue to zero, and compute sound speed ---*/

  for (iPoint = 0; iPoint < nPointDomain; iPoint++)
    node[iPoint]->SetMax_Lambda_Inv(0.0);

  /*--- Loop interior edges ---*/

  for (iEdge = 0; iEdge < geometry->GetnEdge(); iEdge++) {

    /*--- Point identification, Normal vector and area ---*/

    iPoint = geometry->edge[iEdge]->GetNode(0);
    jPoint = geometry->edge[iEdge]->GetNode(1);

    Normal = geometry->edge[iEdge]->GetNormal();

    Area = 0.0;
    for (iDim = 0; iDim < nDim; iDim++) Area += Normal[iDim]*Normal[iDim];
    Area = sqrt(Area);
    
    /*--- Adjustment for grid movement ---

    if (grid_movement) {
      su2double *GridVel_i = geometry->node[iPoint]->GetGridVel();
      su2double *GridVel_j = geometry->node[jPoint]->GetGridVel();
      ProjVel_i = 0.0; ProjVel_j = 0.0;
      for (iDim = 0; iDim < nDim; iDim++) {
        ProjVel_i += GridVel_i[iDim]*Normal[iDim];
        ProjVel_j += GridVel_j[iDim]*Normal[iDim];
      }
      Mean_ProjVel -= 0.5 * (ProjVel_i + ProjVel_j);
    }*/
    
    
    /*--- Compute flux across the cell ---*/
    Mean_DensityInc = 0.5*(node[iPoint]->GetDensity() + node[jPoint]->GetDensity());
    Lambda = 0.0;
    Mean_ProjVel = 0.0;
    for (iDim = 0; iDim < nDim; iDim++)
       Mean_ProjVel += (0.5*(node[iPoint]->GetVelocity(iDim) + node[jPoint]->GetVelocity(iDim))*Normal[iDim]);
    
    Lambda = abs(Mean_DensityInc*Mean_ProjVel*Mean_ProjVel);
    

    /*--- Inviscid contribution ---*/

    if (geometry->node[iPoint]->GetDomain()) node[iPoint]->AddMax_Lambda_Inv(Lambda+EPS);
    if (geometry->node[jPoint]->GetDomain()) node[jPoint]->AddMax_Lambda_Inv(Lambda+EPS);

  }

  /*--- Loop boundary edges ---*/

  for (iMarker = 0; iMarker < geometry->GetnMarker(); iMarker++) {
    for (iVertex = 0; iVertex < geometry->GetnVertex(iMarker); iVertex++) {

      /*--- Point identification, Normal vector and area ---*/

      iPoint = geometry->vertex[iMarker][iVertex]->GetNode();
      Normal = geometry->vertex[iMarker][iVertex]->GetNormal();

      Area = 0.0;
      for (iDim = 0; iDim < nDim; iDim++) Area += Normal[iDim]*Normal[iDim];
      Area = sqrt(Area);

      /*--- Adjustment for grid movement ---*/

      if (grid_movement) {
        su2double *GridVel = geometry->node[iPoint]->GetGridVel();
        ProjVel = 0.0;
        for (iDim = 0; iDim < nDim; iDim++)
          ProjVel += GridVel[iDim]*Normal[iDim];
        Mean_ProjVel -= ProjVel;
      }

       /*--- Compute flux across the cell ---*/
    Mean_DensityInc = node[iPoint]->GetDensity() ;
    Lambda = 0.0;
    Mean_ProjVel = 0.0;
    for (iDim = 0; iDim < nDim; iDim++)
       Mean_ProjVel += node[iPoint]->GetVelocity(iDim)*Normal[iDim];
    
    Lambda = abs(Mean_DensityInc*Mean_ProjVel*Mean_ProjVel);
      if (geometry->node[iPoint]->GetDomain()) {
        node[iPoint]->AddMax_Lambda_Inv(Lambda+EPS);
      }

    }
  }

  /*--- Local time-stepping: each element uses their own speed for steady state
   simulations or for pseudo time steps in a dual time simulation. ---*/

  for (iPoint = 0; iPoint < nPointDomain; iPoint++) {

    Vol = geometry->node[iPoint]->GetVolume();

    if (Vol != 0.0) {
      Local_Delta_Time  = config->GetCFL(iMesh)*Vol / node[iPoint]->GetMax_Lambda_Inv();
      Global_Delta_Time = min(Global_Delta_Time, Local_Delta_Time);
      Min_Delta_Time    = min(Min_Delta_Time, Local_Delta_Time);
      Max_Delta_Time    = max(Max_Delta_Time, Local_Delta_Time);
      if (Local_Delta_Time > config->GetMax_DeltaTime())
        Local_Delta_Time = config->GetMax_DeltaTime();
        Local_Delta_Time = 1.0e-6*config->GetCFL(iMesh);
        node[iPoint]->SetDelta_Time(Local_Delta_Time);
      }
      else {
        node[iPoint]->SetDelta_Time(0.0);
      }
  }
  //cout<<"Momentum"<<endl;
  //cout<<"Min dt: "<<Min_Delta_Time<<", Max dt: "<<Max_Delta_Time<<endl;

  /*--- Compute the max and the min dt (in parallel) ---*/

  if (config->GetConsole_Output_Verb() == VERB_HIGH) {
#ifdef HAVE_MPI
    su2double rbuf_time, sbuf_time;
    sbuf_time = Min_Delta_Time;
    SU2_MPI::Reduce(&sbuf_time, &rbuf_time, 1, MPI_DOUBLE, MPI_MIN, MASTER_NODE, MPI_COMM_WORLD);
    SU2_MPI::Bcast(&rbuf_time, 1, MPI_DOUBLE, MASTER_NODE, MPI_COMM_WORLD);
    Min_Delta_Time = rbuf_time;

    sbuf_time = Max_Delta_Time;
    SU2_MPI::Reduce(&sbuf_time, &rbuf_time, 1, MPI_DOUBLE, MPI_MAX, MASTER_NODE, MPI_COMM_WORLD);
    SU2_MPI::Bcast(&rbuf_time, 1, MPI_DOUBLE, MASTER_NODE, MPI_COMM_WORLD);
    Max_Delta_Time = rbuf_time;
#endif
  }

  /*--- For time-accurate simulations use the minimum delta time of the whole mesh (global) ---*/

  if (time_steping) {
#ifdef HAVE_MPI
    su2double rbuf_time, sbuf_time;
    sbuf_time = Global_Delta_Time;
    SU2_MPI::Reduce(&sbuf_time, &rbuf_time, 1, MPI_DOUBLE, MPI_MIN, MASTER_NODE, MPI_COMM_WORLD);
    SU2_MPI::Bcast(&rbuf_time, 1, MPI_DOUBLE, MASTER_NODE, MPI_COMM_WORLD);
    Global_Delta_Time = rbuf_time;
#endif
    for (iPoint = 0; iPoint < nPointDomain; iPoint++){

      /*--- Sets the regular CFL equal to the unsteady CFL ---*/

      config->SetCFL(iMesh,config->GetUnst_CFL());

      /*--- If the unsteady CFL is set to zero, it uses the defined unsteady time step, otherwise
       it computes the time step based on the unsteady CFL ---*/

      if (config->GetCFL(iMesh) == 0.0){
        node[iPoint]->SetDelta_Time(config->GetDelta_UnstTime());
      } else {
        node[iPoint]->SetDelta_Time(Global_Delta_Time);
      }
    }
  }

  /*--- Recompute the unsteady time step for the dual time strategy
   if the unsteady CFL is diferent from 0 ---*/

  if ((dual_time) && (Iteration == 0) && (config->GetUnst_CFL() != 0.0) && (iMesh == MESH_0)) {
    Global_Delta_UnstTimeND = config->GetUnst_CFL()*Global_Delta_Time/config->GetCFL(iMesh);

#ifdef HAVE_MPI
    su2double rbuf_time, sbuf_time;
    sbuf_time = Global_Delta_UnstTimeND;
    SU2_MPI::Reduce(&sbuf_time, &rbuf_time, 1, MPI_DOUBLE, MPI_MIN, MASTER_NODE, MPI_COMM_WORLD);
    SU2_MPI::Bcast(&rbuf_time, 1, MPI_DOUBLE, MASTER_NODE, MPI_COMM_WORLD);
    Global_Delta_UnstTimeND = rbuf_time;
#endif
    config->SetDelta_UnstTimeND(Global_Delta_UnstTimeND);
  }

  /*--- The pseudo local time (explicit integration) cannot be greater than the physical time ---*/

  if (dual_time)
    for (iPoint = 0; iPoint < nPointDomain; iPoint++) {
      if (!implicit) {
        Local_Delta_Time = min((2.0/3.0)*config->GetDelta_UnstTimeND(), node[iPoint]->GetDelta_Time());
        node[iPoint]->SetDelta_Time(Local_Delta_Time);
      }
    }

}

void CPBIncEulerSolver::SetFreeStream_Solution(CConfig *config){

  unsigned long iPoint;
  unsigned short iDim;

  for (iPoint = 0; iPoint < nPoint; iPoint++){
    node[iPoint]->SetPressure_val(Pressure_Inf);
    for (iDim = 0; iDim < nDim; iDim++){
      node[iPoint]->SetSolution(iDim, Density_Inf*Velocity_Inf[iDim]);
    }
  }
}


void CPBIncEulerSolver::SetPoissonSourceTerm(CGeometry *geometry, CSolver **solver_container, CConfig *config) {
	
	
  unsigned short iVar, jVar, iDim, jDim, nGradVar,KindBC;
  unsigned long iPoint, jPoint, iEdge, iMarker, iVertex;
  su2double Edge_Vector[3], UnitNormal[3], proj_vector_ij, dist_ij, dist_ij_2;
  su2double *Coord_i, *Coord_j;
  su2double MassFlux_Part, Mom_Coeff[3], *Vel_i, *Vel_j,*Normal;
  su2double Criteria, Area, Vol;
  su2double Mom_Coeff_i[3], Mom_Coeff_j[3];
  su2double GradP_f[3], GradP_in[3], GradP_proj;
  su2double *Flow_Dir, Flow_Dir_Mag, Vel_Mag, Net_Mass, alfa, Mass_In, Mass_Out;
  string Marker_Tag;
  bool implicit = (config->GetKind_TimeIntScheme_Flow() == EULER_IMPLICIT);
  
  
  /*--- Initialize mass flux to zero ---*/
  for (iPoint = 0; iPoint < nPoint; iPoint++) node[iPoint]->SetMassFluxZero();
  
  /*--- Loop interior edges ---*/
  for (iEdge = 0; iEdge < geometry->GetnEdge(); iEdge++) {
    iPoint = geometry->edge[iEdge]->GetNode(0);
    jPoint = geometry->edge[iEdge]->GetNode(1);
    
    Normal = geometry->edge[iEdge]->GetNormal();
    Area = 0.0; for (iDim = 0; iDim < nDim; iDim++) Area += Normal[iDim]*Normal[iDim];
    Area = sqrt(Area);
    
	MassFlux_Part = 0.0;
    for (iDim = 0; iDim < nDim; iDim++) 
      MassFlux_Part += GetDensity_Inf()*(0.5*(node[iPoint]->GetVelocity(iDim)+node[jPoint]->GetVelocity(iDim)))*Normal[iDim];
	
	
	/*------- Rhie-Chow interpolation ---------*/
	
	dist_ij = 0; proj_vector_ij = 0; dist_ij_2 = 0.0;
    Coord_i = geometry->node[iPoint]->GetCoord();
    Coord_j = geometry->node[jPoint]->GetCoord();
	for (iDim = 0; iDim < nDim; iDim++) {
		Edge_Vector[iDim] = Coord_j[iDim]-Coord_i[iDim];
		dist_ij_2 += Edge_Vector[iDim]*Edge_Vector[iDim];
		proj_vector_ij += Edge_Vector[iDim]*Normal[iDim];
		UnitNormal[iDim] = Normal[iDim]/Area;
	}
	
	dist_ij = sqrt(dist_ij_2) ;
	if (dist_ij == 0.0) proj_vector_ij = 0.0;
	else proj_vector_ij = proj_vector_ij/dist_ij;
	
    for (iVar = 0; iVar < nVar; iVar++) {
	   Mom_Coeff_i[iVar] = node[iPoint]->Get_Mom_Coeff(iVar);
       Mom_Coeff_j[iVar] = node[jPoint]->Get_Mom_Coeff(iVar);
       
       Mom_Coeff_i[iVar] = node[iPoint]->GetDensity()*geometry->node[iPoint]->GetVolume()/Mom_Coeff_i[iVar];
	   Mom_Coeff_j[iVar] = node[jPoint]->GetDensity()*geometry->node[iPoint]->GetVolume()/Mom_Coeff_j[iVar];
	}

    /*--- Interpolate the pressure gradient based on node values ---*/
    for (iDim = 0; iDim < nDim; iDim++) 		
		GradP_in[iDim] = 0.5*(node[iPoint]->GetGradient_Primitive(0,iDim) + node[jPoint]->GetGradient_Primitive(0,iDim)) ;
		
   /*--- Compute pressure gradient at the face ---*/
    GradP_proj = 0.0;
    for (iDim = 0; iDim < nDim; iDim++) {
       GradP_proj += GradP_in[iDim]*Edge_Vector[iDim];
    }
    if (dist_ij != 0.0) {
      for (iDim = 0; iDim < nDim; iDim++) {
        GradP_f[iDim] = GradP_in[iDim] - (GradP_proj - (node[jPoint]->GetPressure() - node[iPoint]->GetPressure()))*Edge_Vector[iDim]/ dist_ij_2 ;
      }
    }    
   
    for (iDim = 0; iDim < nDim; iDim++) {
		MassFlux_Part -= 0.5*(Mom_Coeff_i[iDim] + Mom_Coeff_j[iDim])*(GradP_f[iDim] - GradP_in[iDim])*Normal[iDim] ;
	}
    
    
    if (geometry->node[iPoint]->GetDomain())
	  node[iPoint]->AddMassFlux(MassFlux_Part);
	if (geometry->node[jPoint]->GetDomain())
	  node[jPoint]->SubtractMassFlux(MassFlux_Part);
  }
    
  /*--- Mass flux correction for outflow ---*/
  Mass_In = 0.0; Mass_Out = 0.0;  
  
  /*--- Loop boundary edges ---*/
  for (iMarker = 0; iMarker < geometry->GetnMarker(); iMarker++) {
	
	KindBC = config->GetMarker_All_KindBC(iMarker);
	Marker_Tag  = config->GetMarker_All_TagBound(iMarker);
    
    switch (KindBC) {
		
		case EULER_WALL: case ISOTHERMAL: case HEAT_FLUX: 
		/*--- Wall boundaries have zero mass flux (irrespective of grid movement) ---*/
		case SYMMETRY_PLANE:
		  MassFlux_Part = 0.0 ;	 
		break;
		
		case INLET_FLOW:
		
		 for (iVertex = 0; iVertex < geometry->GetnVertex(iMarker); iVertex++) {
           iPoint = geometry->vertex[iMarker][iVertex]->GetNode();
           if (geometry->node[iPoint]->GetDomain()) {		
		     Normal = geometry->vertex[iMarker][iVertex]->GetNormal(); 
		     Area = 0.0; 
		     for (iDim = 0; iDim < nDim; iDim++) Area += Normal[iDim]*Normal[iDim];
		     Area = sqrt(Area);
		        
		     Vel_Mag  = config->GetInlet_Ptotal(Marker_Tag)/config->GetVelocity_Ref();

             Flow_Dir = config->GetInlet_FlowDir(Marker_Tag);
		  
             Flow_Dir_Mag = 0.0;
             for (iDim = 0; iDim < nDim; iDim++)
               Flow_Dir_Mag += Flow_Dir[iDim]*Flow_Dir[iDim];
             
             Flow_Dir_Mag = sqrt(Flow_Dir_Mag);
          
             for (iDim = 0; iDim < nDim; iDim++)
               Flow_Dir[iDim] = Flow_Dir[iDim]/Flow_Dir_Mag;
               
	      	 MassFlux_Part = 0.0;
             for (iDim = 0; iDim < nDim; iDim++) 
               MassFlux_Part += GetDensity_Inf()*Vel_Mag*Flow_Dir[iDim]*Area;

             node[iPoint]->SubtractMassFlux(MassFlux_Part);
             
             /*--- Sum up the mass flux entering to be used for mass flow correction at outflow ---*/
             Mass_In += fabs(MassFlux_Part);
		   }
         }
		
		break;
		
		case FAR_FIELD:
		
		 for (iVertex = 0; iVertex < geometry->GetnVertex(iMarker); iVertex++) {
           iPoint = geometry->vertex[iMarker][iVertex]->GetNode();
           if (geometry->node[iPoint]->GetDomain()) {		
		     Normal = geometry->vertex[iMarker][iVertex]->GetNormal(); 
		        
		     MassFlux_Part = 0.0;
		     for (iDim = 0; iDim < nDim; iDim++) 
               MassFlux_Part -= GetDensity_Inf()*(node[iPoint]->GetVelocity(iDim))*Normal[iDim];
             
             node[iPoint]->AddMassFlux(MassFlux_Part);             
		   }
         }
		
		
		break;
		
		case OUTLET_FLOW:
		 for (iVertex = 0; iVertex < geometry->GetnVertex(iMarker); iVertex++) {
           iPoint = geometry->vertex[iMarker][iVertex]->GetNode();
           if (geometry->node[iPoint]->GetDomain()) {		
		     Normal = geometry->vertex[iMarker][iVertex]->GetNormal(); 
		     
		     MassFlux_Part = 0.0;
             for (iDim = 0; iDim < nDim; iDim++) 
                MassFlux_Part -= GetDensity_Inf()*node[iPoint]->GetVelocity(iDim)*Normal[iDim];
             
              /*--- Sum up the mass flux leaving to be used for mass flow correction at outflow ---*/
              Mass_Out += fabs(MassFlux_Part);
              node[iPoint]->AddMassFlux(MassFlux_Part);
		   }
         }         
		break;
		
		default:
		  MassFlux_Part = 0.0 ;
		  node[iPoint]->AddMassFlux(MassFlux_Part);
		break;
		
	}
  }
  
  /*--- Mass flux correction for outflow ---*/
  /*--- Works only when outflow is fully developed ---*/
  /*for (iMarker = 0; iMarker < geometry->GetnMarker(); iMarker++) {
	
	KindBC = config->GetMarker_All_KindBC(iMarker);
	Marker_Tag  = config->GetMarker_All_TagBound(iMarker);
    
    if (KindBC == OUTLET_FLOW) {
		for (iVertex = 0; iVertex < geometry->GetnVertex(iMarker); iVertex++) {
           iPoint = geometry->vertex[iMarker][iVertex]->GetNode();
           if (geometry->node[iPoint]->GetDomain()) {		
		     Normal = geometry->vertex[iMarker][iVertex]->GetNormal(); 
		     
		     MassFlux_Part = 0.0;
             for (iDim = 0; iDim < nDim; iDim++) 
                MassFlux_Part -= GetDensity_Inf()*node[iPoint]->GetVelocity(iDim)*Normal[iDim];
            
             MassFlux_Part = MassFlux_Part*(Mass_In/Mass_Out);
             
             node[iPoint]->AddMassFlux(MassFlux_Part);
		   }
        }
	}
  }*/
  
  ResMassFlux = 0.0;
  Net_Mass = 0.0;
  for (iPoint = 0; iPoint < nPoint; iPoint++) {
	  ResMassFlux += node[iPoint]->GetMassFlux()*node[iPoint]->GetMassFlux();
  } 
  
  ResMassFlux = ResMassFlux/geometry->GetnPoint();
  ResMassFlux = sqrt(ResMassFlux);
  
  Criteria = 1.0e-7;
}


void CPBIncEulerSolver:: Flow_Correction(CGeometry *geometry, CSolver **solver_container, CConfig *config){
	
  unsigned long iEdge, iPoint, jPoint, iMarker, iVertex, Point_Normal;
  unsigned short iDim, iVar, KindBC;
  su2double **vel_corr, vel_corr_i, vel_corr_j, vel_corr_avg;
  su2double Edge_Vec[3], alpha_vel,dist_ij_2;
  su2double *Normal, Area, Vel, Vel_Mag,rho,*Coeff;
  su2double *Pressure_Correc, Current_Pressure, factor, *Flow_Dir;
  su2double alpha_p;//This should be config->getrelaxation (like)
  string Marker_Tag;
  su2double *Coord_i, *Coord_j, dist_ij, delP, Pressure_j, Pressure_i, PCorr_Ref;
	
  /*--- Allocate corrections ---*/
  vel_corr = new su2double* [nPoint];
  Pressure_Correc = new su2double [nPoint];
	
  
  /*--- Pressure Corrections ---*/
	
	for (iPoint = 0; iPoint < nPoint; iPoint++) 
		Pressure_Correc[iPoint] = solver_container[POISSON_SOL]->node[iPoint]->GetSolution(0);
    
    PCorr_Ref = 0.0;//solver_container[POISSON_SOL]->node[PRef_Point]->GetSolution(0);
	

  /*--- Velocity Corrections ---*/
  for (iPoint = 0; iPoint < nPoint; iPoint++) 
    vel_corr[iPoint] = new su2double [nVar];
    
  for (iPoint = 0; iPoint < nPoint; iPoint++)
	for (iVar = 0; iVar < nVar; iVar++)
	    vel_corr[iPoint][iVar] = 0.0;

  for (iPoint = 0; iPoint < nPoint; iPoint++) {
	  
	   for (iVar = 0; iVar < nVar; iVar++) {
			factor = geometry->node[iPoint]->GetVolume()/node[iPoint]->Get_Mom_Coeff(iVar);
			vel_corr[iPoint][iVar] = factor*(solver_container[POISSON_SOL]->node[iPoint]->GetGradient(0,iVar));
		}
   }
		
  /*--- Reassign strong boundary conditions ---*/
  /*--- For now I only have velocity inlet and fully developed outlet. 
   * Will need to add other types of inlet/outlet conditions
   *  where different treatment of pressure might be needed. ---*/
  for (iMarker = 0; iMarker < geometry->GetnMarker(); iMarker++) {
	
	  KindBC = config->GetMarker_All_KindBC(iMarker);
	  Marker_Tag  = config->GetMarker_All_TagBound(iMarker);
    
    switch (KindBC) {		
		case OUTLET_FLOW: 
		 for (iVertex = 0; iVertex < geometry->GetnVertex(iMarker); iVertex++) {
           iPoint = geometry->vertex[iMarker][iVertex]->GetNode();
           Pressure_Correc[iPoint] = PCorr_Ref;
	      }
		break;
		
		case INLET_FLOW:
		case ISOTHERMAL: case HEAT_FLUX:
		for (iVertex = 0; iVertex < geometry->GetnVertex(iMarker); iVertex++) {
           iPoint = geometry->vertex[iMarker][iVertex]->GetNode();
           for (iDim = 0; iDim < nDim; iDim++)
                vel_corr[iPoint][iDim] = 0.0;
	      }
		break;

		default: break;
		
	}
  }
  
  alpha_p = 0.5;
  for (iPoint = 0; iPoint < nPoint; iPoint++) {
	  for (iVar = 0; iVar < nVar; iVar++) {
           Vel = node[iPoint]->GetVelocity(iVar);
           Vel = Vel - vel_corr[iPoint][iVar];
           rho = node[iPoint]->GetDensity();
           Vel = rho*Vel;
           node[iPoint]->SetSolution(iVar,Vel);
           factor = geometry->node[iPoint]->GetVolume()/node[iPoint]->Get_Mom_Coeff(iVar);
		}
		Current_Pressure = solver_container[FLOW_SOL]->node[iPoint]->GetPressure();
		Current_Pressure += alpha_p*(Pressure_Correc[iPoint] - PCorr_Ref);
		node[iPoint]->SetPressure_val(Current_Pressure);
   }
   
   /*--- Reset pressure corrections to zero for next iteration. ---*/
   for (iPoint = 0; iPoint < nPoint; iPoint++) {
	   solver_container[POISSON_SOL]->node[iPoint]->SetSolution(0,0.0);
   }
	
	for (iPoint = 0; iPoint < nPoint; iPoint++)
	  delete [] vel_corr[iPoint];
	  
	delete [] vel_corr;
	delete [] Pressure_Correc;
}

void CPBIncEulerSolver::BC_Euler_Wall(CGeometry *geometry, CSolver **solver_container,
                                 CNumerics *numerics, CConfig *config, unsigned short val_marker) {
  unsigned short iDim, iVar, jVar;
  unsigned long iPoint, iVertex;

  su2double Density = 0.0, Pressure = 0.0, *Normal = NULL, Area, *NormalArea, turb_ke;
  
  bool implicit = (config->GetKind_TimeIntScheme_Flow() == EULER_IMPLICIT);
  /*bool tkeNeeded = (((config->GetKind_Solver() == RANS ) ||
                     (config->GetKind_Solver() == DISC_ADJ_RANS)) &&
                    (config->GetKind_Turb_Model() == SST));*/
  
  Normal     = new su2double[nDim];
  NormalArea = new su2double[nDim];

  /*--- Loop over all the vertices on this boundary marker ---*/
  
  for (iVertex = 0; iVertex < geometry->nVertex[val_marker]; iVertex++) {
    iPoint = geometry->vertex[val_marker][iVertex]->GetNode();
    
    /*--- Check if the node belongs to the domain (i.e, not a halo node) ---*/
    
    if (geometry->node[iPoint]->GetDomain()) {
      
      /*--- Normal vector for this vertex (negative for outward convention) ---*/
      
      geometry->vertex[val_marker][iVertex]->GetNormal(Normal);
      
      Area = 0.0;
      for (iDim = 0; iDim < nDim; iDim++) Area += Normal[iDim]*Normal[iDim];
      Area = sqrt (Area);
      
      for (iDim = 0; iDim < nDim; iDim++) {
        NormalArea[iDim] = -Normal[iDim];
      }

      /*--- Compute the residual ---*/

      Pressure = node[iPoint]->GetPressure();
      Density  = node[iPoint]->GetDensity();

      for (iDim = 0; iDim < nDim; iDim++)
        Residual[iDim] = Pressure*NormalArea[iDim];

      /*--- Add the Reynolds stress tensor contribution ---*/

      /*if (tkeNeeded) {
        turb_ke = solver_container[TURB_SOL]->node[iPoint]->GetSolution(0);
        for (iDim = 0; iDim < nDim; iDim++)
          Residual[iDim+1] += (2.0/3.0)*Density*turb_ke*NormalArea[iDim];
      }*/

      /*--- Add value to the residual ---*/

      LinSysRes.AddBlock(iPoint, Residual);
      
      /*--- Form Jacobians for implicit computations ---*/
      
      if (implicit) {
        
        /*--- Initialize Jacobian ---*/
        
        for (iVar = 0; iVar < nVar; iVar++) {
          for (jVar = 0; jVar < nVar; jVar++)
            Jacobian_i[iVar][jVar] = 0.0;
        }
        
        Jacobian.AddBlock(iPoint, iPoint, Jacobian_i);

      }
    }
  }
  
  delete [] Normal;
  delete [] NormalArea;
    
}

void CPBIncEulerSolver::BC_Far_Field(CGeometry *geometry, CSolver **solver_container, CNumerics *conv_numerics,
                                CNumerics *visc_numerics, CConfig *config, unsigned short val_marker) {
  
  unsigned short iDim, jDim, iVar;
  unsigned long iVertex, iPoint, Point_Normal, total_index;
  
  su2double *V_infty, *V_domain;
  su2double Face_Flux, Flux0, Flux1, MeanDensity, proj_vel;
  
  bool implicit       = config->GetKind_TimeIntScheme_Flow() == EULER_IMPLICIT;
  bool grid_movement  = config->GetGrid_Movement();
  bool viscous        = config->GetViscous();
  
  su2double *Normal = new su2double[nDim];
  su2double *MeanVelocity = new su2double[nDim];
  su2double *val_normal = new su2double[nDim];
  su2double *V_free = new su2double[nDim];

  /*--- Loop over all the vertices on this boundary marker ---*/
  
  for (iVertex = 0; iVertex < geometry->nVertex[val_marker]; iVertex++) {
    iPoint = geometry->vertex[val_marker][iVertex]->GetNode();

   /*--- Check if the node belongs to the domain (i.e, not a halo node) ---*/
    
    if (geometry->node[iPoint]->GetDomain()) {
		
		
	  geometry->vertex[val_marker][iVertex]->GetNormal(Normal);
      
      /*--- Retrieve solution at the farfield boundary node ---*/
      
      V_domain = node[iPoint]->GetPrimitive();
      
      /*--- Index of the closest interior node ---*/
      
      Point_Normal = geometry->vertex[val_marker][iVertex]->GetNormal_Neighbor();
        
      if (grid_movement)
        conv_numerics->SetGridVel(geometry->node[iPoint]->GetGridVel(),
                                  geometry->node[iPoint]->GetGridVel());
      
      Face_Flux = 0.0;
	  for (iDim = 0; iDim < nDim; iDim++) 
		Face_Flux -= V_domain[nDim+1]*V_domain[iDim+1]*Normal[iDim];
	  
	  Flux0 = 0.5*(Face_Flux + fabs(Face_Flux));
	  
      for (iVar = 0; iVar < nVar; iVar++) {
	     Residual[iVar] = Flux0*V_domain[iVar+1];
	  }
      
      for (iDim = 0; iDim < nDim; iDim++)
        Residual[iDim] -= node[iPoint]->GetPressure()*Normal[iDim];
    
	  LinSysRes.AddBlock(iPoint, Residual);
	  
	  if (Face_Flux > 0.0)
	  	  node[iPoint]->SetPressure_val(config->GetPRef_Value());
       
      /*--- Convective Jacobian contribution for implicit integration ---*/
      
      if (implicit) {
		  for (iDim = 0; iDim < nDim; iDim++)
		       for (jDim = 0; jDim < nDim; jDim++)
		          Jacobian_i[iDim][jDim] = 0.0;
		          
    	  proj_vel = 0.0;
		  for (iDim = 0; iDim < nDim; iDim++) {
			  val_normal[iDim] = -Normal[iDim];
			  proj_vel += V_domain[iDim+1]*val_normal[iDim];
		  }
			  
		  if (nDim == 2) {
			  Jacobian_i[0][0] = (V_domain[1]*val_normal[0] + proj_vel);
			  Jacobian_i[0][1] = V_domain[1]*val_normal[1];
			  
			  Jacobian_i[1][0] = V_domain[2]*val_normal[0];
			  Jacobian_i[1][1] = (V_domain[2]*val_normal[1] + proj_vel);
	       }
	       else {
			   Jacobian_i[0][0] = (proj_vel+V_domain[1]*val_normal[0]);
			   Jacobian_i[0][1] = (V_domain[1]*val_normal[1]);
			   Jacobian_i[0][2] = (V_domain[1]*val_normal[2]);
			   
			   Jacobian_i[1][0] = (V_domain[2]*val_normal[0]);
			   Jacobian_i[1][1] = (proj_vel+V_domain[2]*val_normal[1]);
			   Jacobian_i[1][2] = (V_domain[2]*val_normal[2]);
			   
			   Jacobian_i[2][0] = (V_domain[3]*val_normal[0]);
			   Jacobian_i[2][1] = (V_domain[3]*val_normal[1]);
			   Jacobian_i[2][2] = (proj_vel+V_domain[3]*val_normal[2]);
		   }
		  Jacobian.AddBlock(iPoint, iPoint, Jacobian_i);		  
      }     
      
      if (viscous) {

        /*--- Set transport properties at the outlet. ---*/

        V_domain[nDim+2] = node[iPoint]->GetLaminarViscosity();
        V_domain[nDim+3] = node[iPoint]->GetEddyViscosity();

        /*--- Set the normal vector and the coordinates ---*/
        
        visc_numerics->SetNormal(Normal);
        visc_numerics->SetCoord(geometry->node[iPoint]->GetCoord(),
                                geometry->node[Point_Normal]->GetCoord());
        
        /*--- Primitive variables, and gradient ---*/
        
        visc_numerics->SetPrimitive(V_domain, V_domain);
        visc_numerics->SetPrimVarGradient(node[iPoint]->GetGradient_Primitive(),
                                          node[iPoint]->GetGradient_Primitive());
        
        /*--- Turbulent kinetic energy ---*/
        
        if (config->GetKind_Turb_Model() == SST)
          visc_numerics->SetTurbKineticEnergy(solver_container[TURB_SOL]->node[iPoint]->GetSolution(0),
                                              solver_container[TURB_SOL]->node[iPoint]->GetSolution(0));
        
        /*--- Compute and update residual ---*/
        
        visc_numerics->ComputeResidual(Residual, Jacobian_i, Jacobian_j, config);
        
        LinSysRes.AddBlock(iPoint, Residual);
        
        /*--- Jacobian contribution for implicit integration ---*/
        if (implicit)
          Jacobian.AddBlock(iPoint, iPoint, Jacobian_i);
        
      }
      
      
    }
  }
  
  /*--- Free locally allocated memory ---*/
  
  delete [] Normal;
  delete [] V_free;
  
}

void CPBIncEulerSolver::BC_Inlet(CGeometry *geometry, CSolver **solver_container,
                            CNumerics *conv_numerics, CNumerics *visc_numerics, CConfig *config, unsigned short val_marker) {
  unsigned short iDim,iVar;
  unsigned long iVertex, iPoint,total_index, Point_Normal;
  su2double *Flow_Dir, Vel_Mag, Area;
  su2double *V_inlet = new su2double[nDim];
  su2double *Coord_i, *Coord_j, dist_ij, delP, Pressure_j, Pressure_i;
  
  bool implicit      = (config->GetKind_TimeIntScheme_Flow() == EULER_IMPLICIT);
  bool grid_movement = config->GetGrid_Movement();
  string Marker_Tag  = config->GetMarker_All_TagBound(val_marker);

  su2double *Normal = new su2double[nDim];
  
  /*--- Loop over all the vertices on this boundary marker ---*/

  for (iVertex = 0; iVertex < geometry->nVertex[val_marker]; iVertex++) {
    
   
    iPoint = geometry->vertex[val_marker][iVertex]->GetNode();
    
    /*--- Check if the node belongs to the domain (i.e., not a halo node) ---*/
    
    if (geometry->node[iPoint]->GetDomain()) {
      
      /*--- Retrieve the specified velocity for the inlet. ---*/

      Vel_Mag  = config->GetInlet_Ptotal(Marker_Tag)/config->GetVelocity_Ref();
      Flow_Dir = config->GetInlet_FlowDir(Marker_Tag);
      
      /*--- Store the velocity in the primitive variable vector. ---*/

      for (iDim = 0; iDim < nDim; iDim++)
        V_inlet[iDim] = Vel_Mag*Flow_Dir[iDim];
  
       /*--- Impose the value of the velocity as a strong boundary condition (Dirichlet). 
        * Fix the velocity and remove any contribution to the residual at this node. ---*/
      
      node[iPoint]->SetVelocity_Old(V_inlet);
      
      for (iDim = 0; iDim < nDim; iDim++)
        LinSysRes.SetBlock_Zero(iPoint, iDim);
      /*--- Jacobian contribution for implicit integration ---*/
      
      if (implicit) {
        for (iDim = 0; iDim < nDim; iDim++) {
          total_index = iPoint*nVar+iDim;
          Jacobian.DeleteValsRowi(total_index);
        }
      }
    }
  }
  
  /*--- Free locally allocated memory ---*/
  
  delete [] Normal;
  delete [] V_inlet;
  
}

void CPBIncEulerSolver::BC_Outlet(CGeometry *geometry, CSolver **solver_container,
                             CNumerics *conv_numerics, CNumerics *visc_numerics, CConfig *config, unsigned short val_marker) {
  unsigned short iDim,iVar,jDim;
  unsigned long iVertex, iPoint, Point_Normal;
  su2double Area, yCoordRef, yCoord,proj_vel;
  su2double *V_outlet, *V_domain, P_Outlet, Face_Flux, Flux0;
  su2double *Coord_i, *Coord_j, dist_ij, delP, Pressure_j, Pressure_i;
  
  bool implicit      = (config->GetKind_TimeIntScheme_Flow() == EULER_IMPLICIT);
  bool viscous       = config->GetViscous();
  string Marker_Tag  = config->GetMarker_All_TagBound(val_marker);

  su2double *Normal = new su2double[nDim];
  su2double *val_normal = new su2double[nDim];
  /*--- Loop over all the vertices on this boundary marker ---*/

  for (iVertex = 0; iVertex < geometry->nVertex[val_marker]; iVertex++) {
    
   
    iPoint = geometry->vertex[val_marker][iVertex]->GetNode();
    
    /*--- Check if the node belongs to the domain (i.e., not a halo node) ---*/
    
    if (geometry->node[iPoint]->GetDomain()) {
      
      /*--- Normal vector for this vertex (negate for outward convention) ---*/
      
      geometry->vertex[val_marker][iVertex]->GetNormal(Normal);
      
      Area = 0.0;
      for (iDim = 0; iDim < nDim; iDim++) Area += Normal[iDim]*Normal[iDim];
      Area = sqrt (Area);
       
      /*--- Current solution at this boundary node ---*/
      
      V_domain = node[iPoint]->GetPrimitive();
      
      /*--- Retrieve the specified back pressure for this outlet. ---*/

      P_Outlet = config->GetOutlet_Pressure(Marker_Tag)/config->GetPressure_Ref();
 
      node[iPoint]->SetPressure_val(P_Outlet);
      
      for (iVar = 0; iVar < nVar; iVar++)   
         Residual[iVar] = 0.0;
      
      
      Face_Flux = 0.0;
	  for (iDim = 0; iDim < nDim; iDim++) 
		Face_Flux -= V_domain[nDim+1]*V_domain[iDim+1]*Normal[iDim];
	  
	  Flux0 = 0.5*(Face_Flux + fabs(Face_Flux));
	  
      for (iVar = 0; iVar < nVar; iVar++) {
	     Residual[iVar] = Flux0*V_domain[iVar+1];
	  }
      
      for (iDim = 0; iDim < nDim; iDim++)
        Residual[iDim] -= node[iPoint]->GetPressure()*Normal[iDim];
	  
      /*--- Update residual value ---*/
      
      LinSysRes.AddBlock(iPoint, Residual);
      
      
      /*--- Jacobian contribution for implicit integration ---*/
      
      if (implicit) {
		  for (iDim = 0; iDim < nDim; iDim++)
		       for (jDim = 0; jDim < nDim; jDim++)
		          Jacobian_i[iDim][jDim] = 0.0;
		          
		  if (Face_Flux > 0.0) {
			  proj_vel = 0.0;
			  for (iDim = 0; iDim < nDim; iDim++) {
				  val_normal[iDim] = -Normal[iDim];
				  proj_vel += V_domain[iDim+1]*val_normal[iDim];
			  }
			  
			  if (nDim == 2) {
				  Jacobian_i[0][0] = (V_domain[1]*val_normal[0] + proj_vel);
				  Jacobian_i[0][1] = V_domain[1]*val_normal[1];
				  
				  Jacobian_i[1][0] = V_domain[2]*val_normal[0];
				  Jacobian_i[1][1] = (V_domain[2]*val_normal[1] + proj_vel);
		       }
		       else {
				   Jacobian_i[0][0] = (proj_vel+V_domain[1]*val_normal[0]);
				   Jacobian_i[0][1] = (V_domain[1]*val_normal[1]);
				   Jacobian_i[0][2] = (V_domain[1]*val_normal[2]);
				   
				   Jacobian_i[1][0] = (V_domain[2]*val_normal[0]);
				   Jacobian_i[1][1] = (proj_vel+V_domain[2]*val_normal[1]);
				   Jacobian_i[1][2] = (V_domain[2]*val_normal[2]);
				   
				   Jacobian_i[2][0] = (V_domain[3]*val_normal[0]);
				   Jacobian_i[2][1] = (V_domain[3]*val_normal[1]);
				   Jacobian_i[2][2] = (proj_vel+V_domain[3]*val_normal[2]);
			   }
	      }
		  Jacobian.AddBlock(iPoint, iPoint, Jacobian_i);
      }
      
      if (viscous) {

        /*--- Set transport properties at the outlet. ---*/

        V_domain[nDim+2] = node[iPoint]->GetLaminarViscosity();
        V_domain[nDim+3] = node[iPoint]->GetEddyViscosity();

        /*--- Set the normal vector and the coordinates ---*/
        
        visc_numerics->SetNormal(Normal);
        visc_numerics->SetCoord(geometry->node[iPoint]->GetCoord(),
                                geometry->node[Point_Normal]->GetCoord());
        
        /*--- Primitive variables, and gradient ---*/
        
        visc_numerics->SetPrimitive(V_domain, V_domain);
        visc_numerics->SetPrimVarGradient(node[iPoint]->GetGradient_Primitive(),
                                          node[iPoint]->GetGradient_Primitive());
        
        /*--- Turbulent kinetic energy ---*/
        
        if (config->GetKind_Turb_Model() == SST)
          visc_numerics->SetTurbKineticEnergy(solver_container[TURB_SOL]->node[iPoint]->GetSolution(0),
                                              solver_container[TURB_SOL]->node[iPoint]->GetSolution(0));
        
        /*--- Compute and update residual ---*/
        
        visc_numerics->ComputeResidual(Residual, Jacobian_i, Jacobian_j, config);
        
        LinSysRes.AddBlock(iPoint, Residual);
        
        /*--- Jacobian contribution for implicit integration ---*/
        if (implicit)
          Jacobian.AddBlock(iPoint, iPoint, Jacobian_i);
        
      }
 
      
    }
  }
  /*--- Free locally allocated memory ---*/
  delete [] Normal;
  
}

void CPBIncEulerSolver::BC_Sym_Plane(CGeometry *geometry, CSolver **solver_container, CNumerics *conv_numerics, CNumerics *visc_numerics,
                                CConfig *config, unsigned short val_marker) {
  
  unsigned short iDim, iVar, jVar;
  unsigned long iPoint, iVertex;

  su2double Density = 0.0, Pressure = 0.0, *Normal = NULL, Area, *NormalArea, turb_ke;
  
  bool implicit = (config->GetKind_TimeIntScheme_Flow() == EULER_IMPLICIT);
  /*bool tkeNeeded = (((config->GetKind_Solver() == RANS ) ||
                     (config->GetKind_Solver() == DISC_ADJ_RANS)) &&
                    (config->GetKind_Turb_Model() == SST));*/
  
  Normal     = new su2double[nDim];
  NormalArea = new su2double[nDim];

  /*--- Loop over all the vertices on this boundary marker ---*/
  
  for (iVertex = 0; iVertex < geometry->nVertex[val_marker]; iVertex++) {
    iPoint = geometry->vertex[val_marker][iVertex]->GetNode();
    
    /*--- Check if the node belongs to the domain (i.e, not a halo node) ---*/
    
    if (geometry->node[iPoint]->GetDomain()) {
      
      /*--- Normal vector for this vertex (negative for outward convention) ---*/
      
      geometry->vertex[val_marker][iVertex]->GetNormal(Normal);
      
      Area = 0.0;
      for (iDim = 0; iDim < nDim; iDim++) Area += Normal[iDim]*Normal[iDim];
      Area = sqrt (Area);
      
      for (iDim = 0; iDim < nDim; iDim++) {
        NormalArea[iDim] = -Normal[iDim];
      }

      /*--- Compute the residual ---*/

      Pressure = node[iPoint]->GetPressure();
      Density  = node[iPoint]->GetDensity();

      for (iDim = 0; iDim < nDim; iDim++)
        Residual[iDim] = -Pressure*Normal[iDim];

      /*--- Add the Reynolds stress tensor contribution ---*/

      /*if (tkeNeeded) {
        turb_ke = solver_container[TURB_SOL]->node[iPoint]->GetSolution(0);
        for (iDim = 0; iDim < nDim; iDim++)
          Residual[iDim+1] += (2.0/3.0)*Density*turb_ke*NormalArea[iDim];
      }*/

      /*--- Add value to the residual ---*/

      LinSysRes.AddBlock(iPoint, Residual);
      
      /*--- Form Jacobians for implicit computations ---*/
      
      if (implicit) {
        
        /*--- Initialize Jacobian ---*/
        
        for (iVar = 0; iVar < nVar; iVar++) {
          for (jVar = 0; jVar < nVar; jVar++)
            Jacobian_i[iVar][jVar] = 0.0;
        }
        
        Jacobian.AddBlock(iPoint, iPoint, Jacobian_i);

      }
    }
  }
  
  delete [] Normal;
  delete [] NormalArea;
  
}

void CPBIncEulerSolver::BC_Custom(CGeometry *geometry, CSolver **solver_container, CNumerics *numerics, CConfig *config, unsigned short val_marker) { }

/*------------------------------------------------------------------------------------------*/
/*-------------------------- Start PB Navier Stokes solver ---------------------------------*/
/*------------------------------------------------------------------------------------------*/
CPBIncNSSolver::CPBIncNSSolver(void) : CPBIncEulerSolver() {
  
  /*--- Basic array initialization ---*/
  
  CD_Visc = NULL; CL_Visc = NULL; CSF_Visc = NULL; CEff_Visc = NULL;
  CMx_Visc = NULL;   CMy_Visc = NULL;   CMz_Visc = NULL;
  CFx_Visc = NULL;   CFy_Visc = NULL;   CFz_Visc = NULL;
  CoPx_Visc = NULL;   CoPy_Visc = NULL;   CoPz_Visc = NULL;

  ForceViscous = NULL; MomentViscous = NULL; CSkinFriction = NULL;
  
  /*--- Surface based array initialization ---*/
  
  Surface_CL_Visc = NULL; Surface_CD_Visc = NULL; Surface_CSF_Visc = NULL; Surface_CEff_Visc = NULL;
  Surface_CFx_Visc = NULL;   Surface_CFy_Visc = NULL;   Surface_CFz_Visc = NULL;
  Surface_CMx_Visc = NULL;   Surface_CMy_Visc = NULL;   Surface_CMz_Visc = NULL;

  /*--- Rotorcraft simulation array initialization ---*/
  
  CMerit_Visc = NULL; CT_Visc = NULL; CQ_Visc = NULL;
  
}

CPBIncNSSolver::CPBIncNSSolver(CGeometry *geometry, CConfig *config, unsigned short iMesh) : CPBIncEulerSolver() {
  
  unsigned long iPoint, iVertex;
  unsigned short iVar, iDim, iMarker, nLineLets;
  ifstream restart_file;
  unsigned short nZone = geometry->GetnZone();
  bool restart   = (config->GetRestart() || config->GetRestart_Flow());
  int Unst_RestartIter;
  unsigned short iZone = config->GetiZone();
  bool dual_time = ((config->GetUnsteady_Simulation() == DT_STEPPING_1ST) ||
                    (config->GetUnsteady_Simulation() == DT_STEPPING_2ND));
  bool time_stepping = config->GetUnsteady_Simulation() == TIME_STEPPING;
  bool adjoint = (config->GetContinuous_Adjoint()) || (config->GetDiscrete_Adjoint());
  string filename_ = config->GetSolution_FlowFileName();
  bool fsi     = config->GetFSI_Simulation();

  unsigned short direct_diff = config->GetDirectDiff();

  /*--- Check for a restart file to evaluate if there is a change in the angle of attack
   before computing all the non-dimesional quantities. ---*/

  if (!(!restart || (iMesh != MESH_0) || nZone > 1)) {

    /*--- Multizone problems require the number of the zone to be appended. ---*/

    if (nZone > 1) filename_ = config->GetMultizone_FileName(filename_, iZone);

    /*--- Modify file name for a dual-time unsteady restart ---*/

    if (dual_time) {
      if (adjoint) Unst_RestartIter = SU2_TYPE::Int(config->GetUnst_AdjointIter())-1;
      else if (config->GetUnsteady_Simulation() == DT_STEPPING_1ST)
        Unst_RestartIter = SU2_TYPE::Int(config->GetUnst_RestartIter())-1;
      else Unst_RestartIter = SU2_TYPE::Int(config->GetUnst_RestartIter())-2;
      filename_ = config->GetUnsteady_FileName(filename_, Unst_RestartIter);
    }

    /*--- Modify file name for a time stepping unsteady restart ---*/

    if (time_stepping) {
      if (adjoint) Unst_RestartIter = SU2_TYPE::Int(config->GetUnst_AdjointIter())-1;
      else Unst_RestartIter = SU2_TYPE::Int(config->GetUnst_RestartIter())-1;
      filename_ = config->GetUnsteady_FileName(filename_, Unst_RestartIter);
    }

    /*--- Read and store the restart metadata. ---*/

    Read_SU2_Restart_Metadata(geometry, config, false, filename_);
    
  }

  /*--- Array initialization ---*/
  
  CD_Visc = NULL; CL_Visc = NULL; CSF_Visc = NULL; CEff_Visc = NULL;
  CMx_Visc = NULL;   CMy_Visc = NULL;   CMz_Visc = NULL;
  CFx_Visc = NULL;   CFy_Visc = NULL;   CFz_Visc = NULL;
  CoPx_Visc = NULL;   CoPy_Visc = NULL;   CoPz_Visc = NULL;

  Surface_CL_Visc = NULL; Surface_CD_Visc = NULL; Surface_CSF_Visc = NULL; Surface_CEff_Visc = NULL;
  Surface_CFx_Visc = NULL;   Surface_CFy_Visc = NULL;   Surface_CFz_Visc = NULL;
  Surface_CMx_Visc = NULL;   Surface_CMy_Visc = NULL;   Surface_CMz_Visc = NULL;

  CMerit_Visc = NULL;      CT_Visc = NULL;      CQ_Visc = NULL;
  MaxHF_Visc = NULL; ForceViscous = NULL; MomentViscous = NULL;
  CSkinFriction = NULL;    Cauchy_Serie = NULL; HF_Visc = NULL;
  
  /*--- Set the gamma value ---*/
  
  Gamma = config->GetGamma();
  Gamma_Minus_One = Gamma - 1.0;
  
  /*--- Define geometry constants in the solver structure
   * Incompressible flow, primitive variables (P, vx, vy, vz, rho, lamMu, EddyMu) --- */

  nDim = geometry->GetnDim();
  
  nVar = nDim; nPrimVar = nDim+4; nPrimVarGrad = nDim+2;
  
  /*--- Initialize nVarGrad for deallocation ---*/
  
  nVarGrad = nPrimVarGrad;
  
  nMarker      = config->GetnMarker_All();
  nPoint       = geometry->GetnPoint();
  nPointDomain = geometry->GetnPointDomain();
 
  /*--- Store the number of vertices on each marker for deallocation later ---*/

  nVertex = new unsigned long[nMarker];
  for (iMarker = 0; iMarker < nMarker; iMarker++)
    nVertex[iMarker] = geometry->nVertex[iMarker];
 
  /*--- Fluid model intialization. ---*/

  FluidModel = NULL;

  /*--- Perform the non-dimensionalization for the flow equations using the
   specified reference values. ---*/
  
  SetNondimensionalization(geometry, config, iMesh);
  
  /*--- Allocate the node variables ---*/
  node = new CVariable*[nPoint];
  
  /*--- Define some auxiliar vector related with the residual ---*/
  
  Residual      = new su2double[nVar]; for (iVar = 0; iVar < nVar; iVar++) Residual[iVar]      = 0.0;
  Residual_RMS  = new su2double[nVar]; for (iVar = 0; iVar < nVar; iVar++) Residual_RMS[iVar]  = 0.0;
  Residual_Max  = new su2double[nVar]; for (iVar = 0; iVar < nVar; iVar++) Residual_Max[iVar]  = 0.0;
  Res_Conv      = new su2double[nVar]; for (iVar = 0; iVar < nVar; iVar++) Res_Conv[iVar]      = 0.0;
  Res_Visc      = new su2double[nVar]; for (iVar = 0; iVar < nVar; iVar++) Res_Visc[iVar]      = 0.0;
  Res_Sour      = new su2double[nVar]; for (iVar = 0; iVar < nVar; iVar++) Res_Sour[iVar]      = 0.0;
  
  /*--- Define some structures for locating max residuals ---*/
  
  Point_Max     = new unsigned long[nVar];  for (iVar = 0; iVar < nVar; iVar++) Point_Max[iVar]     = 0;
  Point_Max_Coord = new su2double*[nVar];
  for (iVar = 0; iVar < nVar; iVar++) {
    Point_Max_Coord[iVar] = new su2double[nDim];
    for (iDim = 0; iDim < nDim; iDim++) Point_Max_Coord[iVar][iDim] = 0.0;
  }
  
  /*--- Define some auxiliary vectors related to the solution ---*/
  
  Solution   = new su2double[nVar]; for (iVar = 0; iVar < nVar; iVar++) Solution[iVar]   = 0.0;
  Solution_i = new su2double[nVar]; for (iVar = 0; iVar < nVar; iVar++) Solution_i[iVar] = 0.0;
  Solution_j = new su2double[nVar]; for (iVar = 0; iVar < nVar; iVar++) Solution_j[iVar] = 0.0;
  
  /*--- Define some auxiliary vectors related to the geometry ---*/
  
  Vector   = new su2double[nDim]; for (iDim = 0; iDim < nDim; iDim++) Vector[iDim]   = 0.0;
  Vector_i = new su2double[nDim]; for (iDim = 0; iDim < nDim; iDim++) Vector_i[iDim] = 0.0;
  Vector_j = new su2double[nDim]; for (iDim = 0; iDim < nDim; iDim++) Vector_j[iDim] = 0.0;
  
  /*--- Define some auxiliary vectors related to the primitive solution ---*/
  
  Primitive   = new su2double[nPrimVar]; for (iVar = 0; iVar < nPrimVar; iVar++) Primitive[iVar]   = 0.0;
  Primitive_i = new su2double[nPrimVar]; for (iVar = 0; iVar < nPrimVar; iVar++) Primitive_i[iVar] = 0.0;
  Primitive_j = new su2double[nPrimVar]; for (iVar = 0; iVar < nPrimVar; iVar++) Primitive_j[iVar] = 0.0;
  
  /*--- Define some auxiliar vector related with the undivided lapalacian computation ---*/
  
  if (config->GetKind_ConvNumScheme_Flow() == SPACE_CENTERED) {
    iPoint_UndLapl = new su2double [nPoint];
    jPoint_UndLapl = new su2double [nPoint];
  }


  /*--- Initialize the solution and right hand side vectors for storing
   the residuals and updating the solution (always needed even for
   explicit schemes). ---*/
  
  LinSysSol.Initialize(nPoint, nPointDomain, nVar, 0.0);
  LinSysRes.Initialize(nPoint, nPointDomain, nVar, 0.0);
  
  /*--- Jacobians and vector structures for implicit computations ---*/
  
  if (config->GetKind_TimeIntScheme_Flow() == EULER_IMPLICIT) {
    
    Jacobian_i = new su2double* [nVar];
    Jacobian_j = new su2double* [nVar];
    for (iVar = 0; iVar < nVar; iVar++) {
      Jacobian_i[iVar] = new su2double [nVar];
      Jacobian_j[iVar] = new su2double [nVar];
    }
    
    if (rank == MASTER_NODE) cout << "Initialize Jacobian structure (Navier-Stokes). MG level: " << iMesh <<"." << endl;
    Jacobian.Initialize(nPoint, nPointDomain, nVar, nVar, true, geometry, config);
    
    if ((config->GetKind_Linear_Solver_Prec() == LINELET) ||
        (config->GetKind_Linear_Solver() == SMOOTHER_LINELET)) {
      nLineLets = Jacobian.BuildLineletPreconditioner(geometry, config);
      if (rank == MASTER_NODE) cout << "Compute linelet structure. " << nLineLets << " elements in each line (average)." << endl;
    }
    
  }
  
  else {
    if (rank == MASTER_NODE)
      cout << "Explicit scheme. No Jacobian structure (Navier-Stokes). MG level: " << iMesh <<"." << endl;
  }
  
  /*--- Define some auxiliary vectors for computing flow variable
   gradients by least squares, S matrix := inv(R)*traspose(inv(R)),
   c vector := transpose(WA)*(Wb) ---*/
  
  if (config->GetKind_Gradient_Method() == WEIGHTED_LEAST_SQUARES) {
    
    Smatrix = new su2double* [nDim];
    for (iDim = 0; iDim < nDim; iDim++)
      Smatrix[iDim] = new su2double [nDim];
    
    Cvector = new su2double* [nPrimVarGrad];
    for (iVar = 0; iVar < nPrimVarGrad; iVar++)
      Cvector[iVar] = new su2double [nDim];
  }
  
  /*--- Store the value of the characteristic primitive variables at the boundaries ---*/
  
  CharacPrimVar = new su2double** [nMarker];
  for (iMarker = 0; iMarker < nMarker; iMarker++) {
    CharacPrimVar[iMarker] = new su2double* [geometry->nVertex[iMarker]];
    for (iVertex = 0; iVertex < geometry->nVertex[iMarker]; iVertex++) {
      CharacPrimVar[iMarker][iVertex] = new su2double [nPrimVar];
      for (iVar = 0; iVar < nPrimVar; iVar++) {
        CharacPrimVar[iMarker][iVertex][iVar] = 0.0;
      }
    }
  }

  
  /*--- Inviscid force definition and coefficient in all the markers ---*/
  
  CPressure = new su2double* [nMarker];
  CPressureTarget = new su2double* [nMarker];
  for (iMarker = 0; iMarker < nMarker; iMarker++) {
    CPressure[iMarker] = new su2double [geometry->nVertex[iMarker]];
    CPressureTarget[iMarker] = new su2double [geometry->nVertex[iMarker]];
    for (iVertex = 0; iVertex < geometry->nVertex[iMarker]; iVertex++) {
      CPressure[iMarker][iVertex] = 0.0;
      CPressureTarget[iMarker][iVertex] = 0.0;
    }
  }
  
  /*--- Heat flux in all the markers ---*/
  
  HeatFlux = new su2double* [nMarker];
  HeatFluxTarget = new su2double* [nMarker];
  for (iMarker = 0; iMarker < nMarker; iMarker++) {
    HeatFlux[iMarker] = new su2double [geometry->nVertex[iMarker]];
    HeatFluxTarget[iMarker] = new su2double [geometry->nVertex[iMarker]];
    for (iVertex = 0; iVertex < geometry->nVertex[iMarker]; iVertex++) {
      HeatFlux[iMarker][iVertex] = 0.0;
      HeatFluxTarget[iMarker][iVertex] = 0.0;
    }
  }
  
  /*--- Y plus in all the markers ---*/
  
  YPlus = new su2double* [nMarker];
  for (iMarker = 0; iMarker < nMarker; iMarker++) {
    YPlus[iMarker] = new su2double [geometry->nVertex[iMarker]];
    for (iVertex = 0; iVertex < geometry->nVertex[iMarker]; iVertex++) {
      YPlus[iMarker][iVertex] = 0.0;
    }
  }
  
  /*--- Skin friction in all the markers ---*/
  
  CSkinFriction = new su2double** [nMarker];
  for (iMarker = 0; iMarker < nMarker; iMarker++) {
    CSkinFriction[iMarker] = new su2double*[nDim];
    for (iDim = 0; iDim < nDim; iDim++) {
      CSkinFriction[iMarker][iDim] = new su2double[geometry->nVertex[iMarker]];
      for (iVertex = 0; iVertex < geometry->nVertex[iMarker]; iVertex++) {
        CSkinFriction[iMarker][iDim][iVertex] = 0.0;
      }
    }
  }
  
  /*--- Store the value of the Total Pressure at the inlet BC ---*/
  
  Inlet_Ttotal = new su2double* [nMarker];
  for (iMarker = 0; iMarker < nMarker; iMarker++) {
    Inlet_Ttotal[iMarker] = new su2double [geometry->nVertex[iMarker]];
    for (iVertex = 0; iVertex < geometry->nVertex[iMarker]; iVertex++) {
      Inlet_Ttotal[iMarker][iVertex] = 0;
    }
  }
  
  /*--- Store the value of the Total Temperature at the inlet BC ---*/
  
  Inlet_Ptotal = new su2double* [nMarker];
  for (iMarker = 0; iMarker < nMarker; iMarker++) {
    Inlet_Ptotal[iMarker] = new su2double [geometry->nVertex[iMarker]];
    for (iVertex = 0; iVertex < geometry->nVertex[iMarker]; iVertex++) {
      Inlet_Ptotal[iMarker][iVertex] = 0;
    }
  }
  
  /*--- Store the value of the Flow direction at the inlet BC ---*/
  
  Inlet_FlowDir = new su2double** [nMarker];
  for (iMarker = 0; iMarker < nMarker; iMarker++) {
    Inlet_FlowDir[iMarker] = new su2double* [geometry->nVertex[iMarker]];
    for (iVertex = 0; iVertex < geometry->nVertex[iMarker]; iVertex++) {
      Inlet_FlowDir[iMarker][iVertex] = new su2double [nDim];
      for (iDim = 0; iDim < nDim; iDim++) {
        Inlet_FlowDir[iMarker][iVertex][iDim] = 0;
      }
    }
  }
  
  /*--- Non dimensional coefficients ---*/
  
  ForceInviscid  = new su2double[3];
  MomentInviscid = new su2double[3];
  CD_Inv      = new su2double[nMarker];
  CL_Inv      = new su2double[nMarker];
  CSF_Inv = new su2double[nMarker];
  CMx_Inv        = new su2double[nMarker];
  CMy_Inv        = new su2double[nMarker];
  CMz_Inv        = new su2double[nMarker];
  CEff_Inv       = new su2double[nMarker];
  CFx_Inv        = new su2double[nMarker];
  CFy_Inv        = new su2double[nMarker];
  CFz_Inv        = new su2double[nMarker];

  ForceMomentum  = new su2double[3];
  MomentMomentum = new su2double[3];
  CD_Mnt      = new su2double[nMarker];
  CL_Mnt      = new su2double[nMarker];
  CSF_Mnt = new su2double[nMarker];
  CMx_Mnt        = new su2double[nMarker];
  CMy_Mnt        = new su2double[nMarker];
  CMz_Mnt        = new su2double[nMarker];
  CEff_Mnt       = new su2double[nMarker];
  CFx_Mnt        = new su2double[nMarker];
  CFy_Mnt        = new su2double[nMarker];
  CFz_Mnt        = new su2double[nMarker];

  ForceViscous     = new su2double[3];
  MomentViscous    = new su2double[3];
  CD_Visc       = new su2double[nMarker];
  CL_Visc       = new su2double[nMarker];
  CSF_Visc  = new su2double[nMarker];
  CMx_Visc         = new su2double[nMarker];
  CMy_Visc         = new su2double[nMarker];
  CMz_Visc         = new su2double[nMarker];
  CEff_Visc        = new su2double[nMarker];
  CFx_Visc         = new su2double[nMarker];
  CFy_Visc         = new su2double[nMarker];
  CFz_Visc         = new su2double[nMarker];
  CoPx_Visc         = new su2double[nMarker];
  CoPy_Visc         = new su2double[nMarker];
  CoPz_Visc         = new su2double[nMarker];

  Surface_CL_Inv      = new su2double[config->GetnMarker_Monitoring()];
  Surface_CD_Inv      = new su2double[config->GetnMarker_Monitoring()];
  Surface_CSF_Inv = new su2double[config->GetnMarker_Monitoring()];
  Surface_CEff_Inv       = new su2double[config->GetnMarker_Monitoring()];
  Surface_CFx_Inv        = new su2double[config->GetnMarker_Monitoring()];
  Surface_CFy_Inv        = new su2double[config->GetnMarker_Monitoring()];
  Surface_CFz_Inv        = new su2double[config->GetnMarker_Monitoring()];
  Surface_CMx_Inv        = new su2double[config->GetnMarker_Monitoring()];
  Surface_CMy_Inv        = new su2double[config->GetnMarker_Monitoring()];
  Surface_CMz_Inv        = new su2double[config->GetnMarker_Monitoring()];

  Surface_CL_Mnt      = new su2double[config->GetnMarker_Monitoring()];
  Surface_CD_Mnt      = new su2double[config->GetnMarker_Monitoring()];
  Surface_CSF_Mnt = new su2double[config->GetnMarker_Monitoring()];
  Surface_CEff_Mnt       = new su2double[config->GetnMarker_Monitoring()];
  Surface_CFx_Mnt        = new su2double[config->GetnMarker_Monitoring()];
  Surface_CFy_Mnt        = new su2double[config->GetnMarker_Monitoring()];
  Surface_CFz_Mnt        = new su2double[config->GetnMarker_Monitoring()];
  Surface_CMx_Mnt        = new su2double[config->GetnMarker_Monitoring()];
  Surface_CMy_Mnt        = new su2double[config->GetnMarker_Monitoring()];
  Surface_CMz_Mnt        = new su2double[config->GetnMarker_Monitoring()];

  Surface_CL          = new su2double[config->GetnMarker_Monitoring()];
  Surface_CD          = new su2double[config->GetnMarker_Monitoring()];
  Surface_CSF     = new su2double[config->GetnMarker_Monitoring()];
  Surface_CEff           = new su2double[config->GetnMarker_Monitoring()];
  Surface_CFx            = new su2double[config->GetnMarker_Monitoring()];
  Surface_CFy            = new su2double[config->GetnMarker_Monitoring()];
  Surface_CFz            = new su2double[config->GetnMarker_Monitoring()];
  Surface_CMx            = new su2double[config->GetnMarker_Monitoring()];
  Surface_CMy            = new su2double[config->GetnMarker_Monitoring()];
  Surface_CMz            = new su2double[config->GetnMarker_Monitoring()];

  Surface_CL_Visc      = new su2double[config->GetnMarker_Monitoring()];
  Surface_CD_Visc      = new su2double[config->GetnMarker_Monitoring()];
  Surface_CSF_Visc = new su2double[config->GetnMarker_Monitoring()];
  Surface_CEff_Visc       = new su2double[config->GetnMarker_Monitoring()];
  Surface_CFx_Visc        = new su2double[config->GetnMarker_Monitoring()];
  Surface_CFy_Visc        = new su2double[config->GetnMarker_Monitoring()];
  Surface_CFz_Visc        = new su2double[config->GetnMarker_Monitoring()];
  Surface_CMx_Visc        = new su2double[config->GetnMarker_Monitoring()];
  Surface_CMy_Visc        = new su2double[config->GetnMarker_Monitoring()];
  Surface_CMz_Visc        = new su2double[config->GetnMarker_Monitoring()];
  
  /*--- Rotorcraft coefficients ---*/

  CT_Inv           = new su2double[nMarker];
  CQ_Inv           = new su2double[nMarker];
  CMerit_Inv       = new su2double[nMarker];

  CT_Mnt           = new su2double[nMarker];
  CQ_Mnt           = new su2double[nMarker];
  CMerit_Mnt       = new su2double[nMarker];

  CMerit_Visc      = new su2double[nMarker];
  CT_Visc          = new su2double[nMarker];
  CQ_Visc          = new su2double[nMarker];

  /*--- Init total coefficients ---*/

  Total_CD       = 0.0;  Total_CL           = 0.0;  Total_CSF            = 0.0;
  Total_CMx      = 0.0;  Total_CMy          = 0.0;  Total_CMz            = 0.0;
  Total_CEff     = 0.0;
  Total_CFx      = 0.0;  Total_CFy          = 0.0;  Total_CFz            = 0.0;
  Total_CT       = 0.0;  Total_CQ           = 0.0;  Total_CMerit         = 0.0;
  Total_MaxHeat  = 0.0;  Total_Heat         = 0.0;  Total_ComboObj       = 0.0;
  Total_CpDiff   = 0.0;  Total_HeatFluxDiff = 0.0;  Total_Custom_ObjFunc = 0.0;
  AoA_Prev       = 0.0;
  Total_CL_Prev  = 0.0;  Total_CD_Prev      = 0.0;
  Total_CMx_Prev = 0.0;  Total_CMy_Prev     = 0.0;  Total_CMz_Prev       = 0.0;

  /*--- Coefficients for fixed lift mode. ---*/
  
  AoA_Prev = 0.0;
  Total_CL_Prev = 0.0; Total_CD_Prev = 0.0;
  Total_CMx_Prev = 0.0; Total_CMy_Prev = 0.0; Total_CMz_Prev = 0.0;

  /*--- Read farfield conditions from config ---*/
  
  Density_Inf     = config->GetDensity_FreeStreamND();
  Pressure_Inf    = config->GetPressure_FreeStreamND();
  Velocity_Inf    = config->GetVelocity_FreeStreamND();
  Viscosity_Inf   = config->GetViscosity_FreeStreamND();
  Tke_Inf         = config->GetTke_FreeStreamND();
  
  /*--- Initialize the secondary values for direct derivative approxiations ---*/
  
  switch(direct_diff){
    case NO_DERIVATIVE:
      break;
    case D_DENSITY:
      SU2_TYPE::SetDerivative(Density_Inf, 1.0);
      break;
    case D_PRESSURE:
      SU2_TYPE::SetDerivative(Pressure_Inf, 1.0);
      break;
    case D_TEMPERATURE:
      SU2_TYPE::SetDerivative(Temperature_Inf, 1.0);
      break;
    case D_VISCOSITY:
      SU2_TYPE::SetDerivative(Viscosity_Inf, 1.0);
      break;
    case D_MACH: case D_AOA:
    case D_SIDESLIP: case D_REYNOLDS:
    case D_TURB2LAM: case D_DESIGN:
      /*--- Already done in postprocessing of config ---*/
      break;
    default:
      break;
  }
  
  cout<<"PRef_Value: "<<config->GetPRef_Value()<<endl;
  su2double *prefcoord, *Coord_i, dist_iref;
  prefcoord = config->GetPRef_Coord();
  cout<<"PRef Coord[0]: "<<prefcoord[0]<<"PRef Coord[1]: "<<prefcoord[1]<<"PRef Coord[2]: "<<prefcoord[2]<<endl;
  PRef_Point = nPoint+5;
  for (iPoint = 0; iPoint < nPoint; iPoint++) {
	  Coord_i = geometry->node[iPoint]->GetCoord();
	  dist_iref = 0.0;
	  for (iDim = 0; iDim < nDim; iDim++)
	       dist_iref += (prefcoord[iDim] - Coord_i[iDim])*(prefcoord[iDim] - Coord_i[iDim]);
	  dist_iref = sqrt(dist_iref);
	  if (dist_iref < 1.0e-2) {
		  PRef_Point = iPoint;
	  }
  }
  
  cout<<"PRef Point: "<<PRef_Point<<endl;

  /*--- Initialize the cauchy critera array for fixed CL mode ---*/

  if (config->GetFixed_CL_Mode())
    Cauchy_Serie = new su2double [config->GetCauchy_Elems()+1];

  /*--- Initialize the solution to the far-field state everywhere. ---*/

  for (iPoint = 0; iPoint < nPoint; iPoint++)
    node[iPoint] = new CPBIncNSVariable(Pressure_Inf, Velocity_Inf, nDim, nVar, config);

  /*--- Initialize the BGS residuals in FSI problems. ---*/
  if (fsi){
    Residual_BGS      = new su2double[nVar];         for (iVar = 0; iVar < nVar; iVar++) Residual_RMS[iVar]  = 0.0;
    Residual_Max_BGS  = new su2double[nVar];         for (iVar = 0; iVar < nVar; iVar++) Residual_Max_BGS[iVar]  = 0.0;

    /*--- Define some structures for locating max residuals ---*/

    Point_Max_BGS       = new unsigned long[nVar];  for (iVar = 0; iVar < nVar; iVar++) Point_Max_BGS[iVar]  = 0;
    Point_Max_Coord_BGS = new su2double*[nVar];
    for (iVar = 0; iVar < nVar; iVar++) {
      Point_Max_Coord_BGS[iVar] = new su2double[nDim];
      for (iDim = 0; iDim < nDim; iDim++) Point_Max_Coord_BGS[iVar][iDim] = 0.0;
    }
  }
  /*--- Define solver parameters needed for execution of destructor ---*/

  if (config->GetKind_ConvNumScheme_Flow() == SPACE_CENTERED) space_centered = true;
  else space_centered = false;

  if (config->GetKind_TimeIntScheme_Flow() == EULER_IMPLICIT) euler_implicit = true;
  else euler_implicit = false;

  if (config->GetKind_Gradient_Method() == WEIGHTED_LEAST_SQUARES) least_squares = true;
  else least_squares = false;

  /*--- Perform the MPI communication of the solution ---*/

  Set_MPI_Solution(geometry, config);

}


CPBIncNSSolver::~CPBIncNSSolver(void) {

  unsigned short iMarker, iDim;

  unsigned long iVertex;

  if (CD_Visc != NULL)       delete [] CD_Visc;
  if (CL_Visc != NULL)       delete [] CL_Visc;
  if (CSF_Visc != NULL)  delete [] CSF_Visc;
  if (CMx_Visc != NULL)         delete [] CMx_Visc;
  if (CMy_Visc != NULL)         delete [] CMy_Visc;
  if (CMz_Visc != NULL)         delete [] CMz_Visc;
  if (CoPx_Visc != NULL)        delete [] CoPx_Visc;
  if (CoPy_Visc != NULL)        delete [] CoPy_Visc;
  if (CoPz_Visc != NULL)        delete [] CoPz_Visc;
  if (CFx_Visc != NULL)         delete [] CFx_Visc;
  if (CFy_Visc != NULL)         delete [] CFy_Visc;
  if (CFz_Visc != NULL)         delete [] CFz_Visc;
  if (CEff_Visc != NULL)        delete [] CEff_Visc;
  if (CMerit_Visc != NULL)      delete [] CMerit_Visc;
  if (CT_Visc != NULL)          delete [] CT_Visc;
  if (CQ_Visc != NULL)          delete [] CQ_Visc;
  if (HF_Visc != NULL)        delete [] HF_Visc;
  if (MaxHF_Visc != NULL) delete [] MaxHF_Visc;
  if (ForceViscous != NULL)     delete [] ForceViscous;
  if (MomentViscous != NULL)    delete [] MomentViscous;

  if (Surface_CL_Visc != NULL)      delete [] Surface_CL_Visc;
  if (Surface_CD_Visc != NULL)      delete [] Surface_CD_Visc;
  if (Surface_CSF_Visc != NULL) delete [] Surface_CSF_Visc;
  if (Surface_CEff_Visc != NULL)       delete [] Surface_CEff_Visc;
  if (Surface_CFx_Visc != NULL)        delete [] Surface_CFx_Visc;
  if (Surface_CFy_Visc != NULL)        delete [] Surface_CFy_Visc;
  if (Surface_CFz_Visc != NULL)        delete [] Surface_CFz_Visc;
  if (Surface_CMx_Visc != NULL)        delete [] Surface_CMx_Visc;
  if (Surface_CMy_Visc != NULL)        delete [] Surface_CMy_Visc;
  if (Surface_CMz_Visc != NULL)        delete [] Surface_CMz_Visc;

  if (Cauchy_Serie != NULL) delete [] Cauchy_Serie;
  
  if (CSkinFriction != NULL) {
    for (iMarker = 0; iMarker < nMarker; iMarker++) {
      for (iDim = 0; iDim < nDim; iDim++) {
        delete [] CSkinFriction[iMarker][iDim];
      }
      delete [] CSkinFriction[iMarker];
    }
    delete [] CSkinFriction;
  }
  
}

void CPBIncNSSolver::Preprocessing(CGeometry *geometry, CSolver **solver_container, CConfig *config, unsigned short iMesh, unsigned short iRKStep, unsigned short RunTime_EqSystem, bool Output) {
	
	  unsigned long iPoint, ErrorCounter = 0;
  su2double StrainMag = 0.0, Omega = 0.0, *Vorticity;
  
  unsigned long ExtIter     = config->GetExtIter();
  bool cont_adjoint         = config->GetContinuous_Adjoint();
  bool disc_adjoint         = config->GetDiscrete_Adjoint();
  bool implicit             = (config->GetKind_TimeIntScheme_Flow() == EULER_IMPLICIT);
  bool center               = ((config->GetKind_ConvNumScheme_Flow() == SPACE_CENTERED) || (cont_adjoint && config->GetKind_ConvNumScheme_AdjFlow() == SPACE_CENTERED));
  bool center_jst           = center && config->GetKind_Centered_Flow() == JST;
  bool limiter_flow         = ((config->GetKind_SlopeLimit_Flow() != NO_LIMITER) && (ExtIter <= config->GetLimiterIter()) && !(disc_adjoint && config->GetFrozen_Limiter_Disc()));
  bool limiter_turb         = ((config->GetKind_SlopeLimit_Turb() != NO_LIMITER) && (ExtIter <= config->GetLimiterIter()) && !(disc_adjoint && config->GetFrozen_Limiter_Disc()));
  bool limiter_adjflow      = (cont_adjoint && (config->GetKind_SlopeLimit_AdjFlow() != NO_LIMITER) && (ExtIter <= config->GetLimiterIter()));
  bool fixed_cl             = config->GetFixed_CL_Mode();
  bool van_albada       = config->GetKind_SlopeLimit_Flow() == VAN_ALBADA_EDGE;

  /*--- Update the angle of attack at the far-field for fixed CL calculations (only direct problem). ---*/
  
  if ((fixed_cl) && (!disc_adjoint) && (!cont_adjoint)) { SetFarfield_AoA(geometry, solver_container, config, iMesh, Output); }
  
  /*--- Set the primitive variables ---*/
  
  ErrorCounter = SetPrimitive_Variables(solver_container, config, Output);
  
  
  
  /*--- Artificial dissipation ---*/
  
  if (center && !Output) {
    SetMax_Eigenvalue(geometry, config);
    if ((center_jst) && (iMesh == MESH_0)) {
      SetCentered_Dissipation_Sensor(geometry, config);
      SetUndivided_Laplacian(geometry, config);
    }
  }
  
  /*--- Compute gradient of the primitive variables ---*/
  
  if (config->GetKind_Gradient_Method() == GREEN_GAUSS) {
    SetPrimitive_Gradient_GG(geometry, config);
  }
  if (config->GetKind_Gradient_Method() == WEIGHTED_LEAST_SQUARES) {
    SetPrimitive_Gradient_LS(geometry, config);
  }

  /*--- Compute the limiter in case we need it in the turbulence model
   or to limit the viscous terms (check this logic with JST and 2nd order turbulence model) ---*/

  if ((iMesh == MESH_0) && (limiter_flow || limiter_turb || limiter_adjflow)
      && !Output && !van_albada) { SetPrimitive_Limiter(geometry, config); }
  
  /*--- Update the beta value based on the maximum velocity / viscosity. ---*/

  SetBeta_Parameter(geometry, solver_container, config, iMesh);

  /*--- Evaluate the vorticity and strain rate magnitude ---*/
  
  StrainMag_Max = 0.0; Omega_Max = 0.0;
  for (iPoint = 0; iPoint < nPoint; iPoint++) {
    
    solver_container[FLOW_SOL]->node[iPoint]->SetVorticity();
    solver_container[FLOW_SOL]->node[iPoint]->SetStrainMag();
    
    StrainMag = solver_container[FLOW_SOL]->node[iPoint]->GetStrainMag();
    Vorticity = solver_container[FLOW_SOL]->node[iPoint]->GetVorticity();
    Omega = sqrt(Vorticity[0]*Vorticity[0]+ Vorticity[1]*Vorticity[1]+ Vorticity[2]*Vorticity[2]);
    
    StrainMag_Max = max(StrainMag_Max, StrainMag);
    Omega_Max = max(Omega_Max, Omega);
    
  }
  
  /*--- Initialize the Jacobian matrices ---*/
  
  if (implicit && !disc_adjoint) Jacobian.SetValZero();

  /*--- Error message ---*/
  
  if (config->GetConsole_Output_Verb() == VERB_HIGH) {
    
#ifdef HAVE_MPI
    unsigned long MyErrorCounter = ErrorCounter; ErrorCounter = 0;
    su2double MyOmega_Max = Omega_Max; Omega_Max = 0.0;
    su2double MyStrainMag_Max = StrainMag_Max; StrainMag_Max = 0.0;
    
    SU2_MPI::Allreduce(&MyErrorCounter, &ErrorCounter, 1, MPI_UNSIGNED_LONG, MPI_SUM, MPI_COMM_WORLD);
    SU2_MPI::Allreduce(&MyStrainMag_Max, &StrainMag_Max, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
    SU2_MPI::Allreduce(&MyOmega_Max, &Omega_Max, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
#endif

    if (iMesh == MESH_0) {
      config->SetNonphysical_Points(ErrorCounter);
      solver_container[FLOW_SOL]->SetStrainMag_Max(StrainMag_Max);
      solver_container[FLOW_SOL]->SetOmega_Max(Omega_Max);
    }
    
  }
  
}


unsigned long CPBIncNSSolver::SetPrimitive_Variables(CSolver **solver_container, CConfig *config, bool Output) {
	
  
  unsigned long iPoint, ErrorCounter = 0;
  su2double eddy_visc = 0.0, turb_ke = 0.0, DES_LengthScale = 0.0;
  unsigned short turb_model = config->GetKind_Turb_Model();
  bool physical = true;
  
  bool tkeNeeded = false;
  
  for (iPoint = 0; iPoint < nPoint; iPoint++) {
    
    /*--- Retrieve the value of the kinetic energy (if needed) ---*/
    
    if (turb_model != NONE) {
      eddy_visc = solver_container[TURB_SOL]->node[iPoint]->GetmuT();
      if (tkeNeeded) turb_ke = solver_container[TURB_SOL]->node[iPoint]->GetSolution(0);
    }
    
    /*--- Initialize the non-physical points vector ---*/
    
    node[iPoint]->SetNon_Physical(false);
    
    /*--- Incompressible flow, primitive variables --- */

    physical = node[iPoint]->SetPrimVar(Density_Inf, eddy_visc, turb_ke, FluidModel);
    
    /*--- Record any non-physical points. ---*/

    if (!physical) { node[iPoint]->SetNon_Physical(true); ErrorCounter++; }

    /*--- Initialize the convective, source and viscous residual vector ---*/
    
    if (!Output) LinSysRes.SetBlock_Zero(iPoint);
    
  }
  
  //node[PRef_Point]->SetPressure_val(config->GetPRef_Value());

  return ErrorCounter;
	
	
}

void CPBIncNSSolver::Viscous_Residual(CGeometry *geometry, CSolver **solver_container, CNumerics *numerics,
                        CConfig *config, unsigned short iMesh, unsigned short iRKStep) {
							
  unsigned long iPoint, jPoint, iEdge;
  
  bool implicit = (config->GetKind_TimeIntScheme_Flow() == EULER_IMPLICIT);
  
  for (iEdge = 0; iEdge < geometry->GetnEdge(); iEdge++) {
    
    /*--- Points, coordinates and normal vector in edge ---*/
    
    iPoint = geometry->edge[iEdge]->GetNode(0);
    jPoint = geometry->edge[iEdge]->GetNode(1);
    numerics->SetCoord(geometry->node[iPoint]->GetCoord(),
                       geometry->node[jPoint]->GetCoord());
    numerics->SetNormal(geometry->edge[iEdge]->GetNormal());
    
    /*--- Primitive and secondary variables ---*/
    
    numerics->SetPrimitive(node[iPoint]->GetPrimitive(),
                           node[jPoint]->GetPrimitive());
    numerics->SetSecondary(node[iPoint]->GetSecondary(),
                           node[jPoint]->GetSecondary());
    
    /*--- Gradient and limiters ---*/
    
    numerics->SetPrimVarGradient(node[iPoint]->GetGradient_Primitive(),
                                 node[jPoint]->GetGradient_Primitive());
    
    /*--- Turbulent kinetic energy ---*/
    
    if (config->GetKind_Turb_Model() == SST)
      numerics->SetTurbKineticEnergy(solver_container[TURB_SOL]->node[iPoint]->GetSolution(0),
                                     solver_container[TURB_SOL]->node[jPoint]->GetSolution(0));
    
    /*--- Compute and update residual ---*/
    
    numerics->ComputeResidual(Res_Visc, Jacobian_i, Jacobian_j, config);

    LinSysRes.SubtractBlock(iPoint, Res_Visc);
    LinSysRes.AddBlock(jPoint, Res_Visc);
    
    /*--- Implicit part ---*/
    
    if (implicit) {
      Jacobian.SubtractBlock(iPoint, iPoint, Jacobian_i);
      Jacobian.SubtractBlock(iPoint, jPoint, Jacobian_j);
      Jacobian.AddBlock(jPoint, iPoint, Jacobian_i);
      Jacobian.AddBlock(jPoint, jPoint, Jacobian_j);
    }
    
  }
}



void CPBIncNSSolver::SetTime_Step(CGeometry *geometry, CSolver **solver_container, CConfig *config,
                    unsigned short iMesh, unsigned long Iteration) {
						
  su2double Mean_BetaInc2, *Normal, Area, Vol, Mean_SoundSpeed = 0.0, Mean_ProjVel = 0.0, Lambda, Local_Delta_Time, Local_Delta_Time_Visc,
  Global_Delta_Time = 1E6, Mean_LaminarVisc = 0.0, Mean_EddyVisc = 0.0, Mean_Density = 0.0, Mean_Thermal_Conductivity = 0.0, Mean_Cv = 0.0, Lambda_1, Lambda_2, K_v = 0.25, Global_Delta_UnstTimeND;
  unsigned long iEdge, iVertex, iPoint = 0, jPoint = 0;
  unsigned short iDim, iMarker;
  su2double ProjVel, ProjVel_i, ProjVel_j;
  
  bool implicit = (config->GetKind_TimeIntScheme_Flow() == EULER_IMPLICIT);
  bool grid_movement = config->GetGrid_Movement();
  bool dual_time = ((config->GetUnsteady_Simulation() == DT_STEPPING_1ST) ||
                    (config->GetUnsteady_Simulation() == DT_STEPPING_2ND));
  bool energy = config->GetEnergy_Equation();

  Min_Delta_Time = 1.E6; Max_Delta_Time = 0.0;
  
  /*--- Set maximum inviscid eigenvalue to zero, and compute sound speed and viscosity ---*/
  
  for (iPoint = 0; iPoint < nPointDomain; iPoint++) {
    node[iPoint]->SetMax_Lambda_Inv(0.0);
    node[iPoint]->SetMax_Lambda_Visc(0.0);
  }
  
  /*--- Loop interior edges ---*/
  
  for (iEdge = 0; iEdge < geometry->GetnEdge(); iEdge++) {
    
    /*--- Point identification, Normal vector and area ---*/
    
    iPoint = geometry->edge[iEdge]->GetNode(0);
    jPoint = geometry->edge[iEdge]->GetNode(1);
    
    Normal = geometry->edge[iEdge]->GetNormal();
    Area = 0; for (iDim = 0; iDim < nDim; iDim++) Area += Normal[iDim]*Normal[iDim]; Area = sqrt(Area);
    
    /*--- Mean Values ---*/
    
    Mean_ProjVel    = 0.5 * (node[iPoint]->GetProjVel(Normal) + node[jPoint]->GetProjVel(Normal));
    Mean_BetaInc2   = 0.5 * (node[iPoint]->GetBetaInc2()      + node[jPoint]->GetBetaInc2());
    Mean_Density    = 0.5 * (node[iPoint]->GetDensity()       + node[jPoint]->GetDensity());
    Mean_SoundSpeed = sqrt(Mean_BetaInc2*Area*Area);
    
    /*--- Adjustment for grid movement ---*/
    
    if (grid_movement) {
      su2double *GridVel_i = geometry->node[iPoint]->GetGridVel();
      su2double *GridVel_j = geometry->node[jPoint]->GetGridVel();
      ProjVel_i = 0.0; ProjVel_j =0.0;
      for (iDim = 0; iDim < nDim; iDim++) {
        ProjVel_i += GridVel_i[iDim]*Normal[iDim];
        ProjVel_j += GridVel_j[iDim]*Normal[iDim];
      }
      Mean_ProjVel -= 0.5 * (ProjVel_i + ProjVel_j);
    }
    
    /*--- Inviscid contribution ---*/
    
    Lambda = fabs(Mean_ProjVel) + Mean_SoundSpeed;
    if (geometry->node[iPoint]->GetDomain()) node[iPoint]->AddMax_Lambda_Inv(Lambda);
    if (geometry->node[jPoint]->GetDomain()) node[jPoint]->AddMax_Lambda_Inv(Lambda);
    
    /*--- Viscous contribution ---*/
    
    Mean_LaminarVisc          = 0.5*(node[iPoint]->GetLaminarViscosity()    + node[jPoint]->GetLaminarViscosity());
    Mean_EddyVisc             = 0.5*(node[iPoint]->GetEddyViscosity()       + node[jPoint]->GetEddyViscosity());
    Mean_Density              = 0.5*(node[iPoint]->GetDensity()             + node[jPoint]->GetDensity());
    Mean_Thermal_Conductivity = 0.5*(node[iPoint]->GetThermalConductivity() + node[jPoint]->GetThermalConductivity());
    Mean_Cv                   = 0.5*(node[iPoint]->GetSpecificHeatCv()      + node[jPoint]->GetSpecificHeatCv());

    Lambda_1 = (4.0/3.0)*(Mean_LaminarVisc + Mean_EddyVisc);
    Lambda_2 = 0.0;
    if (energy) Lambda_2 = (1.0/Mean_Cv)*Mean_Thermal_Conductivity;
    Lambda = (Lambda_1 + Lambda_2)*Area*Area/Mean_Density;
    
    if (geometry->node[iPoint]->GetDomain()) node[iPoint]->AddMax_Lambda_Visc(Lambda);
    if (geometry->node[jPoint]->GetDomain()) node[jPoint]->AddMax_Lambda_Visc(Lambda);
    
  }
  
  /*--- Loop boundary edges ---*/
  
  for (iMarker = 0; iMarker < geometry->GetnMarker(); iMarker++) {
    for (iVertex = 0; iVertex < geometry->GetnVertex(iMarker); iVertex++) {
      
      /*--- Point identification, Normal vector and area ---*/
      
      iPoint = geometry->vertex[iMarker][iVertex]->GetNode();
      Normal = geometry->vertex[iMarker][iVertex]->GetNormal();
      Area = 0.0; for (iDim = 0; iDim < nDim; iDim++) Area += Normal[iDim]*Normal[iDim]; Area = sqrt(Area);
      
      /*--- Mean Values ---*/
      
      Mean_ProjVel    = node[iPoint]->GetProjVel(Normal);
      Mean_BetaInc2   = node[iPoint]->GetBetaInc2();
      Mean_Density    = node[iPoint]->GetDensity();
      Mean_SoundSpeed = sqrt(Mean_BetaInc2*Area*Area);

      /*--- Adjustment for grid movement ---*/
      
      if (grid_movement) {
        su2double *GridVel = geometry->node[iPoint]->GetGridVel();
        ProjVel = 0.0;
        for (iDim = 0; iDim < nDim; iDim++)
          ProjVel += GridVel[iDim]*Normal[iDim];
        Mean_ProjVel -= ProjVel;
      }
      
      /*--- Inviscid contribution ---*/
      
      Lambda = fabs(Mean_ProjVel) + Mean_SoundSpeed;
      if (geometry->node[iPoint]->GetDomain()) {
        node[iPoint]->AddMax_Lambda_Inv(Lambda);
      }
      
      /*--- Viscous contribution ---*/

      Mean_LaminarVisc          = node[iPoint]->GetLaminarViscosity();
      Mean_EddyVisc             = node[iPoint]->GetEddyViscosity();
      Mean_Density              = node[iPoint]->GetDensity();
      Mean_Thermal_Conductivity = node[iPoint]->GetThermalConductivity();
      Mean_Cv                   = node[iPoint]->GetSpecificHeatCv();

      Lambda_1 = (4.0/3.0)*(Mean_LaminarVisc + Mean_EddyVisc);
      Lambda_2 = 0.0;
      if (energy) Lambda_2 = (1.0/Mean_Cv)*Mean_Thermal_Conductivity;
      Lambda = (Lambda_1 + Lambda_2)*Area*Area/Mean_Density;
      
      if (geometry->node[iPoint]->GetDomain()) node[iPoint]->AddMax_Lambda_Visc(Lambda);
      
    }
  }
  
  /*--- Each element uses their own speed, steady state simulation ---*/
  
  for (iPoint = 0; iPoint < nPointDomain; iPoint++) {
    
    Vol = geometry->node[iPoint]->GetVolume();
    
    if (Vol != 0.0) {
      Local_Delta_Time = config->GetCFL(iMesh)*Vol / node[iPoint]->GetMax_Lambda_Inv();
      Local_Delta_Time_Visc = config->GetCFL(iMesh)*K_v*Vol*Vol/ node[iPoint]->GetMax_Lambda_Visc();
      Local_Delta_Time = min(Local_Delta_Time, Local_Delta_Time_Visc);
      Global_Delta_Time = min(Global_Delta_Time, Local_Delta_Time);
      Min_Delta_Time = min(Min_Delta_Time, Local_Delta_Time);
      Max_Delta_Time = max(Max_Delta_Time, Local_Delta_Time);
      if (Local_Delta_Time > config->GetMax_DeltaTime())
        Local_Delta_Time = config->GetMax_DeltaTime();
        Local_Delta_Time = config->GetCFL(iMesh)*1.0e-6;
      node[iPoint]->SetDelta_Time(Local_Delta_Time);
    }
    else {
      node[iPoint]->SetDelta_Time(0.0);
    }
    
  }
  
  /*--- Compute the max and the min dt (in parallel) ---*/
  if (config->GetConsole_Output_Verb() == VERB_HIGH) {
#ifdef HAVE_MPI
    su2double rbuf_time, sbuf_time;
    sbuf_time = Min_Delta_Time;
    SU2_MPI::Reduce(&sbuf_time, &rbuf_time, 1, MPI_DOUBLE, MPI_MIN, MASTER_NODE, MPI_COMM_WORLD);
    SU2_MPI::Bcast(&rbuf_time, 1, MPI_DOUBLE, MASTER_NODE, MPI_COMM_WORLD);
    Min_Delta_Time = rbuf_time;
    
    sbuf_time = Max_Delta_Time;
    SU2_MPI::Reduce(&sbuf_time, &rbuf_time, 1, MPI_DOUBLE, MPI_MAX, MASTER_NODE, MPI_COMM_WORLD);
    SU2_MPI::Bcast(&rbuf_time, 1, MPI_DOUBLE, MASTER_NODE, MPI_COMM_WORLD);
    Max_Delta_Time = rbuf_time;
#endif
  }
  
  /*--- For exact time solution use the minimum delta time of the whole mesh ---*/
  if (config->GetUnsteady_Simulation() == TIME_STEPPING) {
#ifdef HAVE_MPI
    su2double rbuf_time, sbuf_time;
    sbuf_time = Global_Delta_Time;
    SU2_MPI::Reduce(&sbuf_time, &rbuf_time, 1, MPI_DOUBLE, MPI_MIN, MASTER_NODE, MPI_COMM_WORLD);
    SU2_MPI::Bcast(&rbuf_time, 1, MPI_DOUBLE, MASTER_NODE, MPI_COMM_WORLD);
    Global_Delta_Time = rbuf_time;
#endif
    for (iPoint = 0; iPoint < nPointDomain; iPoint++)
      node[iPoint]->SetDelta_Time(Global_Delta_Time);
  }
  
  /*--- Recompute the unsteady time step for the dual time strategy
   if the unsteady CFL is diferent from 0 ---*/
  if ((dual_time) && (Iteration == 0) && (config->GetUnst_CFL() != 0.0) && (iMesh == MESH_0)) {
    Global_Delta_UnstTimeND = config->GetUnst_CFL()*Global_Delta_Time/config->GetCFL(iMesh);
    
#ifdef HAVE_MPI
    su2double rbuf_time, sbuf_time;
    sbuf_time = Global_Delta_UnstTimeND;
    SU2_MPI::Reduce(&sbuf_time, &rbuf_time, 1, MPI_DOUBLE, MPI_MIN, MASTER_NODE, MPI_COMM_WORLD);
    SU2_MPI::Bcast(&rbuf_time, 1, MPI_DOUBLE, MASTER_NODE, MPI_COMM_WORLD);
    Global_Delta_UnstTimeND = rbuf_time;
#endif
    config->SetDelta_UnstTimeND(Global_Delta_UnstTimeND);
  }
  
  /*--- The pseudo local time (explicit integration) cannot be greater than the physical time ---*/
  if (dual_time)
    for (iPoint = 0; iPoint < nPointDomain; iPoint++) {
      if (!implicit) {
        Local_Delta_Time = min((2.0/3.0)*config->GetDelta_UnstTimeND(), node[iPoint]->GetDelta_Time());
        node[iPoint]->SetDelta_Time(Local_Delta_Time);
      }
    }


}

void CPBIncNSSolver::BC_HeatFlux_Wall(CGeometry *geometry, CSolver **solver_container, CNumerics *conv_numerics, CNumerics *visc_numerics, CConfig *config, unsigned short val_marker){
  
  
  unsigned short iDim, iVar, jVar;// Wall_Function;
  unsigned long iVertex, iPoint, total_index, Point_Normal;
  
  su2double *GridVel, *Normal, Area, Wall_HeatFlux;
  su2double *Coord_i, *Coord_j, dist_ij, delP, Pressure_j, Pressure_i;

  bool implicit      = (config->GetKind_TimeIntScheme_Flow() == EULER_IMPLICIT);
  bool grid_movement = config->GetGrid_Movement();
  
  /*--- Identify the boundary by string name ---*/
  
  string Marker_Tag = config->GetMarker_All_TagBound(val_marker);

  /*--- Loop over all of the vertices on this boundary marker ---*/
  
  for (iVertex = 0; iVertex < geometry->nVertex[val_marker]; iVertex++) {
    iPoint = geometry->vertex[val_marker][iVertex]->GetNode();
    
    /*--- Check if the node belongs to the domain (i.e, not a halo node) ---*/
    
    if (geometry->node[iPoint]->GetDomain()) {
      
      /*--- Compute dual-grid area and boundary normal ---*/
      
      Normal = geometry->vertex[val_marker][iVertex]->GetNormal();
      
      Area = 0.0;
      for (iDim = 0; iDim < nDim; iDim++)
        Area += Normal[iDim]*Normal[iDim];
      Area = sqrt (Area);

      /*--- Initialize the convective & viscous residuals to zero ---*/
      
      for (iVar = 0; iVar < nVar; iVar++) {
        Res_Conv[iVar] = 0.0;
        Res_Visc[iVar] = 0.0;
        if (implicit) {
          for (jVar = 0; jVar < nVar; jVar++)
            Jacobian_i[iVar][jVar] = 0.0;
        }
      }

      /*--- Store the corrected velocity at the wall which will
       be zero (v = 0), unless there are moving walls (v = u_wall)---*/
      
      if (grid_movement) {
        GridVel = geometry->node[iPoint]->GetGridVel();
        for (iDim = 0; iDim < nDim; iDim++) Vector[iDim] = GridVel[iDim];
      } else {
        for (iDim = 0; iDim < nDim; iDim++) Vector[iDim] = 0.0;
      }
      
      /*--- Impose the value of the velocity as a strong boundary
       condition (Dirichlet). Fix the velocity and remove any
       contribution to the residual at this node. ---*/
      
      node[iPoint]->SetVelocity_Old(Vector);
      
      for (iDim = 0; iDim < nDim; iDim++)
        LinSysRes.SetBlock_Zero(iPoint, iDim);
      //node[iPoint]->SetVel_ResTruncError_Zero();

      /*--- Enforce the no-slip boundary condition in a strong way by
       modifying the velocity-rows of the Jacobian (1 on the diagonal). ---*/
      
      if (implicit) {
        for (iVar = 0; iVar < nDim; iVar++) {
          total_index = iPoint*nVar+iVar;
          Jacobian.DeleteValsRowi(total_index);
        }
      }
    }
  }
}

  

void CPBIncNSSolver::BC_Isothermal_Wall(CGeometry *geometry, CSolver **solver_container, CNumerics *conv_numerics, CNumerics *visc_numerics, CConfig *config, unsigned short val_marker){
}

void CPBIncNSSolver::Friction_Forces(CGeometry *geometry, CConfig *config){


unsigned long iVertex, iPoint, iPointNormal;
  unsigned short Boundary, Monitoring, iMarker, iMarker_Monitoring, iDim, jDim;
  su2double Viscosity = 0.0, div_vel, *Normal, MomentDist[3] = {0.0, 0.0, 0.0}, WallDist[3] = {0.0, 0.0, 0.0},
  *Coord, *Coord_Normal, Area, WallShearStress, TauNormal, factor, RefVel2 = 0.0,
  RefDensity = 0.0, Density = 0.0, WallDistMod, FrictionVel, UnitNormal[3] = {0.0, 0.0, 0.0}, TauElem[3] = {0.0, 0.0, 0.0}, TauTangent[3] = {0.0, 0.0, 0.0},
  Tau[3][3] = {{0.0, 0.0, 0.0},{0.0, 0.0, 0.0},{0.0, 0.0, 0.0}}, Force[3] = {0.0, 0.0, 0.0},
  Grad_Vel[3][3] = {{0.0, 0.0, 0.0},{0.0, 0.0, 0.0},{0.0, 0.0, 0.0}},
  delta[3][3] = {{1.0, 0.0, 0.0},{0.0,1.0,0.0},{0.0,0.0,1.0}},
  Grad_Temp[3] = {0.0, 0.0, 0.0}, GradTemperature, thermal_conductivity, MaxNorm = 8.0;
  su2double MomentX_Force[3] = {0.0,0.0,0.0}, MomentY_Force[3] = {0.0,0.0,0.0}, MomentZ_Force[3] = {0.0,0.0,0.0};
  su2double AxiFactor;

#ifdef HAVE_MPI
  su2double MyAllBound_CD_Visc, MyAllBound_CL_Visc, MyAllBound_CSF_Visc, MyAllBound_CMx_Visc, MyAllBound_CMy_Visc, MyAllBound_CMz_Visc, MyAllBound_CoPx_Visc, MyAllBound_CoPy_Visc, MyAllBound_CoPz_Visc, MyAllBound_CFx_Visc, MyAllBound_CFy_Visc, MyAllBound_CFz_Visc, MyAllBound_CT_Visc, MyAllBound_CQ_Visc, MyAllBound_HF_Visc, MyAllBound_MaxHF_Visc, *MySurface_CL_Visc = NULL, *MySurface_CD_Visc = NULL, *MySurface_CSF_Visc = NULL, *MySurface_CEff_Visc = NULL, *MySurface_CFx_Visc = NULL, *MySurface_CFy_Visc = NULL, *MySurface_CFz_Visc = NULL, *MySurface_CMx_Visc = NULL, *MySurface_CMy_Visc = NULL, *MySurface_CMz_Visc = NULL, *MySurface_HF_Visc = NULL, *MySurface_MaxHF_Visc = NULL;
#endif

  string Marker_Tag, Monitoring_Tag;

  su2double Alpha     = config->GetAoA()*PI_NUMBER/180.0;
  su2double Beta      = config->GetAoS()*PI_NUMBER/180.0;
  su2double RefArea   = config->GetRefArea();
  su2double RefLength = config->GetRefLength();
  su2double *Origin = NULL;

  if (config->GetnMarker_Monitoring() != 0) { Origin = config->GetRefOriginMoment(0); }

  bool axisymmetric = config->GetAxisymmetric();
  bool energy       = config->GetEnergy_Equation();

  /*--- Evaluate reference values for non-dimensionalization.
   For dimensional or non-dim based on initial values, use
   the far-field state (inf). For a custom non-dim based
   on user-provided reference values, use the ref values
   to compute the forces. ---*/

  if ((config->GetRef_Inc_NonDim() == DIMENSIONAL) || 
      (config->GetRef_Inc_NonDim() == INITIAL_VALUES)) {
    RefDensity  = Density_Inf;
    RefVel2 = 0.0;
    for (iDim = 0; iDim < nDim; iDim++)
      RefVel2  += Velocity_Inf[iDim]*Velocity_Inf[iDim];
  }
  else if (config->GetRef_Inc_NonDim() == REFERENCE_VALUES) {
    RefDensity = config->GetInc_Density_Ref();
    RefVel2    = config->GetInc_Velocity_Ref()*config->GetInc_Velocity_Ref();
  }

  /*--- Compute factor for force coefficients. ---*/

  factor = 1.0 / (0.5*RefDensity*RefArea*RefVel2);

  /*--- Variables initialization ---*/

  AllBound_CD_Visc = 0.0;    AllBound_CL_Visc = 0.0;       AllBound_CSF_Visc = 0.0;
  AllBound_CMx_Visc = 0.0;      AllBound_CMy_Visc = 0.0;         AllBound_CMz_Visc = 0.0;
  AllBound_CFx_Visc = 0.0;      AllBound_CFy_Visc = 0.0;         AllBound_CFz_Visc = 0.0;
  AllBound_CoPx_Visc = 0.0;      AllBound_CoPy_Visc = 0.0;         AllBound_CoPz_Visc = 0.0;
  AllBound_CT_Visc = 0.0;       AllBound_CQ_Visc = 0.0;          AllBound_CMerit_Visc = 0.0;
  AllBound_HF_Visc = 0.0; AllBound_MaxHF_Visc = 0.0; AllBound_CEff_Visc = 0.0;

  for (iMarker_Monitoring = 0; iMarker_Monitoring < config->GetnMarker_Monitoring(); iMarker_Monitoring++) {
    Surface_CL_Visc[iMarker_Monitoring]      = 0.0; Surface_CD_Visc[iMarker_Monitoring]      = 0.0;
    Surface_CSF_Visc[iMarker_Monitoring] = 0.0; Surface_CEff_Visc[iMarker_Monitoring]       = 0.0;
    Surface_CFx_Visc[iMarker_Monitoring]        = 0.0; Surface_CFy_Visc[iMarker_Monitoring]        = 0.0;
    Surface_CFz_Visc[iMarker_Monitoring]        = 0.0; Surface_CMx_Visc[iMarker_Monitoring]        = 0.0;
    Surface_CMy_Visc[iMarker_Monitoring]        = 0.0; Surface_CMz_Visc[iMarker_Monitoring]        = 0.0;
  }

  /*--- Loop over the Navier-Stokes markers ---*/

  for (iMarker = 0; iMarker < nMarker; iMarker++) {

    Boundary = config->GetMarker_All_KindBC(iMarker);
    Monitoring = config->GetMarker_All_Monitoring(iMarker);

    /*--- Obtain the origin for the moment computation for a particular marker ---*/

    if (Monitoring == YES) {
      for (iMarker_Monitoring = 0; iMarker_Monitoring < config->GetnMarker_Monitoring(); iMarker_Monitoring++) {
        Monitoring_Tag = config->GetMarker_Monitoring_TagBound(iMarker_Monitoring);
        Marker_Tag = config->GetMarker_All_TagBound(iMarker);
        if (Marker_Tag == Monitoring_Tag)
          Origin = config->GetRefOriginMoment(iMarker_Monitoring);
      }
    }

    if ((Boundary == HEAT_FLUX) || (Boundary == ISOTHERMAL)) {

      /*--- Forces initialization at each Marker ---*/

      CD_Visc[iMarker] = 0.0; CL_Visc[iMarker] = 0.0;       CSF_Visc[iMarker] = 0.0;
      CMx_Visc[iMarker] = 0.0;   CMy_Visc[iMarker] = 0.0;         CMz_Visc[iMarker] = 0.0;
      CFx_Visc[iMarker] = 0.0;   CFy_Visc[iMarker] = 0.0;         CFz_Visc[iMarker] = 0.0;
      CoPx_Visc[iMarker] = 0.0;  CoPy_Visc[iMarker] = 0.0;       CoPz_Visc[iMarker] = 0.0;
      CT_Visc[iMarker] = 0.0;    CQ_Visc[iMarker] = 0.0;          CMerit_Visc[iMarker] = 0.0;
      HF_Visc[iMarker] = 0.0;  MaxHF_Visc[iMarker] = 0.0; CEff_Visc[iMarker] = 0.0;

      for (iDim = 0; iDim < nDim; iDim++) ForceViscous[iDim] = 0.0;
      MomentViscous[0] = 0.0; MomentViscous[1] = 0.0; MomentViscous[2] = 0.0;
      MomentX_Force[0] = 0.0; MomentX_Force[1] = 0.0; MomentX_Force[2] = 0.0;
      MomentY_Force[0] = 0.0; MomentY_Force[1] = 0.0; MomentY_Force[2] = 0.0;
      MomentZ_Force[0] = 0.0; MomentZ_Force[1] = 0.0; MomentZ_Force[2] = 0.0;

      /*--- Loop over the vertices to compute the forces ---*/

      for (iVertex = 0; iVertex < geometry->nVertex[iMarker]; iVertex++) {

        iPoint = geometry->vertex[iMarker][iVertex]->GetNode();
        iPointNormal = geometry->vertex[iMarker][iVertex]->GetNormal_Neighbor();

        Coord = geometry->node[iPoint]->GetCoord();
        Coord_Normal = geometry->node[iPointNormal]->GetCoord();

        Normal = geometry->vertex[iMarker][iVertex]->GetNormal();

        for (iDim = 0; iDim < nDim; iDim++) {
          for (jDim = 0 ; jDim < nDim; jDim++) {
            Grad_Vel[iDim][jDim] = node[iPoint]->GetGradient_Primitive(iDim+1, jDim);
          }
        }

        Viscosity = node[iPoint]->GetLaminarViscosity();
        Density = node[iPoint]->GetDensity();

        Area = 0.0; for (iDim = 0; iDim < nDim; iDim++) Area += Normal[iDim]*Normal[iDim]; Area = sqrt(Area);
        for (iDim = 0; iDim < nDim; iDim++) {
          UnitNormal[iDim] = Normal[iDim]/Area;
        }

        /*--- Evaluate Tau ---*/

        div_vel = 0.0; for (iDim = 0; iDim < nDim; iDim++) div_vel += Grad_Vel[iDim][iDim];

        for (iDim = 0; iDim < nDim; iDim++) {
          for (jDim = 0 ; jDim < nDim; jDim++) {
            Tau[iDim][jDim] = Viscosity*(Grad_Vel[jDim][iDim] + Grad_Vel[iDim][jDim]) - TWO3*Viscosity*div_vel*delta[iDim][jDim];
          }
        }

        /*--- Project Tau in each surface element ---*/

        for (iDim = 0; iDim < nDim; iDim++) {
          TauElem[iDim] = 0.0;
          for (jDim = 0; jDim < nDim; jDim++) {
            TauElem[iDim] += Tau[iDim][jDim]*UnitNormal[jDim];
          }
        }

        /*--- Compute wall shear stress (using the stress tensor). Compute wall skin friction coefficient, and heat flux on the wall ---*/

        TauNormal = 0.0;
        for (iDim = 0; iDim < nDim; iDim++)
          TauNormal += TauElem[iDim] * UnitNormal[iDim];

        WallShearStress = 0.0;
        for (iDim = 0; iDim < nDim; iDim++) {
          TauTangent[iDim] = TauElem[iDim] - TauNormal * UnitNormal[iDim];
          CSkinFriction[iMarker][iDim][iVertex] = TauTangent[iDim] / (0.5*RefDensity*RefVel2);
          WallShearStress += TauTangent[iDim] * TauTangent[iDim];
        }
        WallShearStress = sqrt(WallShearStress);

        for (iDim = 0; iDim < nDim; iDim++) WallDist[iDim] = (Coord[iDim] - Coord_Normal[iDim]);
        WallDistMod = 0.0; for (iDim = 0; iDim < nDim; iDim++) WallDistMod += WallDist[iDim]*WallDist[iDim]; WallDistMod = sqrt(WallDistMod);

        /*--- Compute y+ and non-dimensional velocity ---*/

        FrictionVel = sqrt(fabs(WallShearStress)/Density);
        YPlus[iMarker][iVertex] = WallDistMod*FrictionVel/(Viscosity/Density);

        /*--- Note that y+, and heat are computed at the
         halo cells (for visualization purposes), but not the forces ---*/

        if ((geometry->node[iPoint]->GetDomain()) && (Monitoring == YES)) {

          /*--- Axisymmetric simulations ---*/

          if (axisymmetric) AxiFactor = 2.0*PI_NUMBER*geometry->node[iPoint]->GetCoord(1);
          else AxiFactor = 1.0;

          /*--- Force computation ---*/

          for (iDim = 0; iDim < nDim; iDim++) {
            Force[iDim] = TauElem[iDim] * Area * factor * AxiFactor;
            ForceViscous[iDim] += Force[iDim];
            MomentDist[iDim] = Coord[iDim] - Origin[iDim];
          }

          /*--- Moment with respect to the reference axis ---*/

          if (iDim == 3) {
            MomentViscous[0] += (Force[2]*MomentDist[1] - Force[1]*MomentDist[2])/RefLength;
            MomentX_Force[1] += (-Force[1]*Coord[2]);
            MomentX_Force[2] += (Force[2]*Coord[1]);

            MomentViscous[1] += (Force[0]*MomentDist[2] - Force[2]*MomentDist[0])/RefLength;
            MomentY_Force[2] += (-Force[2]*Coord[0]);
            MomentY_Force[0] += (Force[0]*Coord[2]);
          }
          MomentViscous[2] += (Force[1]*MomentDist[0] - Force[0]*MomentDist[1])/RefLength;
          MomentZ_Force[0] += (-Force[0]*Coord[1]);
          MomentZ_Force[1] += (Force[1]*Coord[0]);
          
        }

      }

      /*--- Project forces and store the non-dimensional coefficients ---*/

      if (Monitoring == YES) {
        if (nDim == 2) {
          CD_Visc[iMarker]       =  ForceViscous[0]*cos(Alpha) + ForceViscous[1]*sin(Alpha);
          CL_Visc[iMarker]       = -ForceViscous[0]*sin(Alpha) + ForceViscous[1]*cos(Alpha);
          CEff_Visc[iMarker]        = CL_Visc[iMarker] / (CD_Visc[iMarker]+EPS);
          CMz_Visc[iMarker]         = MomentViscous[2];
          CFx_Visc[iMarker]         = ForceViscous[0];
          CFy_Visc[iMarker]         = ForceViscous[1];
          CoPx_Visc[iMarker]        = MomentZ_Force[1];
          CoPy_Visc[iMarker]        = -MomentZ_Force[0];
          CT_Visc[iMarker]          = -CFx_Visc[iMarker];
          CQ_Visc[iMarker]          = -CMz_Visc[iMarker];
          CMerit_Visc[iMarker]      = CT_Visc[iMarker] / (CQ_Visc[iMarker]+EPS);
        }
        if (nDim == 3) {
          CD_Visc[iMarker]       =  ForceViscous[0]*cos(Alpha)*cos(Beta) + ForceViscous[1]*sin(Beta) + ForceViscous[2]*sin(Alpha)*cos(Beta);
          CL_Visc[iMarker]       = -ForceViscous[0]*sin(Alpha) + ForceViscous[2]*cos(Alpha);
          CSF_Visc[iMarker]  = -ForceViscous[0]*sin(Beta)*cos(Alpha) + ForceViscous[1]*cos(Beta) - ForceViscous[2]*sin(Beta)*sin(Alpha);
          CEff_Visc[iMarker]        = CL_Visc[iMarker]/(CD_Visc[iMarker] + EPS);
          CMx_Visc[iMarker]         = MomentViscous[0];
          CMy_Visc[iMarker]         = MomentViscous[1];
          CMz_Visc[iMarker]         = MomentViscous[2];
          CFx_Visc[iMarker]         = ForceViscous[0];
          CFy_Visc[iMarker]         = ForceViscous[1];
          CFz_Visc[iMarker]         = ForceViscous[2];
          CoPx_Visc[iMarker]        =  -MomentY_Force[0];
          CoPz_Visc[iMarker]        = MomentY_Force[2];
          CT_Visc[iMarker]          = -CFz_Visc[iMarker];
          CQ_Visc[iMarker]          = -CMz_Visc[iMarker];
          CMerit_Visc[iMarker]      = CT_Visc[iMarker] / (CQ_Visc[iMarker] + EPS);
        }

        AllBound_CD_Visc       += CD_Visc[iMarker];
        AllBound_CL_Visc       += CL_Visc[iMarker];
        AllBound_CSF_Visc  += CSF_Visc[iMarker];
        AllBound_CMx_Visc         += CMx_Visc[iMarker];
        AllBound_CMy_Visc         += CMy_Visc[iMarker];
        AllBound_CMz_Visc         += CMz_Visc[iMarker];
        AllBound_CFx_Visc         += CFx_Visc[iMarker];
        AllBound_CFy_Visc         += CFy_Visc[iMarker];
        AllBound_CFz_Visc         += CFz_Visc[iMarker];
        AllBound_CoPx_Visc        += CoPx_Visc[iMarker];
        AllBound_CoPy_Visc        += CoPy_Visc[iMarker];
        AllBound_CoPz_Visc        += CoPz_Visc[iMarker];
        AllBound_CT_Visc          += CT_Visc[iMarker];
        AllBound_CQ_Visc          += CQ_Visc[iMarker];

        /*--- Compute the coefficients per surface ---*/

        for (iMarker_Monitoring = 0; iMarker_Monitoring < config->GetnMarker_Monitoring(); iMarker_Monitoring++) {
          Monitoring_Tag = config->GetMarker_Monitoring_TagBound(iMarker_Monitoring);
          Marker_Tag = config->GetMarker_All_TagBound(iMarker);
          if (Marker_Tag == Monitoring_Tag) {
            Surface_CL_Visc[iMarker_Monitoring]      += CL_Visc[iMarker];
            Surface_CD_Visc[iMarker_Monitoring]      += CD_Visc[iMarker];
            Surface_CSF_Visc[iMarker_Monitoring] += CSF_Visc[iMarker];
            Surface_CEff_Visc[iMarker_Monitoring]       += CEff_Visc[iMarker];
            Surface_CFx_Visc[iMarker_Monitoring]        += CFx_Visc[iMarker];
            Surface_CFy_Visc[iMarker_Monitoring]        += CFy_Visc[iMarker];
            Surface_CFz_Visc[iMarker_Monitoring]        += CFz_Visc[iMarker];
            Surface_CMx_Visc[iMarker_Monitoring]        += CMx_Visc[iMarker];
            Surface_CMy_Visc[iMarker_Monitoring]        += CMy_Visc[iMarker];
            Surface_CMz_Visc[iMarker_Monitoring]        += CMz_Visc[iMarker];
          }
        }

      }

    }
  }

  /*--- Update some global coeffients ---*/

  AllBound_CEff_Visc = AllBound_CL_Visc / (AllBound_CD_Visc + EPS);
  AllBound_CMerit_Visc = AllBound_CT_Visc / (AllBound_CQ_Visc + EPS);
  AllBound_MaxHF_Visc = pow(AllBound_MaxHF_Visc, 1.0/MaxNorm);


#ifdef HAVE_MPI

  /*--- Add AllBound information using all the nodes ---*/

  MyAllBound_CD_Visc        = AllBound_CD_Visc;                      AllBound_CD_Visc = 0.0;
  MyAllBound_CL_Visc        = AllBound_CL_Visc;                      AllBound_CL_Visc = 0.0;
  MyAllBound_CSF_Visc   = AllBound_CSF_Visc;                 AllBound_CSF_Visc = 0.0;
  AllBound_CEff_Visc = 0.0;
  MyAllBound_CMx_Visc          = AllBound_CMx_Visc;                        AllBound_CMx_Visc = 0.0;
  MyAllBound_CMy_Visc          = AllBound_CMy_Visc;                        AllBound_CMy_Visc = 0.0;
  MyAllBound_CMz_Visc          = AllBound_CMz_Visc;                        AllBound_CMz_Visc = 0.0;
  MyAllBound_CFx_Visc          = AllBound_CFx_Visc;                        AllBound_CFx_Visc = 0.0;
  MyAllBound_CFy_Visc          = AllBound_CFy_Visc;                        AllBound_CFy_Visc = 0.0;
  MyAllBound_CFz_Visc          = AllBound_CFz_Visc;                        AllBound_CFz_Visc = 0.0;
  MyAllBound_CoPx_Visc          = AllBound_CoPx_Visc;                        AllBound_CoPx_Visc = 0.0;
  MyAllBound_CoPy_Visc          = AllBound_CoPy_Visc;                        AllBound_CoPy_Visc = 0.0;
  MyAllBound_CoPz_Visc          = AllBound_CoPz_Visc;                        AllBound_CoPz_Visc = 0.0;
  MyAllBound_CT_Visc           = AllBound_CT_Visc;                         AllBound_CT_Visc = 0.0;
  MyAllBound_CQ_Visc           = AllBound_CQ_Visc;                         AllBound_CQ_Visc = 0.0;
  AllBound_CMerit_Visc = 0.0;

  SU2_MPI::Allreduce(&MyAllBound_CD_Visc, &AllBound_CD_Visc, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(&MyAllBound_CL_Visc, &AllBound_CL_Visc, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(&MyAllBound_CSF_Visc, &AllBound_CSF_Visc, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  AllBound_CEff_Visc = AllBound_CL_Visc / (AllBound_CD_Visc + EPS);
  SU2_MPI::Allreduce(&MyAllBound_CMx_Visc, &AllBound_CMx_Visc, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(&MyAllBound_CMy_Visc, &AllBound_CMy_Visc, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(&MyAllBound_CMz_Visc, &AllBound_CMz_Visc, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(&MyAllBound_CFx_Visc, &AllBound_CFx_Visc, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(&MyAllBound_CFy_Visc, &AllBound_CFy_Visc, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(&MyAllBound_CFz_Visc, &AllBound_CFz_Visc, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(&MyAllBound_CoPx_Visc, &AllBound_CoPx_Visc, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(&MyAllBound_CoPy_Visc, &AllBound_CoPy_Visc, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(&MyAllBound_CoPz_Visc, &AllBound_CoPz_Visc, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(&MyAllBound_CT_Visc, &AllBound_CT_Visc, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(&MyAllBound_CQ_Visc, &AllBound_CQ_Visc, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  AllBound_CMerit_Visc = AllBound_CT_Visc / (AllBound_CQ_Visc + EPS);
  
  
  /*--- Add the forces on the surfaces using all the nodes ---*/

  MySurface_CL_Visc      = new su2double[config->GetnMarker_Monitoring()];
  MySurface_CD_Visc      = new su2double[config->GetnMarker_Monitoring()];
  MySurface_CSF_Visc = new su2double[config->GetnMarker_Monitoring()];
  MySurface_CEff_Visc       = new su2double[config->GetnMarker_Monitoring()];
  MySurface_CFx_Visc        = new su2double[config->GetnMarker_Monitoring()];
  MySurface_CFy_Visc        = new su2double[config->GetnMarker_Monitoring()];
  MySurface_CFz_Visc        = new su2double[config->GetnMarker_Monitoring()];
  MySurface_CMx_Visc        = new su2double[config->GetnMarker_Monitoring()];
  MySurface_CMy_Visc        = new su2double[config->GetnMarker_Monitoring()];
  MySurface_CMz_Visc        = new su2double[config->GetnMarker_Monitoring()];

  for (iMarker_Monitoring = 0; iMarker_Monitoring < config->GetnMarker_Monitoring(); iMarker_Monitoring++) {

    MySurface_CL_Visc[iMarker_Monitoring]      = Surface_CL_Visc[iMarker_Monitoring];
    MySurface_CD_Visc[iMarker_Monitoring]      = Surface_CD_Visc[iMarker_Monitoring];
    MySurface_CSF_Visc[iMarker_Monitoring] = Surface_CSF_Visc[iMarker_Monitoring];
    MySurface_CEff_Visc[iMarker_Monitoring]       = Surface_CEff_Visc[iMarker_Monitoring];
    MySurface_CFx_Visc[iMarker_Monitoring]        = Surface_CFx_Visc[iMarker_Monitoring];
    MySurface_CFy_Visc[iMarker_Monitoring]        = Surface_CFy_Visc[iMarker_Monitoring];
    MySurface_CFz_Visc[iMarker_Monitoring]        = Surface_CFz_Visc[iMarker_Monitoring];
    MySurface_CMx_Visc[iMarker_Monitoring]        = Surface_CMx_Visc[iMarker_Monitoring];
    MySurface_CMy_Visc[iMarker_Monitoring]        = Surface_CMy_Visc[iMarker_Monitoring];
    MySurface_CMz_Visc[iMarker_Monitoring]        = Surface_CMz_Visc[iMarker_Monitoring];

    Surface_CL_Visc[iMarker_Monitoring]      = 0.0;
    Surface_CD_Visc[iMarker_Monitoring]      = 0.0;
    Surface_CSF_Visc[iMarker_Monitoring] = 0.0;
    Surface_CEff_Visc[iMarker_Monitoring]       = 0.0;
    Surface_CFx_Visc[iMarker_Monitoring]        = 0.0;
    Surface_CFy_Visc[iMarker_Monitoring]        = 0.0;
    Surface_CFz_Visc[iMarker_Monitoring]        = 0.0;
    Surface_CMx_Visc[iMarker_Monitoring]        = 0.0;
    Surface_CMy_Visc[iMarker_Monitoring]        = 0.0;
    Surface_CMz_Visc[iMarker_Monitoring]        = 0.0;
  }

  SU2_MPI::Allreduce(MySurface_CL_Visc, Surface_CL_Visc, config->GetnMarker_Monitoring(), MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(MySurface_CD_Visc, Surface_CD_Visc, config->GetnMarker_Monitoring(), MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(MySurface_CSF_Visc, Surface_CSF_Visc, config->GetnMarker_Monitoring(), MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  for (iMarker_Monitoring = 0; iMarker_Monitoring < config->GetnMarker_Monitoring(); iMarker_Monitoring++)
    Surface_CEff_Visc[iMarker_Monitoring] = Surface_CL_Visc[iMarker_Monitoring] / (Surface_CD_Visc[iMarker_Monitoring] + EPS);
  SU2_MPI::Allreduce(MySurface_CFx_Visc, Surface_CFx_Visc, config->GetnMarker_Monitoring(), MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(MySurface_CFy_Visc, Surface_CFy_Visc, config->GetnMarker_Monitoring(), MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(MySurface_CFz_Visc, Surface_CFz_Visc, config->GetnMarker_Monitoring(), MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(MySurface_CMx_Visc, Surface_CMx_Visc, config->GetnMarker_Monitoring(), MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(MySurface_CMy_Visc, Surface_CMy_Visc, config->GetnMarker_Monitoring(), MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(MySurface_CMz_Visc, Surface_CMz_Visc, config->GetnMarker_Monitoring(), MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  delete [] MySurface_CL_Visc; delete [] MySurface_CD_Visc; delete [] MySurface_CSF_Visc;
  delete [] MySurface_CEff_Visc;  delete [] MySurface_CFx_Visc;   delete [] MySurface_CFy_Visc;
  delete [] MySurface_CFz_Visc;   delete [] MySurface_CMx_Visc;   delete [] MySurface_CMy_Visc;
  delete [] MySurface_CMz_Visc; 

#endif

  /*--- Update the total coefficients (note that all the nodes have the same value)---*/

  Total_CD          += AllBound_CD_Visc;
  Total_CL          += AllBound_CL_Visc;
  Total_CSF         += AllBound_CSF_Visc;
  Total_CEff        = Total_CL / (Total_CD + EPS);
  Total_CMx         += AllBound_CMx_Visc;
  Total_CMy         += AllBound_CMy_Visc;
  Total_CMz         += AllBound_CMz_Visc;
  Total_CFx         += AllBound_CFx_Visc;
  Total_CFy         += AllBound_CFy_Visc;
  Total_CFz         += AllBound_CFz_Visc;
  Total_CoPx        += AllBound_CoPx_Visc;
  Total_CoPy        += AllBound_CoPy_Visc;
  Total_CoPz        += AllBound_CoPz_Visc;
  Total_CT          += AllBound_CT_Visc;
  Total_CQ          += AllBound_CQ_Visc;
  Total_CMerit      = AllBound_CT_Visc / (AllBound_CQ_Visc + EPS);

  /*--- Update the total coefficients per surface (note that all the nodes have the same value)---*/

  for (iMarker_Monitoring = 0; iMarker_Monitoring < config->GetnMarker_Monitoring(); iMarker_Monitoring++) {
    Surface_CL[iMarker_Monitoring]      += Surface_CL_Visc[iMarker_Monitoring];
    Surface_CD[iMarker_Monitoring]      += Surface_CD_Visc[iMarker_Monitoring];
    Surface_CSF[iMarker_Monitoring] += Surface_CSF_Visc[iMarker_Monitoring];
    Surface_CEff[iMarker_Monitoring]       = Surface_CL[iMarker_Monitoring] / (Surface_CD[iMarker_Monitoring] + EPS);
    Surface_CFx[iMarker_Monitoring]        += Surface_CFx_Visc[iMarker_Monitoring];
    Surface_CFy[iMarker_Monitoring]        += Surface_CFy_Visc[iMarker_Monitoring];
    Surface_CFz[iMarker_Monitoring]        += Surface_CFz_Visc[iMarker_Monitoring];
    Surface_CMx[iMarker_Monitoring]        += Surface_CMx_Visc[iMarker_Monitoring];
    Surface_CMy[iMarker_Monitoring]        += Surface_CMy_Visc[iMarker_Monitoring];
    Surface_CMz[iMarker_Monitoring]        += Surface_CMz_Visc[iMarker_Monitoring];
  }


}

