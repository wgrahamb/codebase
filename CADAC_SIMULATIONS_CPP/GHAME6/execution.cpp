///////////////////////////////////////////////////////////////////////////////
///////////////////////   CADAC++ Simulation GHAME6  ///////////////////////////
///////////////////////////////////////////////////////////////////////////////
//FILE: execution.cpp
//
//Initializing and executing the simulation
//
//020923 Created by Peter H Zipfel
//030415 Migrated to HYPER simulation, PZi
//130619 Made compatible with MS C++ 10, PZi
//131025 Made compatible with MS C++ V12, PZi
///////////////////////////////////////////////////////////////////////////////

#include "class_hierarchy.hpp"

///////////////////////////////////////////////////////////////////////////////
//////////////// Definition of global function prototypes used in main() //////
///////////////////////////////////////////////////////////////////////////////

//acquiring the simmulation title
void acquire_title_options(fstream &input,char *title,char *options,int &nmonte
						   ,int &iseed,int &nmc);

//acquiring the simulation run time
double acquire_endtime(fstream &input);

//numbering the modules
void number_modules(fstream &input,int &num);

//acquiring the calling order of the modules
void order_modules(fstream &input,int &num,Module *module_list);

//acquiring the number of vehicle objects
void number_objects(fstream &input,int &num_vehicles,int &num_hyper,int &num_satellite,int &num_radar);

//creating a type of vehicle object
Cadac *set_obj_type(fstream &input,Module *module_list,int num_modules,
				   int num_satellite,int num_radar);

//running the simulation
void execute(Vehicle &vehicle_list,Module *module_list,double sim_time,
			 double end_time,int num_vehicles,int num_modules,double plot_step,
			 double int_step,double scrn_step,double com_step,double traj_step,char *options,
			 ofstream &ftabout,ofstream *plot_ostream_list,Packet *combus,int *status,
			 int num_hyper,int num_satellite,int num_radar,ofstream &ftraj,char *title,bool traj_merge,
			 int nmonte,int nmc,ofstream *stat_ostream_list,bool *stati_write_term);

// saving status of 'combus' vehicle objects
void combus_status(Packet *combus,int *status,int num_vehicles);

//getting timimg cycles for plotting, screen output and integration
void acquire_timing(fstream &input,double &plot_step,double &scn_step,double &com_step,
					double &traj_step,double &int_step);

//merging the 'ploti.asc' files onto 'plot.asc' 
void merge_plot_files(string *plot_file_list,int num_hyper,char *title);

//merging the 'stati.asc' files onto 'stat.asc' 
void merge_stat_files(string *stat_file_list,int num_hyper,char *title);

//writing 'combus' data on screen
void comscrn_data(Packet *combus,int num_vehicles);

//writing banner on 'traj.asc' file 
//void traj_banner(ofstream &ftraj,Packet *combus,char *title,int num_vehicles);
void traj_banner(ofstream &ftraj,Packet *combus,char *title,int num_vehicles,
				 int nhyper,int nsatellite,int nradar);

//writing 'traj.asc' file of 'combus' data
void traj_data(ofstream &ftraj,Packet *combus,int num_vehicles,bool merge);

//Documenting 'input.asc' with module-variable definitions
void document_input(Document *doc_hyper6,Document *doc_satellite3,Document *doc_radar0);


///////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////  main()   //////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//Main function
//
//011128 Created by Peter H Zipfel
//030415 Adopted for HYPER simulation, PZi
//130619 Made compatible with MS C++ 10 by keeping Console window open, PZi
///////////////////////////////////////////////////////////////////////////////

