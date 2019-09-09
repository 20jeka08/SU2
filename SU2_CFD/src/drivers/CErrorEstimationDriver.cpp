/*!
 * \file CErrorEstimationDriver.cpp
 * \brief Functions for error estimation.
 * \author B. Munguía
 * \version 6.2.0 "Falcon"
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

#include "../../include/drivers/CErrorEstimationDriver.hpp"

CErrorEstimationDriver::CErrorEstimationDriver(void) { }

CErrorEstimationDriver::CErrorEstimationDriver(CDiscAdjSinglezoneDriver* disc_adj_driver,
                                               unsigned short val_nZone,
                                               SU2_Comm MPICommunicator):nZone(val_nZone), fsi(false), fem_solver(false) {

  SU2_MPI::SetComm(MPICommunicator);

  rank = SU2_MPI::GetRank();
  size = SU2_MPI::GetSize();

  geometry = NULL;
  solver   = NULL;
  config   = NULL;
  output   = NULL;
  nInst    = NULL;

  /*--- Definition of the containers for all possible zones. ---*/

  geometry = new CGeometry***[nZone];
  solver   = new CSolver****[nZone];
  config   = new CConfig*[nZone];
  nInst    = new unsigned short[nZone];

  for (iZone = 0; iZone < nZone; iZone++) {
    solver[iZone]   = NULL;
    geometry[iZone] = NULL;
    config[iZone]   = disc_adj_driver->GetConfig(iZone);
    nInst[iZone]    = 1;
  }

  /*--- Preprocessing of the geometry for all zones. In this routine, the edge-
   based data structure is constructed, i.e. node and cell neighbors are
   identified and linked, and face areas are computed ---*/

  for (iZone = 0; iZone < nZone; iZone++) {

    geometry[iZone] = new CGeometry** [nInst[iZone]];

    for (iInst = 0; iInst < nInst[iZone]; iInst++){

      geometry[iZone][iInst] = NULL;
      geometry[iZone][iInst] = new CGeometry* [MESH_0+1];
      geometry[iZone][iInst][MESH_0] = disc_adj_driver->GetGeometry(iZone, iInst, MESH_0);

    }

  }

  nDim = geometry[ZONE_0][INST_0][MESH_0]->GetnDim();

  for (iZone = 0; iZone < nZone; iZone++) {

    /*--- Definition of the solver class: solver[#ZONES][#INSTANCES][#MG_GRIDS][#EQ_SYSTEMS].
    The solver classes are specific to a particular set of governing equations,
    and they contain the subroutines with instructions for computing each spatial
    term of the PDE, i.e. loops over the edges to compute convective and viscous
    fluxes, loops over the nodes to compute source terms, and routines for
    imposing various boundary condition type for the PDE. ---*/

    solver[iZone] = new CSolver*** [nInst[iZone]];


    for (iInst = 0; iInst < nInst[iZone]; iInst++){
      solver[iZone][iInst] = NULL;
      solver[iZone][iInst] = new CSolver** [MESH_0+1];
      solver[iZone][iInst][MESH_0] = NULL;
      solver[iZone][iInst][MESH_0] = new CSolver* [MAX_SOLS];
      for (iSol = 0; iSol < MAX_SOLS; iSol++)
        solver[iZone][iInst][MESH_0][iSol] = disc_adj_driver->GetSolver(iZone, iInst, MESH_0, iSol);

    } // End of loop over iInst

  }

  /*--- Definition of the output class (one for all zones). The output class
   manages the writing of all restart, volume solution, surface solution,
   surface comma-separated value, and convergence history files (both in serial
   and in parallel). ---*/

  output = new COutput(config[ZONE_0]);

}

CErrorEstimationDriver::CErrorEstimationDriver(char* confFile,
                                               unsigned short val_nZone,
                                               SU2_Comm MPICommunicator):config_file_name(confFile), nZone(val_nZone), fsi(false), fem_solver(false) {

  char zone_file_name[MAX_STRING_SIZE];

  SU2_MPI::SetComm(MPICommunicator);

  rank = SU2_MPI::GetRank();
  size = SU2_MPI::GetSize();

  geometry = NULL;
  solver   = NULL;
  config   = NULL;
  output   = NULL;
  nInst    = NULL;

  /*--- Definition of the containers for all possible zones. ---*/

  geometry = new CGeometry***[nZone];
  solver   = new CSolver****[nZone];
  config   = new CConfig*[nZone];
  nInst    = new unsigned short[nZone];

  for (iZone = 0; iZone < nZone; iZone++) {
    solver[iZone]   = NULL;
    geometry[iZone] = NULL;
    config[iZone]   = NULL;
    nInst[iZone]    = 1;
  }

  /*--- Initialize the configuration of the driver ---*/

  driver_config = new CConfig(config_file_name, SU2_CFD, nZone, false); 

  /*--- Loop over all zones to initialize the various classes. In most
   cases, nZone is equal to one. This represents the solution of a partial
   differential equation on a single block, unstructured mesh. ---*/

  for (iZone = 0; iZone < nZone; iZone++) {

    /*--- Definition of the configuration option class for all zones. In this
     constructor, the input configuration file is parsed and all options are
     read and stored. ---*/

    if (driver_config->GetnConfigFiles() > 0){
      strcpy(zone_file_name, driver_config->GetConfigFilename(iZone).c_str());
      config[iZone] = new CConfig(driver_config, zone_file_name, SU2_MET, iZone, nZone, true);
    }
    else{
      config[iZone] = new CConfig(driver_config, config_file_name, SU2_MET, iZone, nZone, true);
    }

    /*--- Set the MPI communicator ---*/

    config[iZone]->SetMPICommunicator(MPICommunicator);

    /* --- For the output config, enable restart reading (to export sensor fields) and change grid file to target mesh ---*/

    config[iZone]->SetRestart(true);

    config[iZone]->SetMGLevels(0);

  }

  /*--- Set the multizone part of the problem. ---*/
  if (driver_config->GetMultizone_Problem()){
    for (iZone = 0; iZone < nZone; iZone++) {
      /*--- Set the interface markers for multizone ---*/
      config[iZone]->SetMultizone(driver_config, config);
    }
  }

  /*--- Preprocessing of the config and mesh files. In this routine, the config file is read
   and it is determined whether a problem is single physics or multiphysics. . ---*/

  Input_Preprocessing(config, geometry);

  /*--- Preprocessing of the geometry for all zones. In this routine, the edge-
   based data structure is constructed, i.e. node and cell neighbors are
   identified and linked, and face areas are computed ---*/

  if (rank == MASTER_NODE)
    cout << endl <<"------------------------- Geometry Preprocessing ------------------------" << endl;

  /*--- Determine whether or not the FEM solver is used, which decides the
   type of geometry classes that are instantiated. Only adapted for single-zone problems ---*/
  fem_solver = ((config[ZONE_0]->GetKind_Solver() == FEM_EULER)          ||
                (config[ZONE_0]->GetKind_Solver() == FEM_NAVIER_STOKES)  ||
                (config[ZONE_0]->GetKind_Solver() == FEM_RANS)           ||
                (config[ZONE_0]->GetKind_Solver() == FEM_LES)            ||
                (config[ZONE_0]->GetKind_Solver() == DISC_ADJ_FEM_EULER) ||
                (config[ZONE_0]->GetKind_Solver() == DISC_ADJ_FEM_NS)    ||
                (config[ZONE_0]->GetKind_Solver() == DISC_ADJ_FEM_RANS));

  if( fem_solver ) {
    switch( config[ZONE_0]->GetKind_FEM_Flow() ) {
      case DG: {
        Geometrical_Preprocessing_DGFEM(config, geometry);
        break;
      }
    }
  }
  else {
    Geometrical_Preprocessing(config, geometry);
  }

  for (iZone = 0; iZone < nZone; iZone++) {

    for (iInst = 0; iInst < nInst[iZone]; iInst++){

      /*--- Computation of positive surface area in the z-plane which is used for
        the calculation of force coefficient (non-dimensionalization). ---*/

      geometry[iZone][iInst][MESH_0]->SetPositive_ZArea(config[iZone]);

    }

  }

  for (iZone = 0; iZone < nZone; iZone++) {

    /*--- Definition of the solver class: solver[#ZONES][#INSTANCES][#MG_GRIDS][#EQ_SYSTEMS].
    The solver classes are specific to a particular set of governing equations,
    and they contain the subroutines with instructions for computing each spatial
    term of the PDE, i.e. loops over the edges to compute convective and viscous
    fluxes, loops over the nodes to compute source terms, and routines for
    imposing various boundary condition type for the PDE. ---*/

    if (rank == MASTER_NODE)
      cout << endl <<"------------------------- Solver Preprocessing --------------------------" << endl;

    solver[iZone] = new CSolver*** [nInst[iZone]];


    for (iInst = 0; iInst < nInst[iZone]; iInst++){
      solver[iZone][iInst] = NULL;
      solver[iZone][iInst] = new CSolver** [MESH_0+1];
      solver[iZone][iInst][MESH_0] = NULL;
      solver[iZone][iInst][MESH_0] = new CSolver* [MAX_SOLS];
      for (iSol = 0; iSol < MAX_SOLS; iSol++)
        solver[iZone][iInst][MESH_0][iSol] = NULL;
      
      Solver_Preprocessing(solver[iZone], geometry[iZone],
                           config[iZone], iInst);

    } // End of loop over iInst

  }

  /*--- Definition of the output class (one for all zones). The output class
   manages the writing of all restart, volume solution, surface solution,
   surface comma-separated value, and convergence history files (both in serial
   and in parallel). ---*/

  output = new COutput(config[ZONE_0]);

}

CErrorEstimationDriver::~CErrorEstimationDriver(void) { }

