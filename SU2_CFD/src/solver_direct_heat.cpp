/*!
 * \file solution_direct_heat.cpp
 * \brief Main subrotuines for solving the heat equation
 * \author F. Palacios, T. Economon
 * \version 4.3.0 "Cardinal"
 *
 * SU2 Lead Developers: Dr. Francisco Palacios (Francisco.D.Palacios@boeing.com).
 *                      Dr. Thomas D. Economon (economon@stanford.edu).
 *
 * SU2 Developers: Prof. Juan J. Alonso's group at Stanford University.
 *                 Prof. Piero Colonna's group at Delft University of Technology.
 *                 Prof. Nicolas R. Gauger's group at Kaiserslautern University of Technology.
 *                 Prof. Alberto Guardone's group at Polytechnic University of Milan.
 *                 Prof. Rafael Palacios' group at Imperial College London.
 *                 Prof. Edwin van der Weide's group at the University of Twente.
 *                 Prof. Vincent Terrapon's group at the University of Liege.
 *
 * Copyright (C) 2012-2016 SU2, the open-source CFD code.
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

CHeatSolver::CHeatSolver(void) : CSolver() { }

CHeatSolver::CHeatSolver(CGeometry *geometry, CConfig *config, unsigned short iMesh) : CSolver() {
  
  unsigned short iVar, iDim, nLineLets, iMarker;
  unsigned long iPoint, index, iVertex;
  su2double dull_val;

  unsigned short iZone = config->GetiZone();
  unsigned short nZone = geometry->GetnZone();
  bool restart = (config->GetRestart() || config->GetRestart_Flow());
  bool adjoint = (config->GetContinuous_Adjoint()) || (config->GetDiscrete_Adjoint());
  bool dual_time = ((config->GetUnsteady_Simulation() == DT_STEPPING_1ST) ||
                    (config->GetUnsteady_Simulation() == DT_STEPPING_2ND));
  bool time_stepping = config->GetUnsteady_Simulation() == TIME_STEPPING;

  int rank = MASTER_NODE;

  bool flow = (config->GetKind_Solver() != HEAT_EQUATION);
#ifdef HAVE_MPI
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
#endif

  /*--- Dimension of the problem --> temperature is the only conservative variable ---*/

  nVar = 1;
  nPoint = geometry->GetnPoint();
  nPointDomain = geometry->GetnPointDomain();
  
  /*--- Initialize nVarGrad for deallocation ---*/

  nVarGrad = nVar;

  /*--- Define geometry constants in the solver structure ---*/

  nDim = geometry->GetnDim();
  node = new CVariable*[nPoint];
  nMarker = config->GetnMarker_All();

  if (iMesh == MESH_0 || config->GetMGCycle() == FULLMG_CYCLE) {

    /*--- Define some auxiliar vector related with the residual ---*/

    Residual = new su2double[nVar];     for (iVar = 0; iVar < nVar; iVar++) Residual[iVar]  = 0.0;
    Residual_RMS = new su2double[nVar]; for (iVar = 0; iVar < nVar; iVar++) Residual_RMS[iVar]  = 0.0;
    Residual_i = new su2double[nVar];   for (iVar = 0; iVar < nVar; iVar++) Residual_i[iVar]  = 0.0;
    Residual_j = new su2double[nVar];   for (iVar = 0; iVar < nVar; iVar++) Residual_j[iVar]  = 0.0;
    Residual_Max = new su2double[nVar]; for (iVar = 0; iVar < nVar; iVar++) Residual_Max[iVar]  = 0.0;
    Res_Conv      = new su2double[nVar]; for (iVar = 0; iVar < nVar; iVar++) Res_Conv[iVar]      = 0.0;
    Res_Visc      = new su2double[nVar]; for (iVar = 0; iVar < nVar; iVar++) Res_Visc[iVar]      = 0.0;

    /*--- Define some structures for locating max residuals ---*/

    Point_Max = new unsigned long[nVar];
    for (iVar = 0; iVar < nVar; iVar++) Point_Max[iVar] = 0;
    Point_Max_Coord = new su2double*[nVar];
    for (iVar = 0; iVar < nVar; iVar++) {
      Point_Max_Coord[iVar] = new su2double[nDim];
      for (iDim = 0; iDim < nDim; iDim++) Point_Max_Coord[iVar][iDim] = 0.0;
    }

    /*--- Define some auxiliar vector related with the solution ---*/

    Solution = new su2double[nVar];
    Solution_i = new su2double[nVar]; Solution_j = new su2double[nVar];

    /*--- Define some auxiliar vector related with the geometry ---*/

    Vector_i = new su2double[nDim]; Vector_j = new su2double[nDim];

    /*--- Jacobians and vector structures for implicit computations ---*/

    Jacobian_i = new su2double* [nVar];
    Jacobian_j = new su2double* [nVar];
    for (iVar = 0; iVar < nVar; iVar++) {
      Jacobian_i[iVar] = new su2double [nVar];
      Jacobian_j[iVar] = new su2double [nVar];
    }

    /*--- Initialization of the structure of the whole Jacobian ---*/

    if (rank == MASTER_NODE) cout << "Initialize Jacobian structure (heat equation)." << endl;
    Jacobian.Initialize(nPoint, nPointDomain, nVar, nVar, true, geometry, config);

    if ((config->GetKind_Linear_Solver_Prec() == LINELET) ||
        (config->GetKind_Linear_Solver() == SMOOTHER_LINELET)) {
      nLineLets = Jacobian.BuildLineletPreconditioner(geometry, config);
      if (rank == MASTER_NODE) cout << "Compute linelet structure. " << nLineLets << " elements in each line (average)." << endl;
    }

    LinSysSol.Initialize(nPoint, nPointDomain, nVar, 0.0);
    LinSysRes.Initialize(nPoint, nPointDomain, nVar, 0.0);

    if (config->GetExtraOutput()) {
      if (nDim == 2) { nOutputVariables = 13; }
      else if (nDim == 3) { nOutputVariables = 19; }
      OutputVariables.Initialize(nPoint, nPointDomain, nOutputVariables, 0.0);
      OutputHeadingNames = new string[nOutputVariables];
    }

    /*--- Computation of gradients by least squares ---*/

    if (config->GetKind_Gradient_Method() == WEIGHTED_LEAST_SQUARES) {
      /*--- S matrix := inv(R)*traspose(inv(R)) ---*/
      Smatrix = new su2double* [nDim];
      for (iDim = 0; iDim < nDim; iDim++)
        Smatrix[iDim] = new su2double [nDim];

      /*--- c vector := transpose(WA)*(Wb) ---*/

      cvector = new su2double* [nVar];
      for (iVar = 0; iVar < nVar; iVar++)
        cvector[iVar] = new su2double [nDim];
    }

  }
  /*--- Store the value of the characteristic primitive variables at the boundaries ---*/

  //CharacPrimVar = new su2double** [nMarker];
  //for (iMarker = 0; iMarker < nMarker; iMarker++) {
  //  CharacPrimVar[iMarker] = new su2double* [geometry->nVertex[iMarker]];
  //  for (iVertex = 0; iVertex < geometry->nVertex[iMarker]; iVertex++) {
  //    CharacPrimVar[iMarker][iVertex] = new su2double [nPrimVar];
  //    for (iVar = 0; iVar < nPrimVar; iVar++) {
  //      CharacPrimVar[iMarker][iVertex][iVar] = 0.0;
  //    }
  //  }
  //}


  /*--- Non-dimensionalization of heat equation */
  config->SetTemperature_Ref(config->GetTemperature_FreeStream());
  config->SetTemperature_FreeStreamND(config->GetTemperature_FreeStream()/config->GetTemperature_Ref());

  if(!flow) {

  config->SetViscosity_Ref(config->GetViscosity_FreeStream());
  config->SetViscosity_FreeStreamND(config->GetViscosity_FreeStream()/config->GetViscosity_Ref());
  cout << config->GetTemperature_Ref() << endl;
  }


  if (!restart || (iMesh != MESH_0)) {
    for (iPoint = 0; iPoint < nPoint; iPoint++)
      node[iPoint] = new CHeatVariable(config->GetTemperature_FreeStreamND(), nDim, nVar, config);
  }
  else {

    /*--- Restart the solution from file information ---*/
    ifstream restart_file;
    string filename = config->GetSolution_FlowFileName();
    su2double Density, StaticEnergy, Laminar_Viscosity, nu, nu_hat, muT = 0.0, U[5];
    int Unst_RestartIter;

    /*--- Modify file name for multizone problems ---*/
    if (nZone >1)
      filename= config->GetMultizone_FileName(filename, iZone);

    /*--- Modify file name for an unsteady restart ---*/
    if (dual_time) {
      if (adjoint) {
        Unst_RestartIter = SU2_TYPE::Int(config->GetUnst_AdjointIter()) - 1;
      } else if (config->GetUnsteady_Simulation() == DT_STEPPING_1ST)
        Unst_RestartIter = SU2_TYPE::Int(config->GetUnst_RestartIter())-1;
      else
        Unst_RestartIter = SU2_TYPE::Int(config->GetUnst_RestartIter())-2;
      filename = config->GetUnsteady_FileName(filename, Unst_RestartIter);
    }

    /*--- Modify file name for a simple unsteady restart ---*/

    if (time_stepping) {
      if (adjoint) {
        Unst_RestartIter = SU2_TYPE::Int(config->GetUnst_AdjointIter()) - 1;
      } else {
        Unst_RestartIter = SU2_TYPE::Int(config->GetUnst_RestartIter())-1;
      }
      filename = config->GetUnsteady_FileName(filename, Unst_RestartIter);
    }

    /*--- Open the restart file, throw an error if this fails. ---*/
    restart_file.open(filename.data(), ios::in);
    if (restart_file.fail()) {
      cout << "There is no turbulent restart file!!" << endl;
      exit(EXIT_FAILURE);
    }

    /*--- In case this is a parallel simulation, we need to perform the
     Global2Local index transformation first. ---*/
    long *Global2Local;
    Global2Local = new long[geometry->GetGlobal_nPointDomain()];
    /*--- First, set all indices to a negative value by default ---*/
    for (iPoint = 0; iPoint < geometry->GetGlobal_nPointDomain(); iPoint++) {
      Global2Local[iPoint] = -1;
    }
    /*--- Now fill array with the transform values only for local points ---*/
    for (iPoint = 0; iPoint < nPointDomain; iPoint++) {
      Global2Local[geometry->node[iPoint]->GetGlobalIndex()] = iPoint;
    }

    /*--- Read all lines in the restart file ---*/
    long iPoint_Local; unsigned long iPoint_Global = 0; string text_line;

    /*--- The first line is the header ---*/
    getline (restart_file, text_line);

    while (getline (restart_file, text_line)) {
      istringstream point_line(text_line);

      /*--- Retrieve local index. If this node from the restart file lives
       on a different processor, the value of iPoint_Local will be -1.
       Otherwise, the local index for this node on the current processor
       will be returned and used to instantiate the vars. ---*/
      iPoint_Local = Global2Local[iPoint_Global];
      if (iPoint_Local >= 0) {

        if (nDim == 2) point_line >> index >> dull_val >> dull_val >> dull_val >> dull_val >> dull_val >> Solution[0];
        if (nDim == 3) point_line >> index >> dull_val >> dull_val >> dull_val >> dull_val >> dull_val >> dull_val >> dull_val >> Solution[0];


        /*--- Instantiate the solution at this node, note that the eddy viscosity should be recomputed ---*/
        node[iPoint_Local] = new CHeatVariable(Solution[0], nDim, nVar, config);
      }
      iPoint_Global++;
    }

    /*--- Instantiate the variable class with an arbitrary solution
     at any halo/periodic nodes. The initial solution can be arbitrary,
     because a send/recv is performed immediately in the solver. ---*/
    for (iPoint = nPointDomain; iPoint < nPoint; iPoint++) {
      node[iPoint] = new CHeatVariable(Solution[0], nDim, nVar, config);
    }

    /*--- Close the restart file ---*/
    restart_file.close();

    /*--- Free memory needed for the transformation ---*/
    delete [] Global2Local;
  }
}