int main() 
{
	double sim_time(0); //simulation time, same as 'time'
	char title[CHARL]; //title from first line of 'input.asc'
	char options[CHARL]; //options from 'input.asc' for output
	Module *module_list=NULL; //list of modules and sequence as per 'input.asc'
	int num_modules; //number of modules
	double plot_step; //writing time step to 'plot.asc' and 'ploti.asc', i=1,2,3...
	double scrn_step; //writing time step to screen (console)
	double int_step; //integration step size (constant throughout simulation)
	double com_step; //writing time step of 'combus' data to screen
	double traj_step; //writing time step of 'combus' to file 'traj.asc'
	int num_vehicles; //total number of vehicle objects
	int num_hyper; //number of hyper objects
	int num_satellite; //number of satellite objects
	int num_radar; //number of radar objects
	Cadac *vehicle_type=NULL; //array of vehicle object pointers
	char vehicle_name[CHARN]; //name of each vehicle type
	double end_time; //run termination time from 'input.asc'
	string *plot_file_list=NULL; //array containing file names of 'ploti.asc', i=1,2,3...
	ofstream *plot_ostream_list=NULL; //array of output streams for 'ploti.asc', i=1,2,3...
	string *stat_file_list=NULL; //array containing file names of 'stati.asc', i=1,2,3...
	ofstream *stat_ostream_list=NULL; //array of output streams for 'stati.asc', i=1,2,3...
	Packet *combus=NULL; //communication bus container storing data for each vehicle object
					//in same sequence as 'vehicle_list'
	int *status=NULL; //array containing status of each vehicle object
	int nmonte=0; //number of MC runs to be executed
	int nmc=0; //MC counter
	int iseed; //seeding srand()
	bool one_traj_banner=true; //write just one banner on file 'traj.asc'
	bool *stati_write_term=NULL; //flag for writing impact data on 'stati.asc' once
	Document *doc_hyper6=NULL;  //array for documenting HYPER6 module-variables of 'input.asc'
	Document *doc_satellite3=NULL;  //array for documenting SAT3 module-variables of 'input.asc'
	Document *doc_radar0=NULL;  //array for documenting RADAR0 module-variables of 'input.asc'
	bool document_hyper6=false; //true if array doc_hyper6 was created
	bool document_satellite3=false; //true if doc_satellite3 was created
	bool document_radar0=false; //true if doc_radar0 was created

	///////////////////////////////////////////////////////////////////////////
	/////////////// Opening of files and creation of stream objects  //////////
	///////////////////////////////////////////////////////////////////////////

	//creating an input stream object and opening 'input.asc' file
	fstream input("input.asc");
	if(input.fail())
	{cerr<<"*** Error: File stream 'input.asc' failed to open (check spelling) ***\n";system("pause");exit(1);}

	//creating an output stream object and opening 'tabout.asc' file
	ofstream ftabout("tabout.asc");
	if(!ftabout){cout<<" *** Error: cannot open 'tabout.asc' file *** \n";system("pause");exit(1);}

	//creating an output stream object and opening 'doc.asc' file
	ofstream fdoc("doc.asc");
	if(!fdoc){cout<<" *** Error: cannot open 'doc.asc' file *** \n";system("pause");exit(1);}

	//creating an output stream object and opening 'traj.asc' file
	ofstream ftraj("traj.asc");
	if(!ftraj){cout<<" *** Error: cannot open 'traj.asc' file *** \n";system("pause");exit(1);}

	//creating file 'input_copy.asc' in local directory for use in 'document_input()'
	ofstream fcopy("input_copy.asc");
	if(!fcopy){cout<<" *** Error: cannot open 'input_copy.asc' file *** \n";system("pause");exit(1);}


	///////////////////////////////////////////////////////////////////////////
	////////////////////////// Monte Carlo Loop ///////////////////////////////
	///////////////////////////////////////////////////////////////////////////
	do
	{
		//initializing flags
		bool one_screen_banner=true; //write just one banner on screen
		bool document_hyper=true; //write  to 'doc.asc' hyper module-variables only once
		bool document_satellite=true; //write  to 'doc.asc' satellite module-variables only once
		bool document_radar=true; //write  to 'doc.asc' radar module-variables only once
		bool traj_merge=false; //flag used in writing 'time=-1' endblock on 'traj.asc'
		
		//aqcuiring title statement and option selections
		acquire_title_options(input,title,options,nmonte,iseed,nmc);

		//initializing random number generator
		if(!nmc) srand(iseed); 

		//acquiring number of module 
		number_modules(input,num_modules);

		//dynamic memory allocation of array 'module_list'
		module_list=new Module[num_modules]; 
		if(module_list==0){cerr<<"*** Error: module_list[] allocation failed *** \n";system("pause");exit(1);}
					
		//acquiring calling order of module 
		order_modules(input,num_modules,module_list);
		
		//acquiring the time stepping
		acquire_timing(input,plot_step,scrn_step,int_step,com_step,traj_step);

		//acquiring number of vehicle objects from 'input.asc'
		number_objects(input,num_vehicles,num_hyper,num_satellite,num_radar);

		//creating the 'vehicle_list' object
		// at this point the constructor 'Vehicle' is called and memory is allocated
		Vehicle vehicle_list(num_vehicles);
		
		//allocating memory for 'ploti.asc' file streams, but do it only once
		if(!nmc) plot_ostream_list=new ofstream[num_vehicles];
		if(plot_ostream_list==0){cerr<<"*** Error: plot_ostream_list[] alloc. failed *** \n";system("pause");exit(1);}

		//allocating memory for 'ploti.asc' files, but do it only once
		if(!nmc) plot_file_list=new string[num_vehicles];
		if(plot_file_list==0){cerr<<"*** Error: plot_file_list[] alloc. failed *** \n";system("pause");exit(1);}

		//allocating memory for 'stati.asc' file streams, but do it only once
		if(!nmc) stat_ostream_list=new ofstream[num_vehicles];
		if(stat_ostream_list==0){cerr<<"*** Error: stat_ostream_list[] alloc. failed *** \n";system("pause");exit(1);}

		//allocating memory for 'stati.asc' files, but do it only once
		if(!nmc) stat_file_list=new string[num_vehicles];
		if(stat_file_list==0){cerr<<"*** Error: stat_file_list[] alloc. failed *** \n";system("pause");exit(1);}

		//allocating memory for 'combus'
		combus=new Packet[num_vehicles];
		if(combus==0){cerr<<"*** Error: combus[] allocation failed *** \n";system("pause");exit(1);}

		//allocating memory for 'status'
		status=new int[num_vehicles];
		if(status==0){cerr<<"*** Error: status[] allocation failed *** \n";system("pause");exit(1);}

		//initialize 'status' to 'alive=1'
		for(int ii=0;ii<num_vehicles;ii++) status[ii]=1;

		//allocating memory for write-flag to 'stati.asc'
		stati_write_term=new bool[num_vehicles];
		if(stati_write_term==0){cerr<<"*** Error: stati_write_term[] allocation failed *** \n";system("pause");exit(1);}

		//initialize 'stati_write_term' to true
		for( int jj=0;jj<num_vehicles;jj++) stati_write_term[jj]=true;

		///////////////////////////////////////////////////////////////////////
		////////////////// Initializing of each vehicle object  ///////////////
		///////////////////////////////////////////////////////////////////////

		for(int i=0;i<num_vehicles;i++)
		{
			//Loading pointers of the vehicle-object classes ('Hyper','Satellite'.'Radar') into 
			// the i-th location of 'vehicle_list'. Vehicle type is read in from 'input.asc' file. 
			//The loading process allocates dynamic memory at the pointer location
			// as required by the vehicle object 
			//The function returns the 'vehicle_type' as specified in 'input.asc' 
			//Furthermore, it passes 'module_list', 'num_modules','num_satellite'and'num_radar'
			// to the 'Hyper', 'Satellite' and 'Radar' constructors
			vehicle_type=set_obj_type(input,module_list,num_modules,num_satellite,num_radar);
 				
			//add vehicle to 'vehicle_list'
			vehicle_list.add_vehicle(*vehicle_type);

			//getting the name of the type of vehicle
			strcpy(vehicle_name,vehicle_list[i]->get_vname());

			//vehicle data and tables read from 'input.asc' 
			vehicle_list[i]->vehicle_data(input,nmonte);

			//executing initialization computations -MOD: insert here new module initialization function		
			for (int j=0;j<num_modules;j++)
			{
				if((module_list[j].name=="newton")&&(module_list[j].initialization=="init"))
					vehicle_list[i]->init_newton();
				else if((module_list[j].name=="kinematics")&&(module_list[j].initialization=="init"))
					vehicle_list[i]->init_kinematics(sim_time,int_step);
				else if((module_list[j].name=="euler")&&(module_list[j].initialization=="init"))
					vehicle_list[i]->init_euler();
				else if((module_list[j].name=="environment")&&(module_list[j].initialization=="init"))
					vehicle_list[i]->init_environment();
				else if((module_list[j].name=="aerodynamics")&&(module_list[j].initialization=="init"))
					vehicle_list[i]->init_aerodynamics();
				else if((module_list[j].name=="propulsion")&&(module_list[j].initialization=="init"))
					vehicle_list[i]->init_propulsion();
				else if((module_list[j].name=="ins")&&(module_list[j].initialization=="init"))
					vehicle_list[i]->init_ins();
			}
			//writing banner to screen and file 'tabout.asc'
			if(!strcmp(vehicle_name,"HYPER6")&&one_screen_banner)
			{
				one_screen_banner=false;
				if(strstr(options,"y_scrn"))
				{
					vehicle_list[i]->scrn_banner();
				}
				if(strstr(options,"y_tabout"))
				{	
					vehicle_list[i]->tabout_banner(ftabout,title,nmonte,nmc);
				}
			}
			//writing one block of data to screen after module initialization
			if(strstr(options,"y_scrn"))
			{
				vehicle_list[i]->scrn_data();
			}
			if(strstr(options,"y_tabout"))
			{	
				vehicle_list[i]->tabout_data(ftabout);
			}

			//executing 'plot' and 'stat' file initialization and documentation, but only at first run
			if(!nmc)
			{		
				//creating output stream objects in 'plot_ostream_list[i]' for every file "ploti.asc"
				//currently only the HYPER6 plot files contain any data
				if(strstr(options,"y_plot"))
				{
					if(!strcmp(vehicle_name,"HYPER6"))
					{
						char index[CHARN]; //
						string plotiasc; //plot file name
						const char *name;

						//building names for plot files
						sprintf(index,"%i",i+1);
						plotiasc="plot"+string(index)+".asc"; //using Standard Library string constructor
						plot_file_list[i]=plotiasc;
						name=plotiasc.c_str(); //using string member function to convert to char array 

						//creating output stream list 'plot_ostream_list[i]', each will write on file 'ploti.asc'
						plot_ostream_list[i].open(name); //'name' must be 'char' type

						//writing banner on 'ploti.asc'
						vehicle_list[i]->plot_banner(plot_ostream_list[i],title);				}
				}

				//creating output stream objects in 'stat_ostream_list[i]' for every file "stati.asc"
				//currently only the HYPER6 stat files contain any data
				if(strstr(options,"y_stat"))
				{
					if(!strcmp(vehicle_name,"HYPER6"))
					{
						char index[CHARN]; //
						string statiasc; //stat file name
						const char *name;

						//building names for stat files
						sprintf(index,"%i",i+1);
						statiasc="stat"+string(index)+".asc"; //using Standard Library string constructor
						stat_file_list[i]=statiasc;
						name=statiasc.c_str(); //using string member function to convert to char array 

						//creating output stream list 'stat_ostream_list[i]', each will write on file 'stati.asc'
						stat_ostream_list[i].open(name); //'name' must be 'char' type

						//writing banner on 'stati.asc'
						vehicle_list[i]->plot_banner(stat_ostream_list[i],title);				}
				}

				//composing documentation of 'round6', 'hyper', 'satellite' and 'radar' module-variables
				//but do it only once for HYPER6, SAT3 and RADAR0 objects
				if(strstr(options,"y_doc"))
				{
					if(!strcmp(vehicle_name,"HYPER6")&&document_hyper)
					{
						document_hyper=false;
						int size=NROUND6+NHYPER;
						doc_hyper6=new Document[size];
						if(doc_hyper6==0){cerr<<"*** Error: doc_hyper6[] allocation failed *** \n";system("pause");exit(1);}
						vehicle_list[i]->document(fdoc,title,doc_hyper6);
						document_hyper6=true;
					}
					if(!strcmp(vehicle_name,"SAT3")&&document_satellite)
					{
						document_satellite=false;
						int size=NROUND3+NSAT;
						doc_satellite3=new Document[size];
						if(doc_satellite3==0){cerr<<"*** Error: doc_satellite3[] allocation failed *** \n";system("pause");exit(1);}
						vehicle_list[i]->document(fdoc,title,doc_satellite3);
						document_satellite3=true;
					}
					if(!strcmp(vehicle_name,"RADAR0")&&document_radar)
					{
						document_radar=false;
						int size=NGROUND0+NRADAR;
						doc_radar0=new Document[size];
						if(doc_radar0==0){cerr<<"*** Error: doc_radar0[] allocation failed *** \n";system("pause");exit(1);}
						vehicle_list[i]->document(fdoc,title,doc_radar0);
						document_radar0=true;
					}
				}
			}//end of initializing plot, stat files and documentation

			//loading packets into 'combus' communication bus
			combus[i]=vehicle_list[i]->loading_packet_init(num_hyper,num_satellite,num_radar);

		} //end of initialization of vehicle object loop

		//writing 'combus' data to screen at time=0, after module initialization			
		if(strstr(options,"y_comscrn"))
		{
			comscrn_data(combus,num_vehicles);
		}
		//writing banner for 'traj.asc' output file
		if(strstr(options,"y_traj")&&one_traj_banner)
		{
			one_traj_banner=false;
			traj_banner(ftraj,combus,title,num_vehicles,num_hyper,num_satellite,num_radar);

			//writing data after 'initial module' calculations
			traj_data(ftraj,combus,num_vehicles,traj_merge);
		}

		//Acuire ending time (last entry on 'input.asc')
		// and put file pointer at the beginning of 'input.asc' for MC repeat runs
		end_time=acquire_endtime(input);

		if(!nmc&&strstr(options,"y_doc")){
			//documenting input.asc (only once)
			document_input(doc_hyper6,doc_satellite3,doc_radar0);
			if(document_hyper6)delete [] doc_hyper6;
			if(document_satellite3)delete [] doc_satellite3;
			if(document_radar0)delete [] doc_radar0;
		}

		///////////////////////////////////////////////////////////////////////	
		/////////////////////// Simulation Execution //////////////////////////
		///////////////////////////////////////////////////////////////////////	

		execute(vehicle_list,module_list,sim_time,
				 end_time,num_vehicles,num_modules,plot_step,
				 int_step,scrn_step,com_step,traj_step,options,ftabout,
				 plot_ostream_list,combus,status,num_hyper,num_satellite,num_radar,ftraj,title,
				 traj_merge,nmonte,nmc,stat_ostream_list,stati_write_term);

		//Deallocate dynamic memory
		delete [] module_list;
		delete [] combus;
		delete [] status;
		delete [] stati_write_term;

		// If MONTE > 0 repeat 'nmonte' times
		nmc++;
	}	// at this point the destructor of the object 'Vehicle vehicle_list' is called 
	while(nmc<nmonte); 

	///////////////////////////////////////////////////////////////////////////	
	///////////////////////// End of Monte Carlo Loop /////////////////////////
	///////////////////////////////////////////////////////////////////////////	
	//////////////////////////// Post-Processing //////////////////////////////
	///////////////////////////////////////////////////////////////////////////	

	//Close input file streams
	input.close();
	fcopy.close();
	int f(0);

	//Close file streams
	ftabout.close();
	for(f=0;f<num_vehicles;f++) plot_ostream_list[f].close();
	fdoc.close();
	for(f=0;f<num_vehicles;f++) stat_ostream_list[f].close();
	ftraj.close();

	//merging 'ploti.asc' files into 'plot.asc'
	if(strstr(options,"y_merge")&&strstr(options,"y_plot"))
	{
		merge_plot_files(plot_file_list,num_hyper,title);
	}
	//merging 'stati.asc' files into 'stat.asc'm using 'merge_plot_data' function
	//adding at the end: time=-1 and dummy block of data
	if(strstr(options,"y_merge")&&strstr(options,"y_stat"))
	{
		merge_stat_files(stat_file_list,num_hyper,title);
	}
	//Deallocate dynamic memory
	delete [] plot_ostream_list;
	delete [] plot_file_list;
	delete [] stat_ostream_list;
	delete [] stat_file_list;

	system("pause");
	return 0;
}
///////////////////////////////////////////////////////////////////////////////	
////////////////////////// End of main ////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////	