void CErrorEstimationDriver::Input_Preprocessing(CConfig **config, CGeometry ****geometry) {

  bool fem_solver = false; 

  for (iZone = 0; iZone < nZone; iZone++) {

    /*--- Determine whether or not the FEM solver is used, which decides the
     type of geometry classes that are instantiated. ---*/
    fem_solver = ((config[iZone]->GetKind_Solver() == FEM_EULER)         ||
                  (config[iZone]->GetKind_Solver() == FEM_NAVIER_STOKES) ||
                  (config[iZone]->GetKind_Solver() == FEM_RANS)          ||
                  (config[iZone]->GetKind_Solver() == FEM_LES)           ||
                  (config[iZone]->GetKind_Solver() == DISC_ADJ_FEM_EULER) ||
                  (config[iZone]->GetKind_Solver() == DISC_ADJ_FEM_NS)    ||
                  (config[iZone]->GetKind_Solver() == DISC_ADJ_FEM_RANS));

    /*--- Read the number of instances for each zone ---*/

    nInst[iZone] = config[iZone]->GetnTimeInstances();

    geometry[iZone] = new CGeometry** [nInst[iZone]];

    for (iInst = 0; iInst < nInst[iZone]; iInst++){

      config[iZone]->SetiInst(iInst);

      /*--- Definition of the geometry class to store the primal grid in the
     partitioning process. ---*/

      CGeometry *geometry_aux = NULL;

      /*--- All ranks process the grid and call ParMETIS for partitioning ---*/

      geometry_aux = new CPhysicalGeometry(config[iZone], iZone, nZone);

      /*--- Color the initial grid and set the send-receive domains (ParMETIS) ---*/

      if ( fem_solver ) geometry_aux->SetColorFEMGrid_Parallel(config[iZone]);
      else              geometry_aux->SetColorGrid_Parallel(config[iZone]);

      /*--- Allocate the memory of the current domain, and divide the grid
     between the ranks. ---*/

      geometry[iZone][iInst]            = NULL;
      geometry[iZone][iInst]            = new CGeometry *[MESH_0+1];
      geometry[iZone][iInst][MESH_0]    = NULL;


      if( fem_solver ) {
        switch( config[iZone]->GetKind_FEM_Flow() ) {
          case DG: {
            geometry[iZone][iInst][MESH_0] = new CMeshFEM_DG(geometry_aux, config[iZone]);
            break;
          }

          default: {
            SU2_MPI::Error("Unknown FEM flow solver.", CURRENT_FUNCTION);
            break;
          }
        }
      }
      else {

        /*--- Build the grid data structures using the ParMETIS coloring. ---*/
        
        geometry[iZone][iInst][MESH_0] = new CPhysicalGeometry(geometry_aux, config[iZone]);

      }

      /*--- Deallocate the memory of geometry_aux and solver_aux ---*/

      delete geometry_aux;

      /*--- Add the Send/Receive boundaries ---*/
      geometry[iZone][iInst][MESH_0]->SetSendReceive(config[iZone]);

      /*--- Add the Send/Receive boundaries ---*/
      geometry[iZone][iInst][MESH_0]->SetBoundaries(config[iZone]);

    }

  }

}

void CErrorEstimationDriver::Geometrical_Preprocessing(CConfig **config, CGeometry ****geometry) {

  bool fea = false;

  for (iZone = 0; iZone < nZone; iZone++) {

    fea = ((config[iZone]->GetKind_Solver() == FEM_ELASTICITY) ||
        (config[iZone]->GetKind_Solver() == DISC_ADJ_FEM));

    for (iInst = 0; iInst < nInst[iZone]; iInst++){

      /*--- Compute elements surrounding points, points surrounding points ---*/
  
      if (rank == MASTER_NODE) cout << "Setting point connectivity." << endl;
      geometry[iZone][iInst][MESH_0]->SetPoint_Connectivity();
      
      /*--- Renumbering points using Reverse Cuthill McKee ordering ---*/
      
      if (rank == MASTER_NODE) cout << "Renumbering points (Reverse Cuthill McKee Ordering)." << endl;
      geometry[iZone][iInst][MESH_0]->SetRCM_Ordering(config[iZone]);
      
      /*--- recompute elements surrounding points, points surrounding points ---*/
      
      if (rank == MASTER_NODE) cout << "Recomputing point connectivity." << endl;
      geometry[iZone][iInst][MESH_0]->SetPoint_Connectivity();
      
      /*--- Compute elements surrounding elements ---*/
      
      if (rank == MASTER_NODE) cout << "Setting element connectivity." << endl;
      geometry[iZone][iInst][MESH_0]->SetElement_Connectivity();
      
      /*--- Check the orientation before computing geometrical quantities ---*/
      
      geometry[iZone][iInst][MESH_0]->SetBoundVolume();
      if (config[iZone]->GetReorientElements()) {
        if (rank == MASTER_NODE) cout << "Checking the numerical grid orientation." << endl;
        geometry[iZone][iInst][MESH_0]->Check_IntElem_Orientation(config[iZone]);
        geometry[iZone][iInst][MESH_0]->Check_BoundElem_Orientation(config[iZone]);
      }
      
      /*--- Create the edge structure ---*/
      
      if (rank == MASTER_NODE) cout << "Identifying edges and vertices." << endl;
      geometry[iZone][iInst][MESH_0]->SetEdges();
      geometry[iZone][iInst][MESH_0]->SetVertex(config[iZone]);
      
      /*--- Compute cell center of gravity ---*/
      
      if ((rank == MASTER_NODE) && (!fea)) cout << "Computing centers of gravity." << endl;
      geometry[iZone][iInst][MESH_0]->SetCoord_CG();
      
      /*--- Create the control volume structures ---*/
      
      if ((rank == MASTER_NODE) && (!fea)) cout << "Setting the control volume structure." << endl;
      geometry[iZone][iInst][MESH_0]->SetControlVolume(config[iZone], ALLOCATE);
      geometry[iZone][iInst][MESH_0]->SetBoundControlVolume(config[iZone], ALLOCATE);
      
      /*--- Visualize a dual control volume if requested ---*/
      
      if ((config[iZone]->GetVisualize_CV() >= 0) &&
          (config[iZone]->GetVisualize_CV() < (long)geometry[iZone][iInst][MESH_0]->GetnPointDomain()))
        geometry[iZone][iInst][MESH_0]->VisualizeControlVolume(config[iZone], UPDATE);
      
      /*--- Identify closest normal neighbor ---*/
      
      if (rank == MASTER_NODE) cout << "Searching for the closest normal neighbors to the surfaces." << endl;
      geometry[iZone][iInst][MESH_0]->FindNormal_Neighbor(config[iZone]);
      
      /*--- Store the global to local mapping. ---*/
      
      if (rank == MASTER_NODE) cout << "Storing a mapping from global to local point index." << endl;
      geometry[iZone][iInst][MESH_0]->SetGlobal_to_Local_Point();
      
      /*--- Compute the surface curvature ---*/
      
      if ((rank == MASTER_NODE) && (!fea)) cout << "Compute the surface curvature." << endl;
      geometry[iZone][iInst][MESH_0]->ComputeSurf_Curvature(config[iZone]);
      
      /*--- Check for periodicity and disable MG if necessary. ---*/
      
      if (rank == MASTER_NODE) cout << "Checking for periodicity." << endl;
      geometry[iZone][iInst][MESH_0]->Check_Periodicity(config[iZone]);

    }

    nDim = geometry[ZONE_0][INST_0][MESH_0]->GetnDim();

  }

  /*--- Create the data structure for MPI point-to-point communications. ---*/
  
  for (iZone = 0; iZone < nZone; iZone++) {
    for (iInst = 0; iInst < nInst[iZone]; iInst++) {
        geometry[iZone][iInst][MESH_0]->PreprocessP2PComms(geometry[iZone][iInst][MESH_0], config[iZone]);
    }
  }
  
  /*--- Perform a few preprocessing routines and communications. ---*/
  
  for (iZone = 0; iZone < nZone; iZone++) {
    for (iInst = 0; iInst < nInst[iZone]; iInst++) {
        
      /*--- Compute the max length. ---*/
        
      if ((rank == MASTER_NODE) && (!fea)) cout << "Finding max control volume width." << endl;
      geometry[iZone][iInst][MESH_0]->SetMaxLength(config[iZone]);
        
      /*--- Communicate the number of neighbors. This is needed for
       some centered schemes and for multigrid in parallel. ---*/
        
      if ((rank == MASTER_NODE) && (size > SINGLE_NODE) && (!fea)) cout << "Communicating number of neighbors." << endl;
      geometry[iZone][iInst][MESH_0]->InitiateComms(geometry[iZone][iInst][MESH_0], config[iZone], NEIGHBORS);
      geometry[iZone][iInst][MESH_0]->CompleteComms(geometry[iZone][iInst][MESH_0], config[iZone], NEIGHBORS);
    }
  }

}

void CErrorEstimationDriver::Geometrical_Preprocessing_DGFEM(CConfig **config, CGeometry ****geometry) { }