CHeatSolver::~CHeatSolver(void) {  

}


void CHeatSolver::Preprocessing(CGeometry *geometry, CSolver **solver_container, CConfig *config, unsigned short iMesh, unsigned short iRKStep, unsigned short RunTime_EqSystem, bool Output) {

  unsigned long iPoint;

  for (iPoint = 0; iPoint < nPoint; iPoint ++) {

    /*--- Initialize the residual vector ---*/

    LinSysRes.SetBlock_Zero(iPoint);

  }
  /*--- Initialize the Jacobian matrices ---*/

  Jacobian.SetValZero();

  if (config->GetKind_Gradient_Method() == GREEN_GAUSS) SetSolution_Gradient_GG(geometry, config);
  if (config->GetKind_Gradient_Method() == WEIGHTED_LEAST_SQUARES) SetSolution_Gradient_LS(geometry, config);
}


void CHeatSolver::Postprocessing(CGeometry *geometry, CSolver **solver_container, CConfig *config, unsigned short iMesh) {  }

void CHeatSolver::Source_Residual(CGeometry *geometry, CSolver **solver_container, CNumerics *numerics, CNumerics *second_numerics, CConfig *config, unsigned short iMesh) {  }

void CHeatSolver::Upwind_Residual(CGeometry *geometry, CSolver **solver_container, CNumerics *numerics, CConfig *config, unsigned short iMesh) {

  su2double *V_i, *V_j, Temp_i, Temp_j;
  unsigned long iEdge, iPoint, jPoint;
  bool flow = (config->GetKind_Solver() != HEAT_EQUATION);

  if(flow) {
      for (iEdge = 0; iEdge < geometry->GetnEdge(); iEdge++) {

        /*--- Points in edge ---*/
        iPoint = geometry->edge[iEdge]->GetNode(0);
        jPoint = geometry->edge[iEdge]->GetNode(1);
        numerics->SetNormal(geometry->edge[iEdge]->GetNormal());

        /*--- Primitive variables w/o reconstruction ---*/
        V_i = solver_container[FLOW_SOL]->node[iPoint]->GetPrimitive();
        V_j = solver_container[FLOW_SOL]->node[jPoint]->GetPrimitive();
        numerics->SetPrimitive(V_i, V_j);

        Temp_i = node[iPoint]->GetSolution(0);
        Temp_j = node[jPoint]->GetSolution(0);
        numerics->SetTemperature(Temp_i, Temp_j);

        numerics->ComputeResidual(Residual, Jacobian_i, Jacobian_j, config);

        LinSysRes.AddBlock(iPoint, Residual);
        LinSysRes.SubtractBlock(jPoint, Residual);

        /*--- Implicit part ---*/

        Jacobian.AddBlock(iPoint, iPoint, Jacobian_i);
        Jacobian.AddBlock(iPoint, jPoint, Jacobian_j);
        Jacobian.SubtractBlock(jPoint, iPoint, Jacobian_i);
        Jacobian.SubtractBlock(jPoint, jPoint, Jacobian_j);
        }
  }

}

