///////////////////////////////////////////////////////////////////////////////
///////////////////////   CADAC++ Simulation AIM5   ////////////////////////
///////////////////////////////////////////////////////////////////////////////
//FILE: execution.cpp
//
//Initializing and executing the simulation
//
//020923 Created by Peter H Zipfel
//030319 Upgraded to SM Item32, PZi
//081010 Modified for GENSIM6, PZi
//130724 Building AIM5, PZi
//131025 Compatible with MS C++ V12, PZi
///////////////////////////////////////////////////////////////////////////////

#include "class_hierarchy.hpp"
#include <ctime>

using namespace std;

///////////////////////////////////////////////////////////////////////////////
//////////////// Definition of global function prototypes used in main() //////
///////////////////////////////////////////////////////////////////////////////

//acquiring the simmulation title
void acquire_title_options(fstream &input,char *title,char *options);

//acquiring the simulation run time
double acquire_endtime(fstream &input);

//numbering the modules
void number_modules(fstream &input,int &num);

//acquiring the calling order of the modules
void order_modules(fstream &input,int &num,Module *module_list);

//acquiring the number of vehicle objects
void number_objects(fstream &input,int &num_vehicles,int &num_aim,int &num_aircraft);

//creating a type of vehicle object
Cadac *set_obj_type(fstream &input,Module *module_list,int num_modules,
				   int num_aim,int num_aircraft);

//running the simulation
void execute(Vehicle &vehicle_list,Module *module_list,double sim_time,
			 double end_time,int num_vehicles,int num_modules,double plot_step,
			 double int_step,double scrn_step,double com_step,double traj_step,char *options,
			 ofstream &ftabout,ofstream *plot_ostream_list,Packet *combus,int *status,
			 int num_aim,int num_aircraft,ofstream &ftraj,char *title,bool traj_merge);


// saving status of 'combus' vehicle objects
void combus_status(Packet *combus,int *status,int num_vehicles);

//getting timimg cycles for plotting, screen output and integration
void acquire_timing(fstream &input,double &plot_step,double &scn_step,double &com_step,
					double &traj_step,double &int_step);

//merging the 'ploti.asc' files onto 'plot.asc' 
void merge_plot_files(string *plot_file_list,int num_aim,char *title);

//writing 'combus' data on screen
void comscrn_data(Packet *combus,int num_vehicles);

//writing banner on 'traj.asc' file 
//void traj_banner(ofstream &ftraj,Packet *combus,char *title,int num_vehicles);
void traj_banner(ofstream &ftraj,Packet *combus,char *title,int num_vehicles,
				 int naim,int naircraft);

//writing 'traj.asc' file of 'combus' data
void traj_data(ofstream &ftraj,Packet *combus,int num_vehicles,bool merge);

//Documenting 'input.asc' with module-variable definitions
void document_input(Document *doc_aim5,Document *doc_aircraft3);


///////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////  main()   //////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//Main function
//
//011128 Created by Peter H Zipfel
//130724 Building AIM5, PZi
///////////////////////////////////////////////////////////////////////////////