///////////////////////////////////////////////////////////////////////////////
//Executing the simulation
//
//Parameters:	&vehicle_list =	vehicle array - list of vehicle objects 
//								and their respective type (hyper, satellite or radar)
//								estabished by global function 'set_obj_type'
//				*module_list = module array - list of modules and their calling sequence
//								established by global function 'order_modules'
//				sim_time = simulation time; called 'time' in output		 
//				end_time = time to stop the simulation - read from 'input.asc' 
//								by global function 'acquire_end'
//				num_vehicles = total number of vehicles - read from 'input.asc' (VEHICLES #)
//								by global function 'number_vehicles'  				
//				num_modules = total number of modules - read from 'input.asc'
//								by global function 'number_modules'
//				plot_step = output writing interval to file 'traj.asc' - sec
//								read from 'input.asc' by global function 'acquire_timing'  				
//				int_step = integration step 
//								read from 'input.asc' by global function 'acquire_timing'  				
//				scrn_step = output writing interval to console - sec  				
//								read from 'input.asc' by global function 'acquire_timing'
//				com_step = output interval to communication bus 'combus'
//				traj_step = output interval to 'traj.asc'
//				*options = output option list
//				&ftabout = output file-stream to 'tabout.asc'
//				*plot_ostream_list = output file-steam list of 'ploti.asc' for each individual hyper 
//								hyper object
//				*combus = commumication bus
//				*status = health of vehicles
//				num_hyper = number of 'Hyper' objects
//				num_satellite = number of 'Satellite' objects
//				num_radar = number of 'Radar' objects
//				&ftraj = output file-stream to 'traj.asc'
//				*title = idenfication of run
//				traj_merge = flag for merging MC runs in 'traj.asc'
//				nmonte = max number of MC runs
//				nmc = current MC iteration number 
//				*stat_ostream_list = output file-steam list of 'stati.asc' for each individual hyper 
//								hyper object
//				*stati_write_term = flag for writing impact data on 'stati.asc' once
//				  				
//011128 Created by Peter H Zipfel
//030415 Adopted for HYPER simulation, PZi
//040315 Calculating event_time, PZi
///////////////////////////////////////////////////////////////////////////////
void execute(Vehicle &vehicle_list,Module *module_list,double sim_time,
			 double end_time,int num_vehicles,int num_modules,double plot_step,
			 double int_step,double scrn_step,double com_step,double traj_step,char *options,
			 ofstream &ftabout,ofstream *plot_ostream_list,Packet *combus,int *status,
			 int num_hyper,int num_satellite,int num_radar,ofstream &ftraj,char *title,bool traj_merge,
			 int nmonte,int nmc,ofstream *stat_ostream_list,bool *stati_write_term)
{
	double scrn_time(0);
	double plot_time(0);
	double traj_time(0);
	double com_time(0);
	int vehicle_slot(0);
	bool increment_scrn_time(false);
	bool increment_plot_time(false);
	bool plot_merge(false);
	double out_fact(0);

	//integration loop
	while (sim_time<=(end_time+int_step))
	{
		//vehicle loop
		for (int i=0;i<num_vehicles;i++)
		{
			//slot occupied by current vehicle in 'vehicle_list[]'
			vehicle_slot=i;

			//watching for the next event			
			vehicle_list[i]->event(options);

			//fixing 'event_epoch' and initializing 'event_time'
			if(vehicle_list[i]->event_epoch)
				vehicle_list[i]->event_time=0;


			//continue only if vehicle is alive (health=1:alive; =-1:hit('satellite' only); =0:dead)
			int health=combus[i].get_status();
			if(health==1)
			{
				//refreshing Markov variables
				vehicle_list[i]->markov_noise(sim_time,int_step,nmonte);

				//module loop -MOD: insert here new module function
				for(int j=0;j<num_modules;j++)
				{
					if(module_list[j].name=="newton")
						vehicle_list[i]->newton(int_step);
					else if(module_list[j].name=="euler")
						vehicle_list[i]->euler(int_step);
					else if(module_list[j].name=="kinematics")
						vehicle_list[i]->kinematics(sim_time,vehicle_list[i]->event_time,int_step,out_fact);
					else if(module_list[j].name=="environment")
						vehicle_list[i]->environment(int_step);
					else if(module_list[j].name=="aerodynamics")
						vehicle_list[i]->aerodynamics();
					else if(module_list[j].name=="forces")
						vehicle_list[i]->forces();
					else if(module_list[j].name=="propulsion")
						vehicle_list[i]->propulsion(int_step);
					else if(module_list[j].name=="actuator")
						vehicle_list[i]->actuator(int_step);
					else if(module_list[j].name=="control") 
						vehicle_list[i]->control(int_step);
					else if(module_list[j].name=="ins")
						vehicle_list[i]->ins(int_step);
					else if(module_list[j].name=="guidance")
						vehicle_list[i]->guidance(int_step);
					else if(module_list[j].name=="gps")
						vehicle_list[i]->gps(int_step);
					else if(module_list[j].name=="startrack")
						vehicle_list[i]->startrack();
					else if(module_list[j].name=="rcs")
						vehicle_list[i]->rcs();
					else if(module_list[j].name=="intercept")
						vehicle_list[i]->intercept(combus,vehicle_slot,int_step,title);
					else if(module_list[j].name=="seeker")
						vehicle_list[i]->seeker(combus,num_vehicles,sim_time,int_step);
					else if(module_list[j].name=="datalink")
						vehicle_list[i]->datalink(combus,num_vehicles);
				} //end of module loop

				//preserving 'health' status of vehicle objects
				combus_status(combus,status,num_vehicles);

				//loading data packet into 'combus' communication bus
				combus[i]=vehicle_list[i]->loading_packet(num_hyper,num_satellite,num_radar);

				//refreshing 'health' status of vehicle objects
				combus[i].set_status(status[i]);

			} //end of active vehicle loop

			//tracking time elapsed during event
			vehicle_list[i]->event_time+=int_step;

			//output to screen and/or 'tabout.asc'
			if(fabs(scrn_time-sim_time)<(int_step/2+EPS))
			{
				if(strstr(options,"y_scrn"))
				{
					vehicle_list[i]->scrn_data();
					if(i==(num_vehicles-1))increment_scrn_time=true;
				}

				if(strstr(options,"y_tabout"))
				{
					vehicle_list[i]->tabout_data(ftabout);
				}
				if(increment_scrn_time) scrn_time+=scrn_step*(1+out_fact);
			}

			//output to 'ploti.asc' file
			if(fabs(plot_time-sim_time)<(int_step/2+EPS))
			{
				if(strstr(options,"y_plot"))
				{
					vehicle_list[i]->plot_data(plot_ostream_list[i],plot_merge);
					if(i==(num_vehicles-1))increment_plot_time=true;
				}
				if(increment_plot_time) plot_time+=plot_step*(1+out_fact);
			}
			//output to 'stati.asc' file 
			if(strstr(options,"y_stat"))
			{
				if(vehicle_list[i]->event_epoch)
					vehicle_list[i]->stat_data(stat_ostream_list[i],nmc,i);
				if(!combus[i].get_status()&&stati_write_term[i])
				{
					stati_write_term[i]=false;
					vehicle_list[i]->stat_data(stat_ostream_list[i],nmc,i);
				}
			}

		} //end of vehicle loop

		//outputting 'combus' to screen 
		if(fabs(com_time-sim_time)<(int_step/2+EPS))
		{
			if(strstr(options,"y_comscrn"))
			{
				comscrn_data(combus,num_vehicles);
			}
			com_time+=com_step*(1+out_fact);
		}
		//outputting'combus' to 'traj.asc' file
		if(fabs(traj_time-sim_time)<(int_step/2+EPS))
		{
			if(strstr(options,"y_traj"))
			{
				traj_data(ftraj,combus,num_vehicles,traj_merge);
			}
			traj_time+=traj_step*(1+out_fact);
		}
		//resetting output events
		increment_scrn_time=false;
		increment_plot_time=false;

		//advancing time
		sim_time+=int_step;

	} //end of integration loop

	//writing last integration out to 'ploti.asc' 
	//with time set to '-1' for multiple CADAC-Studio plots
	if(strstr(options,"y_plot"))
	{
		plot_merge=true;
		for (int i=0;i<num_vehicles;i++)
			vehicle_list[i]->plot_data(plot_ostream_list[i],plot_merge);
	}
	//writing last integration out to 'traj.asc' 
	//with time set to '-1' for multiple CADAC-Studio plots
	if(strstr(options,"y_traj"))
	{
		traj_merge=true;
		traj_data(ftraj,combus,num_vehicles,traj_merge);
	}
} 