void CHeatSolver::Viscous_Residual(CGeometry *geometry, CSolver **solver_container, CNumerics *numerics, CConfig *config, unsigned short iMesh, unsigned short iRKStep) {

  su2double eddy_viscosity_i, eddy_viscosity_j, Temp_i, Temp_j, **Temp_i_Grad, **Temp_j_Grad;
  unsigned long iEdge, iPoint, jPoint;

  bool flow = (config->GetKind_Solver() != HEAT_EQUATION);

  for (iEdge = 0; iEdge < geometry->GetnEdge(); iEdge++) {

    iPoint = geometry->edge[iEdge]->GetNode(0);
    jPoint = geometry->edge[iEdge]->GetNode(1);

    /*--- Points coordinates, and normal vector ---*/

    numerics->SetCoord(geometry->node[iPoint]->GetCoord(),
                       geometry->node[jPoint]->GetCoord());
    numerics->SetNormal(geometry->edge[iEdge]->GetNormal());

    Temp_i_Grad = node[iPoint]->GetGradient();
    Temp_j_Grad = node[jPoint]->GetGradient();
    numerics->SetConsVarGradient(Temp_i_Grad, Temp_j_Grad);

    /*--- Primitive variables w/o reconstruction ---*/
    Temp_i = node[iPoint]->GetSolution(0);
    Temp_j = node[jPoint]->GetSolution(0);
    numerics->SetTemperature(Temp_i, Temp_j);

    /*--- Laminar viscosity to compute thermal conductivity ---*/
    numerics->SetLaminarViscosity(config->GetViscosity_FreeStreamND(), config->GetViscosity_FreeStreamND());

    /*--- Eddy viscosity to compute thermal conductivity ---*/
    if (flow) {
      eddy_viscosity_i = solver_container[FLOW_SOL]->node[iPoint]->GetEddyViscosity();
      eddy_viscosity_j = solver_container[FLOW_SOL]->node[jPoint]->GetEddyViscosity();
    }
    else {
      eddy_viscosity_i = 0.0;
      eddy_viscosity_j = 0.0;
    }

    numerics->SetEddyViscosity(eddy_viscosity_i,eddy_viscosity_j);

    /*--- Compute residual, and Jacobians ---*/

    numerics->ComputeResidual(Residual, Jacobian_i, Jacobian_j, config);

    /*--- Add and subtract residual, and update Jacobians ---*/

    LinSysRes.SubtractBlock(iPoint, Residual);
    LinSysRes.AddBlock(jPoint, Residual);

    Jacobian.SubtractBlock(iPoint, iPoint, Jacobian_i);
    Jacobian.SubtractBlock(iPoint, jPoint, Jacobian_j);
    Jacobian.AddBlock(jPoint, iPoint, Jacobian_i);
    Jacobian.AddBlock(jPoint, jPoint, Jacobian_j);
}


void CHeatSolver::BC_Isothermal_Wall(CGeometry *geometry, CSolver **solver_container, CNumerics *conv_numerics, CNumerics *visc_numerics, CConfig *config,
                                       unsigned short val_marker) {

  unsigned long iPoint, iVertex, Point_Normal;
  unsigned short iVar, iDim;
  su2double *Normal, *Coord_i, *Coord_j, Area, dist_ij, eddy_viscosity, laminar_viscosity, thermal_conductivity, Twall, dTdn, Prandtl_Lam, Prandtl_Turb;
  bool implicit = (config->GetKind_TimeIntScheme_Flow() == EULER_IMPLICIT);

  bool flow = (config->GetKind_Solver() != HEAT_EQUATION);

  Prandtl_Lam = config->GetPrandtl_Lam();
  Prandtl_Turb = config->GetPrandtl_Turb();
  laminar_viscosity = config->GetViscosity_FreeStreamND();

  string Marker_Tag = config->GetMarker_All_TagBound(val_marker);
  Twall = config->GetIsothermal_Temperature(Marker_Tag)/config->GetTemperature_Ref();

  for (iVertex = 0; iVertex < geometry->nVertex[val_marker]; iVertex++) {

    iPoint = geometry->vertex[val_marker][iVertex]->GetNode();

    if (geometry->node[iPoint]->GetDomain()) {

        Point_Normal = geometry->vertex[val_marker][iVertex]->GetNormal_Neighbor();

        Normal = geometry->vertex[val_marker][iVertex]->GetNormal();
        Area = 0.0;
        for (iDim = 0; iDim < nDim; iDim++) Area += Normal[iDim]*Normal[iDim];
        Area = sqrt (Area);

        Coord_i = geometry->node[iPoint]->GetCoord();
        Coord_j = geometry->node[Point_Normal]->GetCoord();
        dist_ij = 0;
        for (iDim = 0; iDim < nDim; iDim++)
          dist_ij += (Coord_j[iDim]-Coord_i[iDim])*(Coord_j[iDim]-Coord_i[iDim]);
        dist_ij = sqrt(dist_ij);

        dTdn = -(node[Point_Normal]->GetSolution(0) - Twall)/dist_ij;

        if(flow)
          eddy_viscosity = solver_container[FLOW_SOL]->node[iPoint]->GetEddyViscosity();
        else
          eddy_viscosity = 0.0;

        thermal_conductivity = eddy_viscosity/Prandtl_Turb + laminar_viscosity/Prandtl_Lam;

        Res_Visc[0] = thermal_conductivity*dTdn*Area;

        if(implicit) {

          Jacobian_i[0][0] = -thermal_conductivity/dist_ij * Area;

        }
        /*--- Viscous contribution to the residual at the wall ---*/

        LinSysRes.SubtractBlock(iPoint, Res_Visc);
        Jacobian.SubtractBlock(iPoint, iPoint, Jacobian_i);
    }

  }

}

void CHeatSolver::BC_Inlet(CGeometry *geometry, CSolver **solver_container,
                            CNumerics *conv_numerics, CNumerics *visc_numerics, CConfig *config, unsigned short val_marker) {

  unsigned short iDim;
  unsigned long iVertex, iPoint, Point_Normal;
  su2double *Flow_Dir,  Vel_Mag;
  su2double *V_inlet, *V_domain;

  bool flow                 = (config->GetKind_Solver() != HEAT_EQUATION);
  bool viscous              = config->GetViscous();
  bool grid_movement        = config->GetGrid_Movement();
  bool implicit             = (config->GetKind_TimeIntScheme_Flow() == EULER_IMPLICIT);
  string Marker_Tag         = config->GetMarker_All_TagBound(val_marker);

  su2double *Normal = new su2double[nDim];

  su2double *Coord_i, *Coord_j, Area, dist_ij, eddy_viscosity, laminar_viscosity, thermal_conductivity, Twall, dTdn, Prandtl_Lam, Prandtl_Turb;
  Prandtl_Lam = config->GetPrandtl_Lam();
  Prandtl_Turb = config->GetPrandtl_Turb();
  laminar_viscosity = config->GetViscosity_FreeStreamND();
  Twall = config->GetTemperature_FreeStreamND();

  for (iVertex = 0; iVertex < geometry->nVertex[val_marker]; iVertex++) {

    iPoint = geometry->vertex[val_marker][iVertex]->GetNode();

    if (geometry->node[iPoint]->GetDomain()) {

        geometry->vertex[val_marker][iVertex]->GetNormal(Normal);
        for (iDim = 0; iDim < nDim; iDim++) Normal[iDim] = -Normal[iDim];

        if(flow) {
            /*--- Normal vector for this vertex (negate for outward convention) ---*/

            conv_numerics->SetNormal(Normal);

            /*--- Retrieve solution at this boundary node ---*/

            V_domain = solver_container[FLOW_SOL]->node[iPoint]->GetPrimitive();

            /*--- Retrieve the specified velocity for the inlet. ---*/

            Vel_Mag  = config->GetInlet_Ptotal(Marker_Tag)/config->GetVelocity_Ref();
            Flow_Dir = config->GetInlet_FlowDir(Marker_Tag);

            V_inlet = solver_container[FLOW_SOL]->GetCharacPrimVar(val_marker, iVertex);

            for (iDim = 0; iDim < nDim; iDim++)
              V_inlet[iDim+1] = Vel_Mag*Flow_Dir[iDim];

            conv_numerics->SetPrimitive(V_domain, V_inlet);

            if (grid_movement)
              conv_numerics->SetGridVel(geometry->node[iPoint]->GetGridVel(), geometry->node[iPoint]->GetGridVel());

            conv_numerics->SetTemperature(node[iPoint]->GetSolution(0), config->GetTemperature_FreeStreamND());

            /*--- Compute the residual using an upwind scheme ---*/

            conv_numerics->ComputeResidual(Residual, Jacobian_i, Jacobian_j, config);

            /*--- Update residual value ---*/

            LinSysRes.AddBlock(iPoint, Residual);

            /*--- Jacobian contribution for implicit integration ---*/

            if (implicit)
              Jacobian.AddBlock(iPoint, iPoint, Jacobian_i);
        }

        /*--- Viscous contribution ---*/

        if (viscous && false ) {

          /*--- Set the normal vector and the coordinates ---*/

          visc_numerics->SetNormal(Normal);
          visc_numerics->SetCoord(geometry->node[iPoint]->GetCoord(), geometry->node[Point_Normal]->GetCoord());

          /*--- Primitive variables, and gradient ---*/

          visc_numerics->SetConsVarGradient(node[iPoint]->GetGradient(), node[iPoint]->GetGradient());

          visc_numerics->SetTemperature(node[iPoint]->GetSolution(0), config->GetTemperature_FreeStreamND());

          /*--- Laminar viscosity  ---*/
          visc_numerics->SetLaminarViscosity(config->GetViscosity_FreeStreamND(), config->GetViscosity_FreeStreamND());

          /*--- Turbulent viscosity  ---*/

          if(flow)
            visc_numerics->SetEddyViscosity(solver_container[FLOW_SOL]->node[iPoint]->GetEddyViscosity(), solver_container[FLOW_SOL]->node[iPoint]->GetEddyViscosity());
          else
            visc_numerics->SetEddyViscosity(0.0, 0.0);

          /*--- Compute and update residual ---*/

          visc_numerics->ComputeResidual(Residual, Jacobian_i, Jacobian_j, config);
          LinSysRes.SubtractBlock(iPoint, Residual);

          /*--- Jacobian contribution for implicit integration ---*/

          if (implicit)
            Jacobian.SubtractBlock(iPoint, iPoint, Jacobian_i);

        }

        if (viscous || true) {


          Point_Normal = geometry->vertex[val_marker][iVertex]->GetNormal_Neighbor();

          geometry->vertex[val_marker][iVertex]->GetNormal(Normal);
          Area = 0.0;
          for (iDim = 0; iDim < nDim; iDim++) Area += Normal[iDim]*Normal[iDim];
          Area = sqrt (Area);

          Coord_i = geometry->node[iPoint]->GetCoord();
          Coord_j = geometry->node[Point_Normal]->GetCoord();
          dist_ij = 0;
          for (iDim = 0; iDim < nDim; iDim++)
            dist_ij += (Coord_j[iDim]-Coord_i[iDim])*(Coord_j[iDim]-Coord_i[iDim]);
          dist_ij = sqrt(dist_ij);

          dTdn = -(node[Point_Normal]->GetSolution(0) - Twall)/dist_ij;

          if(flow)
            eddy_viscosity = solver_container[FLOW_SOL]->node[iPoint]->GetEddyViscosity();
          else
            eddy_viscosity = 0.0;

          thermal_conductivity = eddy_viscosity/Prandtl_Turb + laminar_viscosity/Prandtl_Lam;

          Res_Visc[0] = thermal_conductivity*dTdn*Area;

          if(implicit) {

            Jacobian_i[0][0] = -thermal_conductivity/dist_ij * Area;

          }
          /*--- Viscous contribution to the residual at the wall ---*/

          LinSysRes.SubtractBlock(iPoint, Res_Visc);
          Jacobian.SubtractBlock(iPoint, iPoint, Jacobian_i);

        }
    }
  }

  /*--- Free locally allocated memory ---*/
  delete [] Normal;

}

void CHeatSolver::BC_Outlet(CGeometry *geometry, CSolver **solver_container,
                             CNumerics *conv_numerics, CNumerics *visc_numerics, CConfig *config, unsigned short val_marker) {

  unsigned short iDim;
  unsigned long iVertex, iPoint, Point_Normal;
  su2double *Flow_Dir,  Vel_Mag;
  su2double *V_outlet, *V_domain;

  bool flow                 = (config->GetKind_Solver() != HEAT_EQUATION);
  bool viscous              = config->GetViscous();
  bool grid_movement        = config->GetGrid_Movement();
  bool implicit             = (config->GetKind_TimeIntScheme_Flow() == EULER_IMPLICIT);

  su2double *Normal = new su2double[nDim];

  for (iVertex = 0; iVertex < geometry->nVertex[val_marker]; iVertex++) {

    iPoint = geometry->vertex[val_marker][iVertex]->GetNode();

    if (geometry->node[iPoint]->GetDomain()) {

        Point_Normal = geometry->vertex[val_marker][iVertex]->GetNormal_Neighbor();

        /*--- Normal vector for this vertex (negate for outward convention) ---*/

        geometry->vertex[val_marker][iVertex]->GetNormal(Normal);
        for (iDim = 0; iDim < nDim; iDim++) Normal[iDim] = -Normal[iDim];

        if(flow) {
            conv_numerics->SetNormal(Normal);

            /*--- Retrieve solution at this boundary node ---*/

            V_domain = solver_container[FLOW_SOL]->node[iPoint]->GetPrimitive();

            /*--- Retrieve the specified velocity for the inlet. ---*/

            V_outlet = solver_container[FLOW_SOL]->GetCharacPrimVar(val_marker, iVertex);
            for (iDim = 0; iDim < nDim; iDim++)
              V_outlet[iDim+1] = solver_container[FLOW_SOL]->node[Point_Normal]->GetPrimitive(iDim+1);

            conv_numerics->SetPrimitive(V_domain, V_outlet);

            if (grid_movement)
              conv_numerics->SetGridVel(geometry->node[iPoint]->GetGridVel(), geometry->node[iPoint]->GetGridVel());

            conv_numerics->SetTemperature(node[iPoint]->GetSolution(0), node[Point_Normal]->GetSolution(0));

            /*--- Compute the residual using an upwind scheme ---*/

            conv_numerics->ComputeResidual(Residual, Jacobian_i, Jacobian_j, config);

            /*--- Update residual value ---*/

            LinSysRes.AddBlock(iPoint, Residual);

            /*--- Jacobian contribution for implicit integration ---*/

            if (implicit)
              Jacobian.AddBlock(iPoint, iPoint, Jacobian_i);
        }


        /*--- Viscous contribution ---*/

        if (false) {

          /*--- Set the normal vector and the coordinates ---*/

          visc_numerics->SetNormal(Normal);
          visc_numerics->SetCoord(geometry->node[iPoint]->GetCoord(), geometry->node[Point_Normal]->GetCoord());

          /*--- Primitive variables, and gradient ---*/

          visc_numerics->SetConsVarGradient(node[iPoint]->GetGradient(), node[iPoint]->GetGradient());

          visc_numerics->SetTemperature(node[iPoint]->GetSolution(0), node[iPoint]->GetSolution(0));

          /*--- Laminar viscosity  ---*/
          visc_numerics->SetLaminarViscosity(config->GetViscosity_FreeStreamND(), config->GetViscosity_FreeStreamND());

          /*--- Turbulent viscosity  ---*/
          if(flow)
            visc_numerics->SetEddyViscosity(solver_container[FLOW_SOL]->node[iPoint]->GetEddyViscosity(), solver_container[FLOW_SOL]->node[Point_Normal]->GetEddyViscosity());
          else
            visc_numerics->SetEddyViscosity(0.0, 0.0);
          /*--- Compute and update residual ---*/

          visc_numerics->ComputeResidual(Residual, Jacobian_i, Jacobian_j, config);
          LinSysRes.SubtractBlock(iPoint, Residual);

          /*--- Jacobian contribution for implicit integration ---*/

          if (implicit)
            Jacobian.SubtractBlock(iPoint, iPoint, Jacobian_i);

        }
    }
  }

  /*--- Free locally allocated memory ---*/
  delete [] Normal;

}

void CHeatSolver::ImplicitEuler_Iteration(CGeometry *geometry, CSolver **solver_container, CConfig *config) {

  unsigned short iVar;
  unsigned long iPoint, total_index;
  su2double Delta, Vol;

  //cout << "Norm of right hand side: " << LinSysRes.norm() << endl;

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

 //   Delta = Vol / (solver_container[FLOW_SOL]->node[iPoint]->GetDelta_Time());
 //   Jacobian.AddVal2Diag(iPoint, Delta);

    /*--- Right hand side of the system (-Residual) and initial guess (x = 0) ---*/

    for (iVar = 0; iVar < nVar; iVar++) {
      total_index = iPoint*nVar+iVar;
      LinSysRes[total_index] = - LinSysRes[total_index];
      LinSysSol[total_index] = 0.0;
      AddRes_RMS(iVar, LinSysRes[total_index]*LinSysRes[total_index]);
      //AddRes_Max(iVar, fabs(LinSysRes[total_index]), geometry->node[iPoint]->GetGlobalIndex(), geometry->node[iPoint]->GetCoord());
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
  system.Solve(Jacobian, LinSysRes, LinSysSol, geometry, config);

  cout << LinSysSol.norm() << endl;

  for (iPoint = 0; iPoint < nPointDomain; iPoint++) {
    for (iVar = 0; iVar < nVar; iVar++) {
      node[iPoint]->AddSolution(iVar, LinSysSol[iPoint*nVar+iVar]);
    }
  }

  /*--- MPI solution ---*/

  Set_MPI_Solution(geometry, config);

  /*--- Compute the root mean square residual ---*/

  SetResidual_RMS(geometry, config);

}

void CHeatSolver::Set_MPI_Solution(CGeometry *geometry, CConfig *config) {
  unsigned short iVar, iMarker, iPeriodic_Index, MarkerS, MarkerR;
  unsigned long iVertex, iPoint, nVertexS, nVertexR, nBufferS_Vector, nBufferR_Vector;
  su2double rotMatrix[3][3], *angles, theta, cosTheta, sinTheta, phi, cosPhi, sinPhi, psi, cosPsi, sinPsi, *Buffer_Receive_U = NULL, *Buffer_Send_U = NULL;

#ifdef HAVE_MPI
  int send_to, receive_from;
  MPI_Status status;
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
      Buffer_Receive_U = new su2double [nBufferR_Vector];
      Buffer_Send_U = new su2double[nBufferS_Vector];

      /*--- Copy the solution that should be sended ---*/
      for (iVertex = 0; iVertex < nVertexS; iVertex++) {
        iPoint = geometry->vertex[MarkerS][iVertex]->GetNode();
        for (iVar = 0; iVar < nVar; iVar++)
          Buffer_Send_U[iVar*nVertexS+iVertex] = node[iPoint]->GetSolution(iVar);
      }

#ifdef HAVE_MPI

      /*--- Send/Receive information using Sendrecv ---*/
      SU2_MPI::Sendrecv(Buffer_Send_U, nBufferS_Vector, MPI_DOUBLE, send_to, 0,
                        Buffer_Receive_U, nBufferR_Vector, MPI_DOUBLE, receive_from, 0, MPI_COMM_WORLD, &status);

#else

      /*--- Receive information without MPI ---*/
      for (iVertex = 0; iVertex < nVertexR; iVertex++) {
        for (iVar = 0; iVar < nVar; iVar++)
          Buffer_Receive_U[iVar*nVertexR+iVertex] = Buffer_Send_U[iVar*nVertexR+iVertex];
      }

#endif

      /*--- Deallocate send buffer ---*/
      delete [] Buffer_Send_U;

      /*--- Do the coordinate transformation ---*/
      for (iVertex = 0; iVertex < nVertexR; iVertex++) {
        /*--- Copy transformed conserved variables back into buffer. ---*/
        for (iVar = 0; iVar < nVar; iVar++)
          node[iPoint]->SetSolution(iVar, Buffer_Receive_U[iVar*nVertexR+iVertex]);

      }

      /*--- Deallocate receive buffer ---*/
      delete [] Buffer_Receive_U;

    }

  }

}