int main() 
{
	double sim_time=0; //simulation time, same as 'time'
	char title[CHARL]; //title from first line of 'input.asc'
	char options[CHARL]; //options from 'input.asc' for output
	Module *module_list=NULL; //list of modules and sequence as per 'input.asc'
	int num_modules; //number of modules
	double plot_step; //writing time step to 'plot.asc' and 'ploti.asc', i=1,2,3...
	double scrn_step; //writing time step to screen (console)
	double int_step; //integration step size 
	double com_step; //writing time step of 'combus' data to screen
	double traj_step; //writing time step of 'combus' to file 'traj.asc'
	int num_vehicles; //total number of vehicle objects
	int num_aim; //number of aim objects
	int num_aircraft; //number of aircraft objects
	Cadac *vehicle_type=NULL; //array of vehicle object pointers
	char vehicle_name[CHARN]; //name of each vehicle type
	double end_time; //run termination time from 'input.asc'
	string *plot_file_list=NULL; //array containing file names of 'ploti.asc', i=1,2,3...
	ofstream *plot_ostream_list=NULL; //array of output streams for 'ploti.asc', i=1,2,3...
	Packet *combus=NULL; //communication bus container storing data for each vehicle object
					//in same sequence as 'vehicle_list'
	int *status=NULL; //array containing status of each vehicle object
	bool one_traj_banner=true; //write just one banner on file 'traj.asc'
	Document *doc_aim5=NULL;  //array for documenting AIM5 module-variables of 'input.asc'
	Document *doc_aircraft3=NULL;  //array for documenting AIRCRAFT3 module-variables of 'input.asc'
	bool document_aim5=false; //true if doc_aim5 was created
	bool document_aircraft3=false; //true if doc_aircraft3 was created

	///////////////////////////////////////////////////////////////////////////
	/////////////// Opening of files and creation of stream objects  //////////
	///////////////////////////////////////////////////////////////////////////

	//creating an input stream object and opening 'input.asc' file
	fstream input("CADAC_SIMULATIONS/AIM5/input.asc");
	if(input.fail())
	{cerr<<"*** Error: File stream 'input.asc' failed to open (check spelling) ***\n";system("pause");exit(1);}

	//creating an output stream object and opening 'tabout.asc' file
	ofstream ftabout("CADAC_SIMULATIONS/AIM5/tabout.asc");
	if(!ftabout){cout<<" *** Error: cannot open 'tabout.asc' file *** \n";system("pause");exit(1);}

	//creating an output stream object and opening 'doc.asc' file
	ofstream fdoc("CADAC_SIMULATIONS/AIM5/doc.asc");
	if(!fdoc){cout<<" *** Error: cannot open 'doc.asc' file *** \n";system("pause");exit(1);}

	//creating an output stream object and opening 'traj.asc' file
	ofstream ftraj("CADAC_SIMULATIONS/AIM5/traj.asc");
	if(!ftraj){cout<<" *** Error: cannot open 'traj.asc' file *** \n";system("pause");exit(1);}

	//creating file 'input_copy.asc' in local directory for use in 'document_input()'
	ofstream fcopy("CADAC_SIMULATIONS/AIM5/input_copy.asc");
	if(!fcopy){cout<<" *** Error: cannot open 'input_copy.asc' file *** \n";system("pause");exit(1);}

	///////////////////////////////////////////////////////////////////////////
	
	//initializing flags
	bool one_screen_banner=true; //write just one banner on screen
	bool document_aim=true; //write  to 'doc.asc' aim module-variables only once
	bool document_aircraft=true; //write  to 'doc.asc' aircraft module-variables only once
	bool traj_merge=false; //flag used in writing 'time=-1' endblock on 'traj.asc'
		
	//aqcuiring title statement and option selections
	acquire_title_options(input,title,options);

	//acquiring number of module 
	number_modules(input,num_modules);

	//dynamic memory allocation of array 'module_list'
	try{module_list=new Module[num_modules];} 
	catch(bad_alloc xa){cout<< "*** Allocation failure of 'module_list' ***\n";return 1;}
					
	//acquiring calling order of module 
	order_modules(input,num_modules,module_list);
		
	//acquiring the time stepping
	acquire_timing(input,plot_step,scrn_step,int_step,com_step,traj_step);

	//acquiring number of vehicle objects from 'input.asc'
	number_objects(input,num_vehicles,num_aim,num_aircraft);

	//creating the 'vehicle_list' object
	// at this point the constructor 'Vehicle' is called and memory is allocated
	Vehicle vehicle_list(num_vehicles);
		
	//allocating memory for 'ploti.asc' file streams
	try{plot_ostream_list=new ofstream[num_vehicles];}
	catch(bad_alloc xa){cerr<<"*** Allocation failure of 'plot_ostream_list' *** \n";return 1;}

	//allocating memory for 'ploti.asc' files
	try{plot_file_list=new string[num_vehicles];}
	catch(bad_alloc xa){cerr<<"*** Allocation failure of 'plot_file_list' *** \n";return 1;}

	//allocating memory for 'combus'
	try{combus=new Packet[num_vehicles];}
	catch(bad_alloc xa){cerr<<"*** Allocation failure of 'combus' *** \n";return 1;}

	//allocating memory for 'status'
	try{status=new int[num_vehicles];}
	catch(bad_alloc xa){cerr<<"*** Allocation failure of 'status' *** \n";return 1;}

	//initialize 'status' to 'alive=1'
	for(int ii=0;ii<num_vehicles;ii++) status[ii]=1;

	///////////////////////////////////////////////////////////////////////
	////////////////// Initializing each vehicle object  //////////////////
	///////////////////////////////////////////////////////////////////////

	for(int i=0;i<num_vehicles;i++)
	{
		//Loading pointers of the vehicle-object classes ('Aim'.'Aircraft') into 
		// the i-th location of 'vehicle_list'. Vehicle type is read in from 'input.asc' file. 
		//The loading process allocates dynamic memory at the pointer location
		// as required by the vehicle object 
		//The function returns the 'vehicle_type' as specified in 'input.asc' 
		//Furthermore, it passes 'module_list', 'num_modules','num_aim'and'num_aircraft'
		// to the 'Aim' and 'Aircraft' constructors
		vehicle_type=set_obj_type(input,module_list,num_modules,num_aim,num_aircraft);
 				
		//add vehicle to 'vehicle_list'
		vehicle_list.add_vehicle(*vehicle_type);

		//getting the name of the type of vehicle
		strcpy(vehicle_name,vehicle_list[i]->get_vname());

		//vehicle data and tables read from 'input.asc' 
		vehicle_list[i]->vehicle_data(input);

		//executing initialization computations -MOD		
		for (int j=0;j<num_modules;j++)
		{
			if((module_list[j].name=="kinematics")&&(module_list[j].initialization=="init"))
				vehicle_list[i]->init_kinematics(sim_time);
			else if((module_list[j].name=="control")&&(module_list[j].initialization=="init"))
				vehicle_list[i]->init_control();
			else if((module_list[j].name=="newton")&&(module_list[j].initialization=="init"))
				vehicle_list[i]->init_newton();
		}
		//writing banner to screen and file 'tabout.asc'
		if(!strcmp(vehicle_name,"AIM5")&&one_screen_banner)
		{
			one_screen_banner=false;
			if(strstr(options,"y_scrn"))
			{
				vehicle_list[i]->scrn_banner(); 
			}
			if(strstr(options,"y_tabout"))
			{	
				vehicle_list[i]->tabout_banner(ftabout,title);
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

		//executing 'plot' file initialization and documentation
		//creating output stream objects in 'plot_ostream_list[i]' for every file "ploti.asc"
		if(strstr(options,"y_plot"))
		{
			if(!strcmp(vehicle_name,"AIM5"))
			{
				char index[CHARN]; //
				string plotiasc; //plot file name
				const char *name;

				//building names for plot files
				sprintf(index,"%i",i+1);
				plotiasc="CADAC_SIMULATIONS/AIM5/plot"+string(index)+".asc"; //using Standard Library string constructor
				plot_file_list[i]=plotiasc;
				name=plotiasc.c_str(); //using string member function to convert to char array 

				//creating output stream list 'plot_ostream_list[i]', each will write on file 'ploti.asc'
				plot_ostream_list[i].open(name); //'name' must be 'char' type

				//writing banner on 'ploti.asc'
				vehicle_list[i]->plot_banner(plot_ostream_list[i],title);				}
		}

		//composing documentation of 'flat3','aim' and 'aircraft' module-variables
		//but do it only once for AIM5 and AIRCRAFT3 objects
		if(strstr(options,"y_doc"))
		{
			if(!strcmp(vehicle_name,"AIM5")&&document_aim)
			{
				document_aim=false;
				int size=NFLAT3+NAIM;
				try{doc_aim5=new Document[size];}
				catch(bad_alloc xa){cerr<<"*** Allocation failure of 'doc_aim5' *** \n";return 1;}
				vehicle_list[i]->document(fdoc,title,doc_aim5);
				document_aim5=true;
			}
			if(!strcmp(vehicle_name,"AIRCRAFT3")&&document_aircraft)
			{
				document_aircraft=false;
				int size=NFLAT3+NAIRCRAFT;
				try{doc_aircraft3=new Document[size];}
				catch(bad_alloc xa){cerr<<"*** Allocation failure of 'doc_aircraft3' *** \n";return 1;}
				vehicle_list[i]->document(fdoc,title,doc_aircraft3);
				document_aircraft3=true;
			}
		}

		//loading packets into 'combus' communication bus
		combus[i]=vehicle_list[i]->loading_packet_init(num_aircraft,num_aim);

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
		traj_banner(ftraj,combus,title,num_vehicles,num_aim,num_aircraft);

		//writing data after 'initial module' calculations
		traj_data(ftraj,combus,num_vehicles,traj_merge);
	}

	//acuire ending time (last entry on 'input.asc')
	// and put file pointer at the beginning of 'input.asc' for MC repeat runs
	end_time=acquire_endtime(input);

	if(strstr(options,"y_doc")){
		//documenting input.asc (only once)
		document_input(doc_aim5,doc_aircraft3);
		if(document_aim5)delete [] doc_aim5;
		if(document_aircraft3)delete [] doc_aircraft3;
	}
	///////////////////////////////////////////////////////////////////////	
	/////////////////////// Simulation Execution //////////////////////////
	///////////////////////////////////////////////////////////////////////	

	execute(vehicle_list,module_list,sim_time,
				end_time,num_vehicles,num_modules,plot_step,
				int_step,scrn_step,com_step,traj_step,options,
				ftabout,plot_ostream_list,combus,status,
				num_aim,num_aircraft,ftraj,title,traj_merge);

	//deallocating dynamic memory
	delete [] module_list;
	delete [] combus;
	delete [] status;

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

	//merging 'ploti.asc' files into 'plot.asc'
	if(strstr(options,"y_merge")&&strstr(options,"y_plot"))
	{
		merge_plot_files(plot_file_list,num_aim,title);
	}
	//deallocate dynamic memory
	delete [] plot_ostream_list;
	delete [] plot_file_list;

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
//								and their respective type (aim, or aircraft)
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
//				*plot_ostream_list = output file-steam list of 'ploti.asc' for each individual missile 
//								missile object
//				*combus = commumication bus
//				*status = health of vehicles
//				num_aim = number of 'Aim' objects
//				num_aircraft = number of 'Aircraft' objects
//				&ftraj = output file-stream to 'traj.asc'
//				*title = idenfication of run
//				traj_merge = flag for merging MC runs in 'traj.asc'
//				  				
//011128 Created by Peter H Zipfel
//040705 Calculating 'event_time', PZi
//070531 Incrementing 'sim_time' in 'combus' until 'ENDTIME' is reached, PZi
//081010 Modified for GENSIM6, PZi
//130724 Building AIM5, PZi
///////////////////////////////////////////////////////////////////////////////
void execute(Vehicle &vehicle_list,Module *module_list,double sim_time,
			 double end_time,int num_vehicles,int num_modules,double plot_step,
			 double int_step,double scrn_step,double com_step,double traj_step,char *options,
			 ofstream &ftabout,ofstream *plot_ostream_list,Packet *combus,int *status,
			 int num_aim,int num_aircraft,ofstream &ftraj,char *title,bool traj_merge)
{
	double scrn_time(0);
	double plot_time(0);
	double traj_time(0);
	double com_time(0);
	int vehicle_slot(0);
	bool increment_scrn_time(false);
	bool increment_plot_time(false);
	bool plot_merge(false);

	//integration loop
	while (sim_time<=(end_time+int_step))
	{
		//vehicle loop
		for (int i=0;i<num_vehicles;i++)
		{
			//vehicle is progressing

			//slot occupied by current vehicle in 'vehicle_list[]'
			vehicle_slot=i;

			//watching for the next event			
			vehicle_list[i]->event(options);

			//fixing 'event_epoch' and initializing 'event_time'
			if(vehicle_list[i]->event_epoch)
				vehicle_list[i]->event_time=0;

			//continue only if vehicle is alive (health=1:alive; =-1:hit('aim' only); =0:dead)
			int health=combus[i].get_status();
			if(health==1)
			{
				//module loop -MOD
				for(int j=0;j<num_modules;j++)
				{
					if(module_list[j].name=="environment")
						vehicle_list[i]->environment();
					else if(module_list[j].name=="kinematics")
						vehicle_list[i]->kinematics(sim_time,vehicle_list[i]->event_time);
					else if(module_list[j].name=="newton")
						vehicle_list[i]->newton(int_step);
					else if(module_list[j].name=="aerodynamics")
						vehicle_list[i]->aerodynamics();
					else if(module_list[j].name=="propulsion")
						vehicle_list[i]->propulsion();
					else if(module_list[j].name=="forces")
						vehicle_list[i]->forces();
					else if(module_list[j].name=="control") 
						vehicle_list[i]->control(int_step);
					else if(module_list[j].name=="guidance")
						vehicle_list[i]->guidance(combus,num_vehicles);
					else if(module_list[j].name=="seeker")
						vehicle_list[i]->seeker(combus,num_vehicles,sim_time,int_step);
					else if(module_list[j].name=="intercept")
						vehicle_list[i]->intercept(combus,vehicle_slot,int_step,title);
				} //end of module loop

				//preserving 'health' status of vehicle objects
				combus_status(combus,status,num_vehicles);

				//loading data packet into 'combus' communication bus
				combus[i]=vehicle_list[i]->loading_packet(num_aircraft,num_aim);

				//refreshing 'health' status of vehicle objects
				combus[i].set_status(status[i]);


				//continuing incrementing 'sim_time' in combus packets until 'ENDTIME' is reached
				combus[i].set_data_variable(0,sim_time);

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
					if(increment_scrn_time) scrn_time+=scrn_step;
				}

				//output to 'ploti.asc' file
				if(fabs(plot_time-sim_time)<(int_step/2+EPS))
				{
					if(strstr(options,"y_plot"))
					{
						vehicle_list[i]->plot_data(plot_ostream_list[i],plot_merge);
						if(i==(num_vehicles-1))increment_plot_time=true;
					}
					if(increment_plot_time) plot_time+=plot_step;
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
			com_time+=com_step;
		}
		//outputting'combus' to 'traj.asc' file
		if(fabs(traj_time-sim_time)<(int_step/2+EPS))
		{
			if(strstr(options,"y_traj"))
			{
				traj_data(ftraj,combus,num_vehicles,traj_merge);
			}
			traj_time+=traj_step;
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