void CErrorEstimationDriver::Solver_Preprocessing(CSolver ****solver, CGeometry ***geometry,
                                                   CConfig *config, unsigned short val_iInst) {
  
  bool euler, ns, turbulent,
  fem_euler, fem_ns, fem_turbulent, fem_transition,
  adj_euler, adj_ns, adj_turb,
  heat_fvm,
  fem, disc_adj_fem,
  spalart_allmaras, neg_spalart_allmaras, menter_sst, transition,
  template_solver, disc_adj, disc_adj_turb, disc_adj_heat,
  fem_dg_flow, fem_dg_shock_persson,
  e_spalart_allmaras, comp_spalart_allmaras, e_comp_spalart_allmaras;  

  /*--- Initialize some useful booleans ---*/

  euler            = false;  ns              = false;  turbulent     = false;
  fem_euler        = false;  fem_ns          = false;  fem_turbulent = false;
  adj_euler        = false;  adj_ns          = false;  adj_turb      = false;
  spalart_allmaras = false;  menter_sst      = false;  disc_adj_turb = false;
  neg_spalart_allmaras = false;
  disc_adj         = false;
  fem              = false;  disc_adj_fem     = false;
  heat_fvm         = false;  disc_adj_heat    = false;
  transition       = false;  fem_transition   = false;
  template_solver  = false;
  fem_dg_flow      = false;  fem_dg_shock_persson = false;
  e_spalart_allmaras = false; comp_spalart_allmaras = false; e_comp_spalart_allmaras = false;
  
  bool compressible   = (config->GetKind_Regime() == COMPRESSIBLE);
  bool incompressible = (config->GetKind_Regime() == INCOMPRESSIBLE);

  /*--- Assign booleans ---*/
  
  switch (config->GetKind_Solver()) {
    case TEMPLATE_SOLVER: template_solver = true; break;
    case EULER : euler = true; break;
    case NAVIER_STOKES: ns = true; heat_fvm = config->GetWeakly_Coupled_Heat(); break;
    case RANS : ns = true; turbulent = true; if (config->GetKind_Trans_Model() == LM) transition = true; heat_fvm = config->GetWeakly_Coupled_Heat(); break;
    case FEM_EULER : fem_euler = true; break;
    case FEM_NAVIER_STOKES: fem_ns = true; break;
    case FEM_RANS : fem_ns = true; fem_turbulent = true; if(config->GetKind_Trans_Model() == LM) fem_transition = true; break;
    case FEM_LES : fem_ns = true; break;
    case HEAT_EQUATION_FVM: heat_fvm = true; break;
    case FEM_ELASTICITY: fem = true; break;
    case ADJ_EULER : euler = true; adj_euler = true; break;
    case ADJ_NAVIER_STOKES : ns = true; turbulent = (config->GetKind_Turb_Model() != NONE); adj_ns = true; break;
    case ADJ_RANS : ns = true; turbulent = true; adj_ns = true; adj_turb = (!config->GetFrozen_Visc_Cont()); break;
    case DISC_ADJ_EULER: euler = true; disc_adj = true; break;
    case DISC_ADJ_NAVIER_STOKES: ns = true; disc_adj = true; heat_fvm = config->GetWeakly_Coupled_Heat(); break;
    case DISC_ADJ_RANS: ns = true; turbulent = true; disc_adj = true; disc_adj_turb = (!config->GetFrozen_Visc_Disc()); heat_fvm = config->GetWeakly_Coupled_Heat(); break;
    case DISC_ADJ_FEM_EULER: fem_euler = true; disc_adj = true; break;
    case DISC_ADJ_FEM_NS: fem_ns = true; disc_adj = true; break;
    case DISC_ADJ_FEM_RANS: fem_ns = true; fem_turbulent = true; disc_adj = true; if(config->GetKind_Trans_Model() == LM) fem_transition = true; break;
    case DISC_ADJ_FEM: fem = true; disc_adj_fem = true; break;
    case DISC_ADJ_HEAT: heat_fvm = true; disc_adj_heat = true; break;
  }
  
  /*--- Determine the kind of FEM solver used for the flow. ---*/

  switch( config->GetKind_FEM_Flow() ) {
    case DG: fem_dg_flow = true; break;
  }

  /*--- Determine the kind of shock capturing method for FEM DG solver. ---*/

  switch( config->GetKind_FEM_DG_Shock() ) {
    case PERSSON: fem_dg_shock_persson = true; break;
  }

  /*--- Assign turbulence model booleans ---*/

  if (turbulent || fem_turbulent){
    switch (config->GetKind_Turb_Model()) {
      case SA:     spalart_allmaras = true;     break;
      case SA_NEG: neg_spalart_allmaras = true; break;
      case SST:    menter_sst = true;           break;
      case SA_E:   e_spalart_allmaras = true;   break;
      case SA_COMP: comp_spalart_allmaras = true; break;
      case SA_E_COMP: e_comp_spalart_allmaras = true; break;
      default: SU2_MPI::Error("Specified turbulence model unavailable or none selected", CURRENT_FUNCTION); break;
    }
  }
  
  /*--- Definition of the Class for the solution: solver[DOMAIN][INSTANCE][MESH_0][EQUATION]. Note that euler, ns
   and potential are incompatible, they use the same position in sol container ---*/


    /*--- Allocate solution for a template problem ---*/

  if (template_solver) {
    solver[val_iInst][MESH_0][TEMPLATE_SOL] = new CTemplateSolver(geometry[val_iInst][MESH_0], config);
    DOFsPerPoint += solver[val_iInst][MESH_0][TEMPLATE_SOL]->GetnVar();
  }

    /*--- Allocate solution for direct problem, and run the preprocessing and postprocessing ---*/
    /*--- Note that we need both direct and adjoint for the error estimate ---*/

  if (euler || disc_adj) {
    if (compressible) {
      solver[val_iInst][MESH_0][FLOW_SOL] = new CEulerSolver(geometry[val_iInst][MESH_0], config, MESH_0);
      solver[val_iInst][MESH_0][FLOW_SOL]->Preprocessing(geometry[val_iInst][MESH_0], solver[val_iInst][MESH_0], config, MESH_0, NO_RK_ITER, RUNTIME_FLOW_SYS, false);
    }
    if (incompressible) {
      solver[val_iInst][MESH_0][FLOW_SOL] = new CIncEulerSolver(geometry[val_iInst][MESH_0], config, MESH_0);
      solver[val_iInst][MESH_0][FLOW_SOL]->Preprocessing(geometry[val_iInst][MESH_0], solver[val_iInst][MESH_0], config, MESH_0, NO_RK_ITER, RUNTIME_FLOW_SYS, false);
    }
    DOFsPerPoint += solver[val_iInst][MESH_0][FLOW_SOL]->GetnVar();
  }
  if (ns) {
    if (compressible) {
      solver[val_iInst][MESH_0][FLOW_SOL] = new CNSSolver(geometry[val_iInst][MESH_0], config, MESH_0);
    }
    if (incompressible) {
      solver[val_iInst][MESH_0][FLOW_SOL] = new CIncNSSolver(geometry[val_iInst][MESH_0], config, MESH_0);
    }
    DOFsPerPoint += solver[val_iInst][MESH_0][FLOW_SOL]->GetnVar();
  }
  if (turbulent) {
    if (spalart_allmaras || e_spalart_allmaras || comp_spalart_allmaras || e_comp_spalart_allmaras || neg_spalart_allmaras) {
      solver[val_iInst][MESH_0][TURB_SOL] = new CTurbSASolver(geometry[val_iInst][MESH_0], config, MESH_0, solver[val_iInst][MESH_0][FLOW_SOL]->GetFluidModel() );
      solver[val_iInst][MESH_0][FLOW_SOL]->Preprocessing(geometry[val_iInst][MESH_0], solver[val_iInst][MESH_0], config, MESH_0, NO_RK_ITER, RUNTIME_FLOW_SYS, false);
      solver[val_iInst][MESH_0][TURB_SOL]->Postprocessing(geometry[val_iInst][MESH_0], solver[val_iInst][MESH_0], config, MESH_0);
    }
    else if (menter_sst) {
      solver[val_iInst][MESH_0][TURB_SOL] = new CTurbSSTSolver(geometry[val_iInst][MESH_0], config, MESH_0);
      solver[val_iInst][MESH_0][FLOW_SOL]->Preprocessing(geometry[val_iInst][MESH_0], solver[val_iInst][MESH_0], config, MESH_0, NO_RK_ITER, RUNTIME_FLOW_SYS, false);
      solver[val_iInst][MESH_0][TURB_SOL]->Postprocessing(geometry[val_iInst][MESH_0], solver[val_iInst][MESH_0], config, MESH_0);
      solver[val_iInst][MESH_0][FLOW_SOL]->Preprocessing(geometry[val_iInst][MESH_0], solver[val_iInst][MESH_0], config, MESH_0, NO_RK_ITER, RUNTIME_FLOW_SYS, false);
    }
    DOFsPerPoint += solver[val_iInst][MESH_0][TURB_SOL]->GetnVar();
    if (transition) {
      solver[val_iInst][MESH_0][TRANS_SOL] = new CTransLMSolver(geometry[val_iInst][MESH_0], config, MESH_0);
      DOFsPerPoint += solver[val_iInst][MESH_0][TRANS_SOL]->GetnVar();
    }
  }

    /*--- Allocate solution for adjoint problem ---*/

  if (disc_adj || euler || ns || turbulent) {
    solver[val_iInst][MESH_0][ADJFLOW_SOL] = new CDiscAdjSolver(geometry[val_iInst][MESH_0], config, solver[val_iInst][MESH_0][FLOW_SOL], RUNTIME_FLOW_SYS, MESH_0);
    DOFsPerPoint += solver[val_iInst][MESH_0][ADJFLOW_SOL]->GetnVar();
    if (disc_adj_turb || turbulent) {
      solver[val_iInst][MESH_0][ADJTURB_SOL] = new CDiscAdjSolver(geometry[val_iInst][MESH_0], config, solver[val_iInst][MESH_0][TURB_SOL], RUNTIME_TURB_SYS, MESH_0);
      DOFsPerPoint += solver[val_iInst][MESH_0][ADJTURB_SOL]->GetnVar();
    }
  }  

  /*--- Check for restarts and use the LoadRestart() routines. ---*/

  bool update_geo = true;
  if (config->GetFSI_Simulation()) update_geo = false;

  Solver_Restart(solver, geometry, config, update_geo, val_iInst);

}