void CHeatSolver::Set_MPI_Solution_Old(CGeometry *geometry, CConfig *config) {
  unsigned short iVar, iMarker, iPeriodic_Index, MarkerS, MarkerR;
  unsigned long iVertex, iPoint, nVertexS, nVertexR, nBufferS_Vector, nBufferR_Vector;
  su2double rotMatrix[3][3], *angles, theta, cosTheta, sinTheta, phi, cosPhi, sinPhi, psi, cosPsi, sinPsi,
  *Buffer_Receive_U = NULL, *Buffer_Send_U = NULL;

#ifdef HAVE_MPI
  int send_to, receive_from;
  MPI_Status status;
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
      Buffer_Receive_U = new su2double [nBufferR_Vector];
      Buffer_Send_U = new su2double[nBufferS_Vector];

      /*--- Copy the solution old that should be sended ---*/
      for (iVertex = 0; iVertex < nVertexS; iVertex++) {
        iPoint = geometry->vertex[MarkerS][iVertex]->GetNode();
        for (iVar = 0; iVar < nVar; iVar++)
          Buffer_Send_U[iVar*nVertexS+iVertex] = node[iPoint]->GetSolution_Old(iVar);
      }

#ifdef HAVE_MPI

      /*--- Send/Receive information using Sendrecv ---*/
      SU2_MPI::Sendrecv(Buffer_Send_U, nBufferS_Vector, MPI_DOUBLE, send_to, 0,
                        Buffer_Receive_U, nBufferR_Vector, MPI_DOUBLE, receive_from, 0, MPI_COMM_WORLD, &status);

#else

      /*--- Receive information without MPI ---*/
      for (iVertex = 0; iVertex < nVertexR; iVertex++) {
        for (iVar = 0; iVar < nVar; iVar++)
          Buffer_Receive_U[iVar*nVertexR+iVertex] = Buffer_Send_U[iVar*nVertexR+iVertex];
      }

#endif

      /*--- Deallocate send buffer ---*/
      delete [] Buffer_Send_U;

      /*--- Do the coordinate transformation ---*/
      for (iVertex = 0; iVertex < nVertexR; iVertex++) {

        /*--- Copy transformed conserved variables back into buffer. ---*/
        for (iVar = 0; iVar < nVar; iVar++)
          node[iPoint]->SetSolution_Old(iVar, Buffer_Receive_U[iVar*nVertexR+iVertex]);

      }

      /*--- Deallocate receive buffer ---*/
      delete [] Buffer_Receive_U;

    }

  }
}