void CErrorEstimationDriver::Solver_Restart(CSolver ****solver, CGeometry ***geometry,
                             CConfig *config, bool update_geo, unsigned short val_iInst) {

  bool euler, ns, turbulent,
  adj_euler, adj_ns, adj_turb,
  heat_fvm, fem, fem_euler, fem_ns, fem_dg_flow,
  template_solver, disc_adj, disc_adj_fem, disc_adj_turb, disc_adj_heat;
  int val_iter = 0;

  /*--- Initialize some useful booleans ---*/

  euler            = false;  ns           = false;  turbulent   = false;
  adj_euler        = false;  adj_ns       = false;  adj_turb    = false;
  fem_euler        = false;  fem_ns       = false;  fem_dg_flow = false;
  disc_adj         = false;
  fem              = false;  disc_adj_fem     = false;
  disc_adj_turb    = false;
  heat_fvm         = false;  disc_adj_heat    = false;
  template_solver  = false;

  /*--- Check for restarts and use the LoadRestart() routines. ---*/

  bool restart      = config->GetRestart();
  bool restart_flow = config->GetRestart_Flow();
  bool no_restart   = false;

  /*--- Adjust iteration number for unsteady restarts. ---*/

  bool dual_time = ((config->GetUnsteady_Simulation() == DT_STEPPING_1ST) ||
                    (config->GetUnsteady_Simulation() == DT_STEPPING_2ND));
  bool time_stepping = config->GetUnsteady_Simulation() == TIME_STEPPING;
  bool adjoint = (config->GetDiscrete_Adjoint() || config->GetContinuous_Adjoint());
  bool dynamic = (config->GetDynamic_Analysis() == DYNAMIC); // Dynamic simulation (FSI).

  if (dual_time) {
    if (adjoint) val_iter = SU2_TYPE::Int(config->GetUnst_AdjointIter())-1;
    else if (config->GetUnsteady_Simulation() == DT_STEPPING_1ST)
      val_iter = SU2_TYPE::Int(config->GetUnst_RestartIter())-1;
    else val_iter = SU2_TYPE::Int(config->GetUnst_RestartIter())-2;
  }

  if (time_stepping) {
    if (adjoint) val_iter = SU2_TYPE::Int(config->GetUnst_AdjointIter())-1;
    else val_iter = SU2_TYPE::Int(config->GetUnst_RestartIter())-1;
  }

  /*--- Assign booleans ---*/

  switch (config->GetKind_Solver()) {
    case TEMPLATE_SOLVER: template_solver = true; break;
    case EULER : euler = true; break;
    case NAVIER_STOKES: ns = true; heat_fvm = config->GetWeakly_Coupled_Heat(); break;
    case RANS : ns = true; turbulent = true; heat_fvm = config->GetWeakly_Coupled_Heat(); break;
    case FEM_EULER : fem_euler = true; break;
    case FEM_NAVIER_STOKES: fem_ns = true; break;
    case FEM_RANS : fem_ns = true; break;
    case FEM_LES : fem_ns = true; break;
    case HEAT_EQUATION_FVM: heat_fvm = true; break;
    case FEM_ELASTICITY: fem = true; break;
    case ADJ_EULER : euler = true; adj_euler = true; break;
    case ADJ_NAVIER_STOKES : ns = true; turbulent = (config->GetKind_Turb_Model() != NONE); adj_ns = true; break;
    case ADJ_RANS : ns = true; turbulent = true; adj_ns = true; adj_turb = (!config->GetFrozen_Visc_Cont()); break;
    case DISC_ADJ_EULER: euler = true; disc_adj = true; break;
    case DISC_ADJ_NAVIER_STOKES: ns = true; disc_adj = true; heat_fvm = config->GetWeakly_Coupled_Heat(); break;
    case DISC_ADJ_RANS: ns = true; turbulent = true; disc_adj = true; disc_adj_turb = (!config->GetFrozen_Visc_Disc()); heat_fvm = config->GetWeakly_Coupled_Heat(); break;
    case DISC_ADJ_FEM_EULER: fem_euler = true; disc_adj = true; break;
    case DISC_ADJ_FEM_NS: fem_ns = true; disc_adj = true; break;
    case DISC_ADJ_FEM_RANS: fem_ns = true; turbulent = true; disc_adj = true; disc_adj_turb = (!config->GetFrozen_Visc_Disc()); break;
    case DISC_ADJ_FEM: fem = true; disc_adj_fem = true; break;
    case DISC_ADJ_HEAT: heat_fvm = true; disc_adj_heat = true; break;

  }

  /*--- Determine the kind of FEM solver used for the flow. ---*/

  switch( config->GetKind_FEM_Flow() ) {
    case DG: fem_dg_flow = true; break;
  }

  /*--- Load restarts for any of the active solver containers. Note that
   these restart routines fill the fine grid and interpolate to all MG levels. ---*/

  if (restart || restart_flow) {
    if (euler || ns) {
      solver[val_iInst][MESH_0][FLOW_SOL]->LoadRestart(geometry[val_iInst], solver[val_iInst], config, val_iter, update_geo);
    }
    if (turbulent) {
      solver[val_iInst][MESH_0][TURB_SOL]->LoadRestart(geometry[val_iInst], solver[val_iInst], config, val_iter, update_geo);
    }
    if (fem) {
      if (dynamic) val_iter = SU2_TYPE::Int(config->GetDyn_RestartIter())-1;
      solver[val_iInst][MESH_0][FEA_SOL]->LoadRestart(geometry[val_iInst], solver[val_iInst], config, val_iter, update_geo);
    }
    if (fem_euler || fem_ns) {
      if (fem_dg_flow)
        solver[val_iInst][MESH_0][FLOW_SOL]->LoadRestart(geometry[val_iInst], solver[val_iInst], config, val_iter, update_geo);
    }
    if (heat_fvm) {
      solver[val_iInst][MESH_0][HEAT_SOL]->LoadRestart(geometry[val_iInst], solver[val_iInst], config, val_iter, update_geo);
    }
  }

  if (restart) {
    if (template_solver) {
      no_restart = true;
    }
    if (heat_fvm) {
      solver[val_iInst][MESH_0][HEAT_SOL]->LoadRestart(geometry[val_iInst], solver[val_iInst], config, val_iter, update_geo);
    }
    if (adj_euler || adj_ns) {
      solver[val_iInst][MESH_0][ADJFLOW_SOL]->LoadRestart(geometry[val_iInst], solver[val_iInst], config, val_iter, update_geo);
    }
    if (adj_turb) {
      no_restart = true;
    }
    if (disc_adj) {
      solver[val_iInst][MESH_0][ADJFLOW_SOL]->LoadRestart(geometry[val_iInst], solver[val_iInst], config, val_iter, update_geo);
      if (disc_adj_turb)
        solver[val_iInst][MESH_0][ADJTURB_SOL]->LoadRestart(geometry[val_iInst], solver[val_iInst], config, val_iter, update_geo);
      if (disc_adj_heat)
        solver[val_iInst][MESH_0][ADJHEAT_SOL]->LoadRestart(geometry[val_iInst], solver[val_iInst], config, val_iter, update_geo);
    }
    if (disc_adj_fem) {
        if (dynamic) val_iter = SU2_TYPE::Int(config->GetDyn_RestartIter())-1;
        solver[val_iInst][MESH_0][ADJFEA_SOL]->LoadRestart(geometry[val_iInst], solver[val_iInst], config, val_iter, update_geo);
    }
    if (disc_adj_heat) {
      solver[val_iInst][MESH_0][ADJHEAT_SOL]->LoadRestart(geometry[val_iInst], solver[val_iInst], config, val_iter, update_geo);
    }
  }

  /*--- Exit if a restart was requested for a solver that is not available. ---*/

  if (no_restart) {
    SU2_MPI::Error(string("A restart capability has not been implemented yet for this solver.\n") +
                   string("Please set RESTART_SOL= NO and try again."), CURRENT_FUNCTION);
  }

  /*--- Think about calls to pre / post-processing here, plus realizability checks. ---*/
  
}

void CErrorEstimationDriver::ComputeMetric() {

  CSolver *solver_flow = solver[ZONE_0][INST_0][MESH_0][FLOW_SOL],
          *solver_adj  = solver[ZONE_0][INST_0][MESH_0][ADJFLOW_SOL];

  if (rank == MASTER_NODE)
    cout << endl <<"----------------------------- Compute Metric ----------------------------" << endl;

  //--- 2D
  if(nDim == 2){
    //--- Volume flow grad
    if(rank == MASTER_NODE) cout << "Computing flow volume gradient via L2 Projection." << endl;
    solver_flow->SetGradient_L2Proj2(geometry[ZONE_0][INST_0][MESH_0], 
                                     config[ZONE_0]);

    //--- Volume flow Hess
    if(rank == MASTER_NODE) cout << "Computing flow volume Hessian via L2 Projection." << endl;
    solver_flow->SetHessian_L2Proj2(geometry[ZONE_0][INST_0][MESH_0], 
                                    config[ZONE_0]);

    //--- Volume adj grad
    if(rank == MASTER_NODE) cout << "Computing adjoint volume gradient via L2 Projection." << endl;
    solver_adj->SetGradient_L2Proj2(geometry[ZONE_0][INST_0][MESH_0], 
                                    config[ZONE_0]);

    //--- Metric
    if(rank == MASTER_NODE) cout << "Computing goal-oriented metric tensor." << endl;
    SumWeightedHessian2(solver_flow, solver_adj, geometry[ZONE_0][INST_0][MESH_0]);
  }

  //--- 3D
  else{
    //--- Volume flow grad
    if(rank == MASTER_NODE) cout << "Computing flow volume gradient via L2 Projection." << endl;
    solver_flow->SetGradient_L2Proj3(geometry[ZONE_0][INST_0][MESH_0], 
                                     config[ZONE_0]);

    //--- Volume flow Hess
    if(rank == MASTER_NODE) cout << "Computing flow volume Hessian via L2 Projection." << endl;
    solver_flow->SetHessian_L2Proj3(geometry[ZONE_0][INST_0][MESH_0], 
                                    config[ZONE_0]);

    //--- Volume adj grad
    if(rank == MASTER_NODE) cout << "Computing adjoint volume gradient via L2 Projection." << endl;
    solver_adj->SetGradient_L2Proj3(geometry[ZONE_0][INST_0][MESH_0], 
                                    config[ZONE_0]);

    //--- Metric
    if(rank == MASTER_NODE) cout << "Computing goal-oriented metric tensor." << endl;
    SumWeightedHessian3(solver_flow, solver_adj, geometry[ZONE_0][INST_0][MESH_0]);
  }
}