void CHeatSolver::Set_MPI_Solution_Gradient(CGeometry *geometry, CConfig *config) {
  unsigned short iVar, iDim, iMarker, iPeriodic_Index, MarkerS, MarkerR;
  unsigned long iVertex, iPoint, nVertexS, nVertexR, nBufferS_Vector, nBufferR_Vector;
  su2double rotMatrix[3][3], *angles, theta, cosTheta, sinTheta, phi, cosPhi, sinPhi, psi, cosPsi, sinPsi,
  *Buffer_Receive_Gradient = NULL, *Buffer_Send_Gradient = NULL;

  su2double **Gradient = new su2double* [nVar];
  for (iVar = 0; iVar < nVar; iVar++)
    Gradient[iVar] = new su2double[nDim];

#ifdef HAVE_MPI
  int send_to, receive_from;
  MPI_Status status;
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
      nBufferS_Vector = nVertexS*nVar*nDim;        nBufferR_Vector = nVertexR*nVar*nDim;

      /*--- Allocate Receive and send buffers  ---*/
      Buffer_Receive_Gradient = new su2double [nBufferR_Vector];
      Buffer_Send_Gradient = new su2double[nBufferS_Vector];

      /*--- Copy the solution old that should be sended ---*/
      for (iVertex = 0; iVertex < nVertexS; iVertex++) {
        iPoint = geometry->vertex[MarkerS][iVertex]->GetNode();
        for (iVar = 0; iVar < nVar; iVar++)
          for (iDim = 0; iDim < nDim; iDim++)
            Buffer_Send_Gradient[iDim*nVar*nVertexS+iVar*nVertexS+iVertex] = node[iPoint]->GetGradient(iVar, iDim);
      }

#ifdef HAVE_MPI

      /*--- Send/Receive information using Sendrecv ---*/
      SU2_MPI::Sendrecv(Buffer_Send_Gradient, nBufferS_Vector, MPI_DOUBLE, send_to, 0,
                        Buffer_Receive_Gradient, nBufferR_Vector, MPI_DOUBLE, receive_from, 0, MPI_COMM_WORLD, &status);

#else

      /*--- Receive information without MPI ---*/
      for (iVertex = 0; iVertex < nVertexR; iVertex++) {
        for (iVar = 0; iVar < nVar; iVar++)
          for (iDim = 0; iDim < nDim; iDim++)
            Buffer_Receive_Gradient[iDim*nVar*nVertexR+iVar*nVertexR+iVertex] = Buffer_Send_Gradient[iDim*nVar*nVertexR+iVar*nVertexR+iVertex];
      }

#endif

      /*--- Deallocate send buffer ---*/
      delete [] Buffer_Send_Gradient;

      /*--- Do the coordinate transformation ---*/
      for (iVertex = 0; iVertex < nVertexR; iVertex++) {

        /*--- Store the received information ---*/
        for (iVar = 0; iVar < nVar; iVar++)
          for (iDim = 0; iDim < nDim; iDim++)
            node[iPoint]->SetGradient(iVar, iDim, Buffer_Receive_Gradient[iDim*nVar*nVertexR+iVar*nVertexR+iVertex]);

      }

      /*--- Deallocate receive buffer ---*/
      delete [] Buffer_Receive_Gradient;

    }

  }

  for (iVar = 0; iVar < nVar; iVar++)
    delete [] Gradient[iVar];
  delete [] Gradient;

}