void CErrorEstimationDriver::SumWeightedHessian2(CSolver   *solver_flow,
                                                 CSolver   *solver_adj,
                                                 CGeometry *geometry) {

  unsigned long iPoint, 
                nPointDomain = geometry->GetnPointDomain();
  unsigned short nVarMetr = solver_flow->GetnVar(), 
                 nFluxMetr = 2;  //--- TODO: adjust size for goal (for different solvers, currently Euler) vs. feature
  unsigned short nMetr = 3*(nDim-1);

  for (iPoint = 0; iPoint < nPointDomain; ++iPoint) {

    //--- initialize metric tensor
    for(unsigned short im = 0; im < nMetr; ++im)
      solver_flow->node[iPoint]->SetAnisoMetr(im, 0.0);

    //--- perform summation of weighted Hessians
    for (unsigned short iVar = 0; iVar < nVarMetr; ++iVar) {

      for (unsigned short iFlux = 0; iFlux < nFluxMetr; ++iFlux) {
        const unsigned short ig = iVar*nDim + iFlux;
        const su2double grad = solver_adj->node[iPoint]->GetAnisoGrad(ig);

        for (unsigned short im = 0; im < nMetr; ++im) {
          const unsigned short ih = iFlux*nVarMetr*nMetr + iVar*nMetr + im;  
          const su2double hess = solver_flow->node[iPoint]->GetAnisoHess(ih);
          const su2double part = abs(grad)*hess;
          solver_flow->node[iPoint]->AddAnisoMetr(im,part);
        }
      }
    }
  }

  //--- constrain size and obtain global scaling
  su2double localScale = 0.0,
            globalScale = 0.0,
            p = 1.0, // For now, hardcode L1 metric
            hmax = config[ZONE_0]->GetMesh_Hmax(),
            hmin = config[ZONE_0]->GetMesh_Hmin(),
            outNPoint = su2double(config[ZONE_0]->GetMesh_Complexity());  // Constraint mesh complexity

  su2double **A      = new su2double*[nDim],
            **EigVec = new su2double*[nDim], 
            *EigVal  = new su2double[nDim];

  for(unsigned short iDim = 0; iDim < nDim; ++iDim){
    A[iDim]      = new su2double[nDim];
    EigVec[iDim] = new su2double[nDim];
  }

  for(iPoint = 0; iPoint < nPointDomain; ++iPoint) {
    CVariable *var = solver_flow->node[iPoint];

    const su2double a = var->GetAnisoMetr(0);
    const su2double b = var->GetAnisoMetr(1);
    const su2double c = var->GetAnisoMetr(2);
    
    A[0][0] = a; A[0][1] = b;
    A[1][0] = b; A[1][1] = c;

    CNumerics::EigenDecomposition(A, EigVec, EigVal, nDim);

    for(unsigned short iDim = 0; iDim < nDim; ++iDim) EigVal[iDim] = max(abs(EigVal[iDim]), 1.E-16);

    CNumerics::EigenRecomposition(A, EigVec, EigVal, nDim);

    var->SetAnisoMetr(0, A[0][0]);
    var->SetAnisoMetr(1, A[0][1]);
    var->SetAnisoMetr(2, A[1][1]);

    const su2double Vol = geometry->node[iPoint]->GetVolume();

    localScale += pow(abs(EigVal[0]*EigVal[1]),p/(2.*p+nDim))*Vol;
  }

  // for(iPoint = 0; iPoint < nPointDomain; ++iPoint) {
  //   CVariable *var = solver_flow->node[iPoint];

  //   const su2double a = var->GetAnisoMetr(0);
  //   const su2double b = var->GetAnisoMetr(1);
  //   const su2double c = var->GetAnisoMetr(2);
  //   const su2double d = pow(a-c,2.) + 4.*b*b;

  //   const su2double Lam1 = (a+c+sqrt(d))/2.;
  //   const su2double Lam2 = (a+c-sqrt(d))/2.;

  //   su2double RuH[2][2] = {{b, b},
  //                          {Lam1-a, Lam2-a}};

  //   if(abs(b) < 1.0e-16){
  //     RuH[0][0] = 1.0; RuH[0][1] = 0.0;
  //     RuH[1][0] = 0.0; RuH[1][1] = 1.0;
  //   }

  //   const su2double RuU[2][2]    = {{RuH[0][0]/sqrt(RuH[0][0]*RuH[0][0]+RuH[1][0]*RuH[1][0]), RuH[0][1]/sqrt(RuH[0][1]*RuH[0][1]+RuH[1][1]*RuH[1][1])},
  //                                   {RuH[1][0]/sqrt(RuH[0][0]*RuH[0][0]+RuH[1][0]*RuH[1][0]), RuH[1][1]/sqrt(RuH[0][1]*RuH[0][1]+RuH[1][1]*RuH[1][1])}};

  //   // const su2double Lam1new = min(max(abs(Lam1), 1./(hmax*hmax)), 1./(hmin*hmin));
  //   // const su2double Lam2new = min(max(abs(Lam2), 1./(hmax*hmax)), 1./(hmin*hmin));
  //   const su2double Lam1new = max(abs(Lam1), 1.E-16);
  //   const su2double Lam2new = max(abs(Lam2), 1.E-16);

  //   const su2double LamRuU[2][2] = {{abs(Lam1new)*RuU[0][0],abs(Lam1new)*RuU[1][0]},
  //                                   {abs(Lam2new)*RuU[0][1],abs(Lam2new)*RuU[1][1]}};

  //   const su2double MetrNew[3]   = {RuU[0][0]*LamRuU[0][0]+RuU[0][1]*LamRuU[1][0], 
  //                                   RuU[0][0]*LamRuU[0][1]+RuU[0][1]*LamRuU[1][1], 
  //                                   RuU[1][0]*LamRuU[0][1]+RuU[1][1]*LamRuU[1][1]};

  //   var->SetAnisoMetr(0, MetrNew[0]);
  //   var->SetAnisoMetr(1, MetrNew[1]);
  //   var->SetAnisoMetr(2, MetrNew[2]);

  //   const su2double Vol = geometry->node[iPoint]->GetVolume();

  //   localScale += pow(abs(Lam1*Lam2),p/(2.*p+nDim))*Vol;
  // }

#ifdef HAVE_MPI
  SU2_MPI::Allreduce(&localScale, &globalScale, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
#else
  globalScale = localScale;
#endif

  //--- normalize to achieve Lp metric for constraint complexity
  for (iPoint = 0; iPoint < nPointDomain; ++iPoint) {
    CVariable *var = solver_flow->node[iPoint];

    const su2double a = var->GetAnisoMetr(0);
    const su2double b = var->GetAnisoMetr(1);
    const su2double c = var->GetAnisoMetr(2);
    
    A[0][0] = a; A[0][1] = b;
    A[1][0] = b; A[1][1] = c;

    CNumerics::EigenDecomposition(A, EigVec, EigVal, nDim);

    // const su2double factor = pow(outNPoint/(1.54*globalScale), 2./nDim);
    const su2double factor = pow(outNPoint/(globalScale), 2./nDim) * pow(abs(EigVal[0]*EigVal[1]), -1./(2.*p+nDim));

    // const su2double Lam2new = min(max(abs(factor*Lam2), 1./(hmax*hmax)), 1./(hmin*hmin));
    EigVal[0] = abs(factor*EigVal[0]);
    EigVal[1] = abs(factor*EigVal[1]);
    su2double Lam1new = EigVal[0];
    su2double Lam2new = EigVAl[1];
    if(sqrt(Lam1new*Lam2new) < 1./(hmax*hmax)) {
      EigVal[0] = sqrt(Lam1new/Lam2new)/(hmax*hmax);
      EigVal[1] = sqrt(Lam2new/Lam1new)/(hmax*hmax);
    }
    else if(sqrt(Lam1new*Lam2new) > 1./(hmin*hmin)) {
      EigVal[0] = sqrt(Lam1new/Lam2new)/(hmin*hmin);
      EigVal[1] = sqrt(Lam2new/Lam1new)/(hmin*hmin);
    }

    CNumerics::EigenRecomposition(A, EigVec, EigVal, nDim);

    var->SetAnisoMetr(0, A[0][0]);
    var->SetAnisoMetr(1, A[0][1]);
    var->SetAnisoMetr(2, A[1][1]);
  }

  // //--- normalize to achieve Lp metric for constraint complexity
  // for (iPoint = 0; iPoint < nPointDomain; ++iPoint) {
  //   CVariable *var = solver_flow->node[iPoint];

  //   const su2double a = var->GetAnisoMetr(0);
  //   const su2double b = var->GetAnisoMetr(1);
  //   const su2double c = var->GetAnisoMetr(2);
  //   const su2double d = pow(a-c,2.) + 4.*b*b;

  //   su2double Lam1 = (a+c+sqrt(d))/2.;
  //   su2double Lam2 = (a+c-sqrt(d))/2.;

  //   // const su2double factor = pow(outNPoint/(1.54*globalScale), 2./nDim);
  //   const su2double factor = pow(outNPoint/(globalScale), 2./nDim) * pow(abs(Lam1*Lam2), -1./(2.*p+nDim));

  //   su2double RuH[2][2] = {{b, b},
  //                          {Lam1-a, Lam2-a}};

  //   if(abs(b) < 1.0e-16){
  //     RuH[0][0] = 1.0; RuH[0][1] = 0.0;
  //     RuH[1][0] = 0.0; RuH[1][1] = 1.0;
  //   }

  //   const su2double RuU[2][2]    = {{RuH[0][0]/sqrt(RuH[0][0]*RuH[0][0]+RuH[1][0]*RuH[1][0]), RuH[0][1]/sqrt(RuH[0][1]*RuH[0][1]+RuH[1][1]*RuH[1][1])},
  //                                   {RuH[1][0]/sqrt(RuH[0][0]*RuH[0][0]+RuH[1][0]*RuH[1][0]), RuH[1][1]/sqrt(RuH[0][1]*RuH[0][1]+RuH[1][1]*RuH[1][1])}};

  //   // const su2double Lam1new = factor*Lam1;
  //   // const su2double Lam2new = factor*Lam2;
  //   // const su2double Lam1new = min(max(abs(factor*Lam1), 1./(hmax*hmax)), 1./(hmin*hmin));
  //   // const su2double Lam2new = min(max(abs(factor*Lam2), 1./(hmax*hmax)), 1./(hmin*hmin));
  //   Lam1 = abs(factor*Lam1);
  //   Lam2 = abs(factor*Lam2);
  //   su2double Lam1new = Lam1;
  //   su2double Lam2new = Lam2;
  //   if(sqrt(Lam1new*Lam2new) < 1./(hmax*hmax)) {
  //     Lam1new = sqrt(Lam1/Lam2)/(hmax*hmax);
  //     Lam2new = sqrt(Lam2/Lam1)/(hmax*hmax);
  //   }
  //   else if(sqrt(Lam1new*Lam2new) > 1./(hmin*hmin)) {
  //     Lam1new = sqrt(Lam1/Lam2)/(hmin*hmin);
  //     Lam2new = sqrt(Lam2/Lam1)/(hmin*hmin);
  //   }

  //   const su2double LamRuU[2][2] = {{abs(Lam1new)*RuU[0][0],abs(Lam1new)*RuU[1][0]},
  //                                   {abs(Lam2new)*RuU[0][1],abs(Lam2new)*RuU[1][1]}};

  //   const su2double MetrNew[3]   = {RuU[0][0]*LamRuU[0][0]+RuU[0][1]*LamRuU[1][0], 
  //                                   RuU[0][0]*LamRuU[0][1]+RuU[0][1]*LamRuU[1][1], 
  //                                   RuU[1][0]*LamRuU[0][1]+RuU[1][1]*LamRuU[1][1]};

  //   var->SetAnisoMetr(0, MetrNew[0]);
  //   var->SetAnisoMetr(1, MetrNew[1]);
  //   var->SetAnisoMetr(2, MetrNew[2]);
  // }

  //--- compute min, max, total complexity
  su2double localMinDensity = 1.E9, localMaxDensity = 0., localTotComplex = 0.;
  su2double globalMinDensity = 1.E9, globalMaxDensity = 0., globalTotComplex = 0.;
  for (iPoint = 0; iPoint < nPointDomain; ++iPoint) {
    CVariable *var = solver_flow->node[iPoint];

    const su2double a = var->GetAnisoMetr(0);
    const su2double b = var->GetAnisoMetr(1);
    const su2double c = var->GetAnisoMetr(2);
    
    A[0][0] = a; A[0][1] = b;
    A[1][0] = b; A[1][1] = c;

    CNumerics::EigenDecomposition(A, EigVec, EigVal, nDim);

    const su2double Vol = geometry->node[iPoint]->GetVolume();
    const su2double density = sqrt(abs(EigVal[0]*EigVal[1]));

    localMinDensity = min(localMinDensity, density);
    localMaxDensity = max(localMaxDensity, density);
    localTotComplex += density*Vol;
  }

  // //--- compute min, max, total complexity
  // su2double localMinDensity = 1.E9, localMaxDensity = 0., localTotComplex = 0.;
  // su2double globalMinDensity = 1.E9, globalMaxDensity = 0., globalTotComplex = 0.;
  // for (iPoint = 0; iPoint < nPointDomain; ++iPoint) {
  //   CVariable *var = solver_flow->node[iPoint];

  //   const su2double a = var->GetAnisoMetr(0);
  //   const su2double b = var->GetAnisoMetr(1);
  //   const su2double c = var->GetAnisoMetr(2);
  //   const su2double d = pow(a-c,2.) + 4.*b*b;

  //   const su2double Lam1 = (a+c+sqrt(d))/2.;
  //   const su2double Lam2 = (a+c-sqrt(d))/2.;

  //   const su2double Vol = geometry->node[iPoint]->GetVolume();
  //   const su2double density = sqrt(abs(Lam1*Lam2));

  //   localMinDensity = min(localMinDensity, density);
  //   localMaxDensity = max(localMaxDensity, density);
  //   localTotComplex += density*Vol;
  // }

#ifdef HAVE_MPI
  SU2_MPI::Allreduce(&localMinDensity, &globalMinDensity, 1, MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(&localMaxDensity, &globalMaxDensity, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
  SU2_MPI::Allreduce(&localTotComplex, &globalTotComplex, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
#else
  globalMinDensity = localMinDensity;
  globalMaxDensity = localMaxDensity;
  globalTotComplex = localTotComplex;
#endif

  if(rank == MASTER_NODE) {
    cout << "Minimum density: " << globalMinDensity << "." << endl;
    cout << "Maximum density: " << globalMaxDensity << "." << endl;
    cout << "Mesh complexity: " << globalTotComplex << "." << endl;
  }
}

void CErrorEstimationDriver::SumWeightedHessian3(CSolver   *solver_flow,
                                                 CSolver   *solver_adj,
                                                 CGeometry *geometry) {

  unsigned long iPoint, nPointDomain = geometry->GetnPointDomain();
  unsigned short nVarMetr = solver_flow->GetnVar(), 
                 nFluxMetr = 3;  //--- TODO: adjust size for goal (for different solvers, currently Euler) vs. feature
  unsigned short nMetr = 3*(nDim-1);

  for (iPoint = 0; iPoint < nPointDomain; ++iPoint) {

    //--- initialize metric tensor
    for(unsigned short im = 0; im < nMetr; ++im)
      solver_flow->node[iPoint]->SetAnisoMetr(im, 0.0);

    //--- perform summation of weighted Hessians
    for (unsigned short iVar = 0; iVar < nVarMetr; ++iVar) {

      for (unsigned short iFlux = 0; iFlux < nFluxMetr; ++iFlux) {
        const unsigned short ig = iVar*nDim + iFlux;
        const su2double grad = solver_adj->node[iPoint]->GetAnisoGrad(ig);

        for (unsigned short im = 0; im < nMetr; ++im) {
          const unsigned short ih = iFlux*nVarMetr*nMetr + iVar*nMetr + im;  
          const su2double hess = solver_flow->node[iPoint]->GetAnisoHess(ih);
          const su2double part = abs(grad)*hess;
          solver_flow->node[iPoint]->AddAnisoMetr(im,part);
        }
      }
    }
  }

  //--- constrain size and obtain global scaling
  su2double localScale = 0.0,
            globalScale = 0.0,
            p = 1.0, // For now, hardcode L1 metric
            hmax = config[ZONE_0]->GetMesh_Hmax(),
            hmin = config[ZONE_0]->GetMesh_Hmin(),
            outNPoint = su2double(config[ZONE_0]->GetMesh_Complexity());  // Constraint mesh complexity

  su2double **A      = new su2double*[nDim],
            **EigVec = new su2double*[nDim], 
            *EigVal  = new su2double[nDim];

  for(unsigned short iDim = 0; iDim < nDim; ++iDim){
    A[iDim]      = new su2double[nDim];
    EigVec[iDim] = new su2double[nDim];
  }

  for(iPoint = 0; iPoint < nPointDomain; ++iPoint) {
    CVariable *var = solver_flow->node[iPoint];

    const su2double a = var->GetAnisoMetr(0);
    const su2double b = var->GetAnisoMetr(1);
    const su2double c = var->GetAnisoMetr(2);
    const su2double d = var->GetAnisoMetr(3);
    const su2double e = var->GetAnisoMetr(4);
    const su2double f = var->GetAnisoMetr(5);

    A[0][0] = a; A[0][1] = b; A[0][2] = c;
    A[1][0] = b; A[1][1] = d; A[1][2] = e;
    A[2][0] = c; A[2][1] = e; A[2][2] = f;

    CNumerics::EigenDecomposition(A, EigVec, EigVal, nDim);

    for(unsigned short iDim = 0; iDim < nDim; ++iDim) EigVal[iDim] = min(max(abs(EigVal[iDim]), 1./(hmax*hmax)), 1./(hmin*hmin));

    CNumerics::EigenRecomposition(A, EigVec, EigVal, nDim);

    var->SetAnisoMetr(0, A[0][0]);
    var->SetAnisoMetr(1, A[0][1]);
    var->SetAnisoMetr(2, A[0][2]);
    var->SetAnisoMetr(3, A[1][1]);
    var->SetAnisoMetr(4, A[1][2]);
    var->SetAnisoMetr(5, A[2][2]);

    const su2double Vol = geometry->node[iPoint]->GetVolume();

    localScale += pow(abs(EigVal[0]*EigVal[1]*EigVal[2]),p/(2.*p+nDim))*Vol;
  }

#ifdef HAVE_MPI
  SU2_MPI::Allreduce(&localScale, &globalScale, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
#else
  globalScale = localScale;
#endif

  //--- normalize to achieve Lp metric for constraint complexity
  for (iPoint = 0; iPoint < nPointDomain; ++iPoint) {
    CVariable *var = solver_flow->node[iPoint];

    const su2double a = var->GetAnisoMetr(0);
    const su2double b = var->GetAnisoMetr(1);
    const su2double c = var->GetAnisoMetr(2);
    const su2double d = var->GetAnisoMetr(3);
    const su2double e = var->GetAnisoMetr(4);
    const su2double f = var->GetAnisoMetr(5);

    // const su2double factor = pow(outNPoint/(1.54*globalScale), 2./nDim);
    const su2double factor = pow(outNPoint/(globalScale), 2./nDim) * pow(abs(EigVal[0]*EigVal[1]*EigVal[2]), -1./(2.*p+nDim));

    A[0][0] = a; A[0][1] = b; A[0][2] = c;
    A[1][0] = b; A[1][1] = d; A[1][2] = e;
    A[2][0] = c; A[2][1] = e; A[2][2] = f;

    CNumerics::EigenDecomposition(A, EigVec, EigVal, nDim);

    for(unsigned short iDim = 0; iDim < nDim; ++iDim) EigVal[iDim] = factor*EigVal[iDim];

    CNumerics::EigenRecomposition(A, EigVec, EigVal, nDim);

    //--- store lower triangle to be consistent with AMG
    var->SetAnisoMetr(0, A[0][0]);
    var->SetAnisoMetr(1, A[1][0]);
    var->SetAnisoMetr(2, A[1][1]);
    var->SetAnisoMetr(3, A[2][0]);
    var->SetAnisoMetr(4, A[2][1]);
    var->SetAnisoMetr(5, A[2][2]);
  }
}

void CErrorEstimationDriver::Output() {

  unsigned long ExtIter = config[ZONE_0]->GetExtIter_OffSet();

  /*--- Set Kind_Solver to primal since adaptation requires Mach (or pressure) ---*/

  switch (config[ZONE_0]->GetKind_Solver()) {
    case DISC_ADJ_EULER: config[ZONE_0]->SetKind_Solver(EULER); break;
    case DISC_ADJ_NAVIER_STOKES: config[ZONE_0]->SetKind_Solver(NAVIER_STOKES); break;
    case DISC_ADJ_RANS: config[ZONE_0]->SetKind_Solver(RANS); break;
  }

  /*--- Set DiscreteAdjoint flag to false so we write to correct files ---*/

  config[ZONE_0]->SetDiscrete_Adjoint(false);

  if (rank == MASTER_NODE) cout << endl << "-------------------------- File Output Summary --------------------------";

    /*--- Execute the routine for writing restart, volume solution,
     surface solution, and surface comma-separated value files. ---*/

  output->SetResult_Files_Parallel(solver, geometry, config, ExtIter, nZone);

  if (rank == MASTER_NODE) cout << "-------------------------------------------------------------------------" << endl << endl;

  
}

void CErrorEstimationDriver::SetAdaptationData() {

  /*--- Set Kind_Solver to primal since adaptation requires Mach (or pressure) ---*/

  switch (config[ZONE_0]->GetKind_Solver()) {
    case DISC_ADJ_EULER: config[ZONE_0]->SetKind_Solver(EULER); break;
    case DISC_ADJ_NAVIER_STOKES: config[ZONE_0]->SetKind_Solver(NAVIER_STOKES); break;
    case DISC_ADJ_RANS: config[ZONE_0]->SetKind_Solver(RANS); break;
  }

  /*--- Set DiscreteAdjoint flag to false so we write to correct files ---*/

  config[ZONE_0]->SetDiscrete_Adjoint(false);

  if (rank == MASTER_NODE) cout << endl << "---------------------------- Sort Metric Data ---------------------------" << endl;

  /*--- Execute the routines for collecting the restart data. ---*/

  output->SetResult_Parallel(solver, geometry, config, 1);

  if (rank == MASTER_NODE) cout << "-------------------------------------------------------------------------" << endl;

}

vector<vector<passivedouble> > CErrorEstimationDriver::GetAdaptationData() {

  vector<vector<passivedouble> > Parallel_Data = output->GetResult_Parallel();

  return Parallel_Data;

}

void CErrorEstimationDriver::SetConnectivityData() {

  if (rank == MASTER_NODE) cout << endl << "------------------------- Sort Connectivity Data ------------------------" << endl;

  /*--- Execute the routines for collecting the restart data. ---*/

  output->SetConnectivity_Parallel(geometry, config, 1);

  if (rank == MASTER_NODE) cout << "-------------------------------------------------------------------------" << endl << endl;

}

vector<vector<unsigned long> > CErrorEstimationDriver::GetConnectivityEdg(unsigned short val_iZone, unsigned short val_iInst) {

  vector<vector<unsigned long> > Edg = output->GetConnEdg(config[val_iZone], geometry[val_iZone][val_iInst][MESH_0]);
  return Edg;
}

vector<vector<unsigned long> > CErrorEstimationDriver::GetConnectivityTri(unsigned short val_iZone, unsigned short val_iInst) {

  vector<vector<unsigned long> > Tri = output->GetConnTri(config[val_iZone], geometry[val_iZone][val_iInst][MESH_0]);
  return Tri;
}

vector<vector<unsigned long> > CErrorEstimationDriver::GetConnectivityTet(unsigned short val_iZone, unsigned short val_iInst) {

  vector<vector<unsigned long> > Tet = output->GetConnTet(config[val_iZone], geometry[val_iZone][val_iInst][MESH_0]);
  return Tet;
}

unsigned short CErrorEstimationDriver::GetnMarker_All() {

  unsigned short nMarker_All = config[ZONE_0]->GetnMarker_All();
  return nMarker_All;
}

string CErrorEstimationDriver::GetMarker_All_TagBound(unsigned short val_iMarker) {

  string Marker_Tag = config[ZONE_0]->GetMarker_All_TagBound(val_iMarker);
  return Marker_Tag;
}

void CErrorEstimationDriver::CleanAdaptationData() {

  output->CleanResult_Parallel();
}

void CErrorEstimationDriver::CleanConnectivityData() {

  output->CleanConnectivity_Parallel();
}

void CErrorEstimationDriver::Postprocessing() {

  if (rank == MASTER_NODE)
    cout << endl <<"------------------- Error Estimation Postprocessing ---------------------" << endl;

  for (iZone = 0; iZone < nZone; iZone++) {
    for (iInst = 0; iInst < nInst[iZone]; iInst++){

      Solver_Postprocessing(solver[iZone], geometry[iZone][iInst],
                            config[iZone], iInst);
    } // End of loop over iInst
  }
  if (rank == MASTER_NODE) cout << "Deleted CSolver containers." << endl;

  for (iZone = 0; iZone < nZone; iZone++) {
    if (geometry[iZone] != NULL) {
      for (iInst = 0; iInst < nInst[iZone]; iInst++){
        for (unsigned short iMGlevel = 0; iMGlevel < config[iZone]->GetnMGLevels()+1; iMGlevel++) {
          if (geometry[iZone][iInst][iMGlevel] != NULL) delete geometry[iZone][iInst][iMGlevel];
        }
        if (geometry[iZone][iInst] != NULL) delete [] geometry[iZone][iInst];
      } // End of loop over iInst
      delete [] geometry[iZone];
    }
  }
  delete [] geometry;
  if (rank == MASTER_NODE) cout << "Deleted CGeometry containers." << endl;

  /*--- Deallocate config container ---*/
  if (config!= NULL) {
    for (iZone = 0; iZone < nZone; iZone++) {
      if (config[iZone] != NULL) {
        delete config[iZone];
      }
    }
    delete [] config;
  }
  if (rank == MASTER_NODE) cout << "Deleted CConfig containers." << endl;

}

void CErrorEstimationDriver::Solver_Postprocessing(CSolver ****solver, CGeometry **geometry,
                                                   CConfig *config, unsigned short val_iInst) {
  
  unsigned short iMGlevel;
  bool euler, ns, turbulent,
  adj_euler, adj_ns, adj_turb,
  heat_fvm, fem,
  spalart_allmaras, neg_spalart_allmaras, menter_sst, transition,
  template_solver, disc_adj, disc_adj_turb, disc_adj_fem, disc_adj_heat,
  e_spalart_allmaras, comp_spalart_allmaras, e_comp_spalart_allmaras;

  /*--- Initialize some useful booleans ---*/
  
  euler            = false;  ns              = false;  turbulent = false;
  adj_euler        = false;  adj_ns          = false;  adj_turb  = false;
  spalart_allmaras = false;  menter_sst      = false;  disc_adj_turb = false;
  neg_spalart_allmaras = false;
  disc_adj        = false;
  fem              = false;  disc_adj_fem    = false;
  heat_fvm        = false;   disc_adj_heat   = false;
  transition       = false;
  template_solver  = false;
  e_spalart_allmaras = false; comp_spalart_allmaras = false; e_comp_spalart_allmaras = false;

  /*--- Assign booleans ---*/
  
  switch (config->GetKind_Solver()) {
    case TEMPLATE_SOLVER: template_solver = true; break;
    case EULER : euler = true; break;
    case NAVIER_STOKES: ns = true; heat_fvm = config->GetWeakly_Coupled_Heat(); break;
    case RANS : ns = true; turbulent = true; if (config->GetKind_Trans_Model() == LM) transition = true; heat_fvm = config->GetWeakly_Coupled_Heat(); break;
    case FEM_EULER : euler = true; break;
    case FEM_NAVIER_STOKES:
    case FEM_LES: ns = true; break;
    case FEM_RANS: ns = true; turbulent = true; if (config->GetKind_Trans_Model() == LM) transition = true; break;
    case HEAT_EQUATION_FVM: heat_fvm = true; break;
    case FEM_ELASTICITY: fem = true; break;
    case ADJ_EULER : euler = true; adj_euler = true; break;
    case ADJ_NAVIER_STOKES : ns = true; turbulent = (config->GetKind_Turb_Model() != NONE); adj_ns = true; break;
    case ADJ_RANS : ns = true; turbulent = true; adj_ns = true; adj_turb = (!config->GetFrozen_Visc_Cont()); break;
    case DISC_ADJ_EULER: euler = true; disc_adj = true; break;
    case DISC_ADJ_NAVIER_STOKES: ns = true; disc_adj = true; heat_fvm = config->GetWeakly_Coupled_Heat(); break;
    case DISC_ADJ_RANS: ns = true; turbulent = true; disc_adj = true; disc_adj_turb = (!config->GetFrozen_Visc_Disc()); heat_fvm = config->GetWeakly_Coupled_Heat(); break;
    case DISC_ADJ_FEM_EULER: euler = true; disc_adj = true; break;
    case DISC_ADJ_FEM_NS: ns = true; disc_adj = true; break;
    case DISC_ADJ_FEM_RANS: ns = true; turbulent = true; disc_adj = true; disc_adj_turb = (!config->GetFrozen_Visc_Disc()); break;
    case DISC_ADJ_FEM: fem = true; disc_adj_fem = true; break;
    case DISC_ADJ_HEAT: heat_fvm = true; disc_adj_heat = true; break;
  }
  
  /*--- Assign turbulence model booleans ---*/
  
  if (turbulent)
    switch (config->GetKind_Turb_Model()) {
    case SA:     spalart_allmaras = true;     break;
    case SA_NEG: neg_spalart_allmaras = true; break;
    case SST:    menter_sst = true;           break;
    case SA_E: e_spalart_allmaras = true; break;
    case SA_COMP: comp_spalart_allmaras = true; break;
    case SA_E_COMP: e_comp_spalart_allmaras = true; break;
    }
  
  /*--- Definition of the Class for the solution: solver[DOMAIN][MESH_LEVEL][EQUATION]. Note that euler, ns
   and potential are incompatible, they use the same position in sol container ---*/
  
  for (iMGlevel = 0; iMGlevel <= config->GetnMGLevels(); iMGlevel++) {
    
    /*--- DeAllocate solution for a template problem ---*/
    
    if (template_solver) {
      delete solver[val_iInst][iMGlevel][TEMPLATE_SOL];
    }

    /*--- DeAllocate solution for adjoint problem ---*/
    
    if (adj_euler || adj_ns || disc_adj) {
      delete solver[val_iInst][iMGlevel][ADJFLOW_SOL];
      if (disc_adj_turb || adj_turb) {
        delete solver[val_iInst][iMGlevel][ADJTURB_SOL];
      }
      if (heat_fvm) {
        delete solver[val_iInst][iMGlevel][ADJHEAT_SOL];
      }
    }

    if (disc_adj_heat) {
      delete solver[val_iInst][iMGlevel][ADJHEAT_SOL];
    }

    /*--- DeAllocate solution for direct problem ---*/
    
    if (euler || ns) {
      delete solver[val_iInst][iMGlevel][FLOW_SOL];
    }

    if (turbulent) {
      if (spalart_allmaras || neg_spalart_allmaras || menter_sst || e_spalart_allmaras || comp_spalart_allmaras || e_comp_spalart_allmaras) {
        delete solver[val_iInst][iMGlevel][TURB_SOL];
      }
      if (transition) {
        delete solver[val_iInst][iMGlevel][TRANS_SOL];
      }
    }
    if (heat_fvm) {
      delete solver[val_iInst][iMGlevel][HEAT_SOL];
    }
    if (fem) {
      delete solver[val_iInst][iMGlevel][FEA_SOL];
    }
    if (disc_adj_fem) {
      delete solver[val_iInst][iMGlevel][ADJFEA_SOL];
    }
    
    delete [] solver[val_iInst][iMGlevel];
  }
  
  delete [] solver[val_iInst];

}

void CErrorEstimationDriver::Solver_Deletion(CSolver ****solver,
                                    CConfig *config, unsigned short val_iInst) {
  bool euler, ns, turbulent,
  adj_euler, adj_ns, adj_turb,
  heat_fvm, fem,
  spalart_allmaras, neg_spalart_allmaras, menter_sst, transition,
  template_solver, disc_adj, disc_adj_turb, disc_adj_fem, disc_adj_heat,
  e_spalart_allmaras, comp_spalart_allmaras, e_comp_spalart_allmaras;

  /*--- Initialize some useful booleans ---*/
  
  euler            = false;  ns              = false;  turbulent = false;
  adj_euler        = false;  adj_ns          = false;  adj_turb  = false;
  spalart_allmaras = false;  menter_sst      = false;  disc_adj_turb = false;
  neg_spalart_allmaras = false;
  disc_adj        = false;
  fem              = false;  disc_adj_fem    = false;
  heat_fvm        = false;   disc_adj_heat   = false;
  transition       = false;
  template_solver  = false;
  e_spalart_allmaras = false; comp_spalart_allmaras = false; e_comp_spalart_allmaras = false;

  /*--- Assign booleans ---*/
  
  switch (config->GetKind_Solver()) {
    case TEMPLATE_SOLVER: template_solver = true; break;
    case EULER : euler = true; break;
    case NAVIER_STOKES: ns = true; heat_fvm = config->GetWeakly_Coupled_Heat(); break;
    case RANS : ns = true; turbulent = true; if (config->GetKind_Trans_Model() == LM) transition = true; heat_fvm = config->GetWeakly_Coupled_Heat(); break;
    case FEM_EULER : euler = true; break;
    case FEM_NAVIER_STOKES:
    case FEM_LES: ns = true; break;
    case FEM_RANS: ns = true; turbulent = true; if (config->GetKind_Trans_Model() == LM) transition = true; break;
    case HEAT_EQUATION_FVM: heat_fvm = true; break;
    case FEM_ELASTICITY: fem = true; break;
    case ADJ_EULER : euler = true; adj_euler = true; break;
    case ADJ_NAVIER_STOKES : ns = true; turbulent = (config->GetKind_Turb_Model() != NONE); adj_ns = true; break;
    case ADJ_RANS : ns = true; turbulent = true; adj_ns = true; adj_turb = (!config->GetFrozen_Visc_Cont()); break;
    case DISC_ADJ_EULER: euler = true; disc_adj = true; break;
    case DISC_ADJ_NAVIER_STOKES: ns = true; disc_adj = true; heat_fvm = config->GetWeakly_Coupled_Heat(); break;
    case DISC_ADJ_RANS: ns = true; turbulent = true; disc_adj = true; disc_adj_turb = (!config->GetFrozen_Visc_Disc()); heat_fvm = config->GetWeakly_Coupled_Heat(); break;
    case DISC_ADJ_FEM_EULER: euler = true; disc_adj = true; break;
    case DISC_ADJ_FEM_NS: ns = true; disc_adj = true; break;
    case DISC_ADJ_FEM_RANS: ns = true; turbulent = true; disc_adj = true; disc_adj_turb = (!config->GetFrozen_Visc_Disc()); break;
    case DISC_ADJ_FEM: fem = true; disc_adj_fem = true; break;
    case DISC_ADJ_HEAT: heat_fvm = true; disc_adj_heat = true; break;
  }
  
  /*--- Assign turbulence model booleans ---*/
  
  if (turbulent){
    switch (config->GetKind_Turb_Model()) {
    case SA:     spalart_allmaras = true;     break;
    case SA_NEG: neg_spalart_allmaras = true; break;
    case SST:    menter_sst = true;           break;
    case SA_E: e_spalart_allmaras = true; break;
    case SA_COMP: comp_spalart_allmaras = true; break;
    case SA_E_COMP: e_comp_spalart_allmaras = true; break;
    }
  }
  
  /*--- Definition of the Class for the solution: solver[DOMAIN][MESH_LEVEL][EQUATION]. Note that euler, ns
   and potential are incompatible, they use the same position in sol container ---*/
  
    
    /*--- DeAllocate solution for a template problem ---*/
    
  if (template_solver) {
    delete solver[val_iInst][MESH_0][TEMPLATE_SOL];
  }

    /*--- DeAllocate solution for adjoint problem ---*/

  if (adj_euler || adj_ns || disc_adj) {
    delete solver[val_iInst][MESH_0][ADJFLOW_SOL];
    if (disc_adj_turb || adj_turb) {
      delete solver[val_iInst][MESH_0][ADJTURB_SOL];
    }
    if (heat_fvm) {
      delete solver[val_iInst][MESH_0][ADJHEAT_SOL];
    }
  }

  if (disc_adj_heat) {
    delete solver[val_iInst][MESH_0][ADJHEAT_SOL];
  }

    /*--- DeAllocate solution for direct problem ---*/

  if (euler || ns) {
    delete solver[val_iInst][MESH_0][FLOW_SOL];
  }

  if (turbulent) {
    if (spalart_allmaras || neg_spalart_allmaras || menter_sst || e_spalart_allmaras || comp_spalart_allmaras || e_comp_spalart_allmaras) {
      delete solver[val_iInst][MESH_0][TURB_SOL];
    }
    if (transition) {
      delete solver[val_iInst][MESH_0][TRANS_SOL];
    }
  }
  if (heat_fvm) {
    delete solver[val_iInst][MESH_0][HEAT_SOL];
  }
  if (fem) {
    delete solver[val_iInst][MESH_0][FEA_SOL];
  }
  if (disc_adj_fem) {
    delete solver[val_iInst][MESH_0][ADJFEA_SOL];
  }

  delete [] solver[val_iInst][MESH_0];

  delete [] solver[val_iInst];

}