void CHeatSolver::SetResidual_DualTime(CGeometry *geometry, CSolver **solver_container, CConfig *config,
                                        unsigned short iRKStep, unsigned short iMesh, unsigned short RunTime_EqSystem) {

  /*--- Local variables ---*/

  unsigned short iVar, jVar, iMarker, iDim;
  unsigned long iPoint, jPoint, iEdge, iVertex;

  su2double *U_time_nM1, *U_time_n, *U_time_nP1;
  su2double Volume_nM1, Volume_nP1, TimeStep;
  su2double *Normal = NULL, *GridVel_i = NULL, *GridVel_j = NULL, Residual_GCL;

  bool implicit       = (config->GetKind_TimeIntScheme_Flow() == EULER_IMPLICIT);
  bool FlowEq         = (RunTime_EqSystem == RUNTIME_FLOW_SYS);
  bool AdjEq          = (RunTime_EqSystem == RUNTIME_ADJFLOW_SYS);
  bool grid_movement  = config->GetGrid_Movement();

  /*--- Store the physical time step ---*/

  TimeStep = config->GetDelta_UnstTimeND();

  /*--- Compute the dual time-stepping source term for static meshes ---*/

  if (!grid_movement) {

    /*--- Loop over all nodes (excluding halos) ---*/

    for (iPoint = 0; iPoint < nPointDomain; iPoint++) {

      /*--- Retrieve the solution at time levels n-1, n, and n+1. Note that
       we are currently iterating on U^n+1 and that U^n & U^n-1 are fixed,
       previous solutions that are stored in memory. ---*/

      U_time_nM1 = node[iPoint]->GetSolution_time_n1();
      U_time_n   = node[iPoint]->GetSolution_time_n();
      U_time_nP1 = node[iPoint]->GetSolution();

      /*--- CV volume at time n+1. As we are on a static mesh, the volume
       of the CV will remained fixed for all time steps. ---*/

      Volume_nP1 = geometry->node[iPoint]->GetVolume();

      /*--- Compute the dual time-stepping source term based on the chosen
       time discretization scheme (1st- or 2nd-order).---*/

      for (iVar = 0; iVar < nVar; iVar++) {
        if (config->GetUnsteady_Simulation() == DT_STEPPING_1ST)
          Residual[iVar] = (U_time_nP1[iVar] - U_time_n[iVar])*Volume_nP1 / TimeStep;
        if (config->GetUnsteady_Simulation() == DT_STEPPING_2ND)
          Residual[iVar] = ( 3.0*U_time_nP1[iVar] - 4.0*U_time_n[iVar]
                            +1.0*U_time_nM1[iVar])*Volume_nP1 / (2.0*TimeStep);
      }
      if ((FlowEq || AdjEq)) Residual[0] = 0.0;

      /*--- Store the residual and compute the Jacobian contribution due
       to the dual time source term. ---*/

      LinSysRes.AddBlock(iPoint, Residual);
      if (implicit) {
        for (iVar = 0; iVar < nVar; iVar++) {
          for (jVar = 0; jVar < nVar; jVar++) Jacobian_i[iVar][jVar] = 0.0;
          if (config->GetUnsteady_Simulation() == DT_STEPPING_1ST)
            Jacobian_i[iVar][iVar] = Volume_nP1 / TimeStep;
          if (config->GetUnsteady_Simulation() == DT_STEPPING_2ND)
            Jacobian_i[iVar][iVar] = (Volume_nP1*3.0)/(2.0*TimeStep);
        }
        if ((FlowEq || AdjEq)) Jacobian_i[0][0] = 0.0;
        Jacobian.AddBlock(iPoint, iPoint, Jacobian_i);
      }
    }

  }
}
