/*
    Author: Mylonakis Emmanouil
    Registration Number: 2015030079
    Date: 30/03/2021
    Operating System: Windows 10.

    Description: Exercise 3. Create Dataset for the simple RSYM simulator.                           
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

/* ============== LAB_3 ============================= */
int CMOS_NET[1024][4]; // Store netlist from Library file
int EQUAL_NODES[2048][32]; //Keep equivelant of nodes as we change their number from input file.
int VCC_lib[256]; //Vcc nodes from library file
int GND_lib[256]; //GND nodes from library file
int VCC_RAILS = -1;//[VCC/GND]_RAILS in case input file defines VCC or GND. Otherwise, no use.
int GND_RAILS = -1;
int *INPUT_new = NULL; // Section ## INPUTS, store input nodes dynamically.
int *OUTPUT_new = NULL;
char lib_file[64];
char file_out[64];
int index_VCC_GND_lib = 0;
void derive_output_file_for_lab_2(FILE *fp);
int create_CMOS_Netlist(FILE *fp);
void read_lib_file_until_gate(FILE *fp, char *GateName);
int search_Array(int *A, int size, int x);
/* 
    Update array EQUAL_NODES with the node-value from netlist of file.
    Array A has the values-nodes from section IN {NODE1, NODE2,...}
    or the value-node from section OUT {NODE}
*/
void update_EQUAL_NODES_col1(int *A, int size);

/*
    Finds the row with node-value = Node and updates
    the respective column with Node_New. So, the
    Node and Node_New are equivelant. 
    No overwrites. 
*/
void update_EQUAL_NODES_row(int Node, int Node_New);
void final_configuration(int i); //Updates CMOS_NET with the nodes of VCC,GND and EQUAL_NODES[].

/* BELOW ARE FUNCTION FOR LAB_2 */
/* ========== FOR MAIN  FUNCTIONALITY ========== */
void run_testbench(FILE *fp); /*Running the testbench. Supports multiple testbenches, 
                                calls create_graph(), then update stores-updates the values
                                from sections #TEST_IN #TEST_OUT and ## TEST_VECTORS,
                                then, call function simulate().
                                Terminates if it reads from file ## END_SIMULATION,*/
void simulate(); //Simulates circuit in digital level.
void create_graph(); // Creates the Graph.

/* ========== UTILITY FUNCTIONS ========== */
void checkInput(int argc, char *argv[]); // Checks User's Input Correcteness.
void usage(); //Prints a help message for user.
void free_pointer(); //free memory before program terminate.
int gate_value(int G, int index); //Returns transistror's gate value G from graph[index][].
int get_info_node(int N, int index); //Returns node's information-value from graph[index][].
bool is_node_output(int N); // Check if the node N is OUTPUT.
void update_nodes_info(int Node, int index, int value);//Update Node of graph[index][] with value-color.
void print_results(int x); //Print results. values x specifies from wich graph.
void reset_graph(); //Reset graph for the next simulate/for the next TEST_VECTOR
int isGraphConverged(int i, int j); //Check if the graph has converged in order to stop simulating.
void update_tests_IOs(FILE *fp);//Stores from file the values from section ## TEST_IN ## TEST_OUT
void update_test_vector(FILE *fp); //Stores from file the values from section ## VECTOR
void store_circuit_infos(FILE *fp); //Stores from file every info until the section ## TESTBENCH.
void store_RAILS(char *buff);                 //Update infos from section ## RAILS.
int call_strtok(char *line, int **array);  // Calls repeattadly strtok() function. Allocate array's size properly and returns the number of rows.
void my_test();//Just a printing for me :).

int VCC = -1, GND = -1; // Section ## RAILS, store nodes for VCC and GNB.
int *INPUT = NULL; // Section ## INPUTS, store input nodes dynamically.
int *OUTPUT = NULL; // Section ## OUTPUTS, store output nodes dynamically.
int *TEST_IN = NULL; // Section ## TEST_IN, store test-input nodes dynamically
int *TEST_OUT = NULL; // Section ## TEST_OUT, store test-output nodes dynamically
int *VECTOR = NULL; // Section ## TEST_VECTOR, store test-vector nodes dynamically
int **MOS = NULL; // Section ## NETLIST, store infos for every transistor dynamically.
int **graph = NULL; /*For the Graph. Dimensions are [4]x[#Nodes]. Columns are generated dynamically.
                      *Graph[0] = [Node_1, Node_2, ..., Node_n]
                      *Graph[1] = the initial values. Ex If Node_1=VCC and Node_4 = GND =>
                                => *Graph[1] = [1, X, X, 0, X, ..., X]
                     *Graph[2] and Graph[3] stores the values of the nodes in correspondence with the columns/node.
                     *Graph[0] and *Graph[1] never change. We swap 2 pointers between the rows 2 and 3
                     to take and update information.
                    */
int numOfInputs, numOfOutputs; //Number of Inputs and Outputs our circuit has.
int numOfMos = 0; //Number of Transistor in our netlist.
int numOfTestIn, numOfTestOut, vector_len; //Number of TEST_IN, TEST_OUT and TEST_VECTOR's length.
int nodes = 0;//Number of Nodes.

char filename_out[100]; //output's file name.

int main(int argc, char *argv[])
{
    checkInput(argc, argv); //Check input's correctness.

    FILE* fp = fopen(argv[1], "r");
    if(fp == NULL)
    {
        printf("Error: fopen failed :(\n");
        exit(1);
    }
    
    //Lab 3.
    sprintf(file_out, "LAB_3_OUTPUT_%s", argv[1]);
    FILE *fp_out = fopen(file_out, "w");
    fclose(fp_out);
    derive_output_file_for_lab_2(fp);
    
    //Lab 2.
    if (INPUT != NULL)
        free(INPUT);
    if(OUTPUT != NULL)
        free(OUTPUT);
    
    numOfInputs = 0;
    numOfOutputs = 0;
    FILE *fp_in = fopen(file_out, "r");
    //Create and clear output file.
    sprintf(filename_out, "Result_%s", file_out);
    fp_out = fopen(filename_out, "w");
    fclose(fp_out);
    //Store circuit's information.
    store_circuit_infos(fp_in);
    //Run Testbench.
    run_testbench(fp_in);

    //Release memory.
    free_pointer();
    fclose(fp_in);
    return 0;
}

/* =========================== LAB 3 ============================== */
void derive_output_file_for_lab_2(FILE *fp_in)
{
    char *lineBuffer = NULL;
    size_t len = 0;
    ssize_t read;

    int Netlist_rows;

    while ((read = getline(&lineBuffer, &len, fp_in)) != -1)
    {
        if (strcmp(lineBuffer, "## LIBRARY\n") == 0)
        {
            getline(&lineBuffer, &len, fp_in);
            sprintf(lib_file, "%s", lineBuffer);
            lib_file[strlen(lineBuffer)-1] = '\0';
        }
        if(strcmp(lineBuffer, "## RAILS\n") == 0)
        {
            getline(&lineBuffer, &len, fp_in);
            store_RAILS(lineBuffer);//Func from Lab_2. Stores result in global variables VCC and GND
            VCC_RAILS = VCC;
            GND_RAILS = GND;
        }

        if (strcmp(lineBuffer, "## INPUTS\n") == 0)
        {
            getline(&lineBuffer, &len, fp_in);
            numOfInputs = call_strtok(lineBuffer, &INPUT);
            INPUT_new = (int *)malloc(sizeof(int) * numOfInputs);
            for (int i = 0; i<numOfInputs; i++)
                INPUT_new[i] = INPUT[i];
        }
        if (strcmp(lineBuffer, "## OUTPUTS\n") == 0)
        {
            getline(&lineBuffer, &len, fp_in);
            numOfOutputs = call_strtok(lineBuffer, &OUTPUT);
            OUTPUT_new = (int *)malloc(sizeof(int) * numOfOutputs);
            
            for (int i = 0; i < numOfOutputs; i++)
                OUTPUT_new[i] = OUTPUT[i];
        }
        if (strcmp(lineBuffer, "## NETLIST\n") == 0)
        {
            Netlist_rows = create_CMOS_Netlist(fp_in);
            break;
        }

    }
    //Lets Create the output file.
    fseek(fp_in, 0, SEEK_SET);
    while ((read = getline(&lineBuffer, &len, fp_in)) != -1)
        if(strcmp(lineBuffer, "## TESTBENCH\n") == 0) break;

    FILE *fp_out = fopen(file_out, "a");
    fprintf(fp_out, "## RAILS\n");
    fprintf(fp_out, "VCC %d ; GND %d\n", VCC_lib[0], GND_lib[0]);

    fprintf(fp_out, "## INPUTS\n");
    for(int i=0; i<numOfInputs; i++)
        fprintf(fp_out, "%d ; ", INPUT_new[i]);

    fprintf(fp_out, "\n## OUTPUTS\n");
    for (int i = 0; i < numOfOutputs; i++)
        fprintf(fp_out, "%d ; ", OUTPUT_new[i]);

    fprintf(fp_out, "\n## NETLIST\n");
    for(int i=0; i<Netlist_rows; i++)
    {
        fprintf(fp_out, "U%d ", i+1);
        if(CMOS_NET[i][0] == 'P')
            fprintf(fp_out, "PMOS ");
        else if (CMOS_NET[i][0] == 'N')
            fprintf(fp_out, "NMOS ");
        
        for(int j=1; j<4; j++)
            fprintf(fp_out, "%d ", CMOS_NET[i][j]);
        fprintf(fp_out, "\n");        
    }

    fprintf(fp_out, "## TESTBENCH\n");
    //While No End of file keep reading. Below code supports multiple testbenches.
    while ((read = getline(&lineBuffer, &len, fp_in)) != -1)
    {

        if (feof(fp_in)) break;

        lineBuffer[strlen(lineBuffer) - 1] = '\0';        
        if (strcmp(lineBuffer, "## TEST_IN") == 0)
        {
            /* Find the relation betweet the given TEST_IN and TEST_OUT we are reading from file
               and change their values with new INPUT and OUTPUT we have created in netlist.
               Thus, if TEST_(IN/OUT) haven't every input/output-node, in case user would like to check a sub-circuit,
               we create TEST_(IN/OUT) propeply.
            */
            fprintf(fp_out, "## TEST_IN\n");
            update_tests_IOs(fp_in);           
            for(int i=0; i<numOfTestIn; i++)
            {
                int pos = search_Array(INPUT, numOfInputs, TEST_IN[i]);
                if(pos >= 0)
                    fprintf(fp_out, " %d ;", INPUT_new[pos]);               
            }
            fprintf(fp_out, "\n## TEST_OUT\n");
            for (int i = 0; i < numOfOutputs; i++)
            {
                int pos = search_Array(OUTPUT, numOfOutputs, TEST_OUT[i]);
                if (pos >= 0)
                    fprintf(fp_out, " %d ;", OUTPUT_new[pos]);
            }
            fprintf(fp_out, "\n");

            if (TEST_IN != NULL)
                free(TEST_IN);

            if (TEST_OUT != NULL)
                free(TEST_OUT);
        }
        else
            fprintf(fp_out, "%s\n", lineBuffer);
    }

    fclose(fp_in);
    fclose(fp_out);
}

int create_CMOS_Netlist(FILE *fp)
{
    // Variables for file reading.
    char *line_buf = NULL;
    FILE *fp_lib;
    char *lib_buf = NULL;
    size_t len = 0;
    ssize_t read;
    
    // Variables for strtok function
    const char delims[] = "';', ',', ' '";
    char *component = NULL;
    char *gates_name = NULL;
    char *token_node = NULL;
    //Some usefull variables.
    char this_VCC[16]; //Current VCC from Library file
    char this_GND[16]; //Current GND from Library file
    int *this_IN_lib; //Current INPUTS from Libray file
    int *this_OUT_lib; //Current INPUTS from Libray file
    int IN_NODES[8]; //Input nodes where the specific gate adapts
    int OUT_NODES[8]; //Output node where the specific gate adapts. Size of 8, just a the very very special case.
    int numOfIns, numOfOuts;

    //Some usefull indexes.
    int offset = 1; //What number of repetition. We are going to concat this number with the number of node.
    int CMOS_NET_row = 0;
    int CMOS_NET_col = 1; //?
    //The number -1 is declares the end, because we are not going to
    //have equal number of columns per row.
    EQUAL_NODES[0][0] = -1; // Just an initiation.
    EQUAL_NODES[0][1] = -1;
    EQUAL_NODES[1][0] = -1;
    EQUAL_NODES[1][1] = -1;

    fp_lib = fopen(lib_file, "r"); // Open Library file for reading.

    while ((read = getline(&line_buf, &len, fp)) != -1)
    {
        if (strcmp(line_buf, "## TESTBENCH\n") == 0)
            break;

        fseek(fp_lib, 0, SEEK_SET); //Seek pointer of lib file back to start.
        component = strtok(line_buf, delims); // is GX or UX ?

        if (component[0] == 'G')
        {
            //Store the nodes from file where pins of gates adapt.
            gates_name = strtok(NULL, delims); // Name of Gate       
            read_lib_file_until_gate(fp_lib, gates_name); //Guide FP in the specific gate of library file
            
            //Now, let's store IN, where gate adapts from current line of file.
            strtok(NULL, delims); // Section IN
            int i = 0;
            while (1)
            {
                token_node = strtok(NULL, delims);
                if (strcmp(token_node, "OUT") == 0) // Section OUT
                    break;

                IN_NODES[i] = atoi(token_node);
                i++;
            }
            //Store Out.
            i=0;
            while (1)
            {
                token_node = strtok(NULL, delims);
                if (token_node == NULL) // Section OUT
                    break;

                OUT_NODES[i] = atoi(token_node);
                i++;
            }

            //Store VCC/GND from library file
            getline(&lib_buf, &len, fp_lib);  // ## RAILS
            getline(&lib_buf, &len, fp_lib);  // {VCC NODE}/{GND NODE}
            lib_buf[strlen(lib_buf) - 1] = '\0';         
            store_RAILS(lib_buf); //Func from LAB_2. Store VCC and GND in global vars.
            sprintf(this_VCC, "%d%d", offset, VCC);
            sprintf(this_GND, "%d%d", offset, GND);
            VCC_lib[index_VCC_GND_lib] = atoi(this_VCC);
            GND_lib[index_VCC_GND_lib] = atoi(this_GND);
            
            //Store Inputs from library file
            getline(&lib_buf, &len, fp_lib); // ## INPUTS
            getline(&lib_buf, &len, fp_lib); // {NODE, NODE, ...}
            lib_buf[strlen(lib_buf) - 1] = '\0';
            numOfIns = call_strtok(lib_buf, &this_IN_lib);
            
            //Store Outputs from library file
            getline(&lib_buf, &len, fp_lib); // ## OUTPUTS
            getline(&lib_buf, &len, fp_lib); // {NODE, NODE, ...}
            lib_buf[strlen(lib_buf) - 1] = '\0';
            numOfOuts = call_strtok(lib_buf, &this_OUT_lib);
            //Store Nodes from file if they don't exist in EQUAL_NODES.
            update_EQUAL_NODES_col1(IN_NODES, numOfIns);
            update_EQUAL_NODES_col1(OUT_NODES, numOfOuts);

            //Store Netlist from Library File
            getline(&lib_buf, &len, fp_lib);//Section ## NETLIST
            while ((read = getline(&lib_buf, &len, fp_lib)) != -1)
            {
                lib_buf[strlen(lib_buf) - 1] = '\0';
                if(strcmp(lib_buf, "## END_GATE") == 0)
                    break;

                strtok(lib_buf, delims); //UX
                //The Name token_node of var there is nothing about its meaning.
                //It just have arleady declared and no more usefull, so should i declare a new one ?
                token_node = strtok(NULL, delims); //[P/N]MOS
                if(strcmp(token_node, "PMOS") == 0)
                    CMOS_NET[CMOS_NET_row][0] = 'P';                
                else if(strcmp(token_node, "NMOS") == 0)
                    CMOS_NET[CMOS_NET_row][0] = 'N';
                
                while((token_node = strtok(NULL, delims)) != NULL)
                {
                    char buf[5];
                    int node_new;
                    int node = atoi(token_node);

                    sprintf(buf, "%d%d", offset, node);
                    node_new = atoi(buf);
                    //Check if Node is input or output in library file.
                    bool isInput = false;
                    bool isOutput = false;
                    int i=0;
                    int j=0;
                    while (i < numOfIns || j < numOfOuts)
                    {
                        if (i < numOfIns)
                        {
                            if(this_IN_lib[i] == node)
                            {
                                isInput = true;
                                break;
                            }
                        }

                        if (j < numOfOuts)
                        {
                            if (this_OUT_lib[j] == node)
                            {
                                isOutput = true;
                                break;
                            }
                        }
                        i++;
                        j++;
                    }
                    
                    if (isInput) //If is Input-node in library file
                    {
                        // If is input-node for the circuit.
                        int pos = search_Array(INPUT_new, numOfInputs, IN_NODES[i]);
                        if (pos >= 0) 
                            INPUT_new[pos] = node_new;//We have new input node.

                        update_EQUAL_NODES_row(IN_NODES[i], node_new);

                    }
                    else if (isOutput)//If is Output-node in library file
                    {
                        // If is output-node for the circuit.
                        int pos = search_Array(OUTPUT_new, numOfOutputs, OUT_NODES[i]);
                        if (pos >= 0)
                            OUTPUT_new[pos] = node_new;
                        update_EQUAL_NODES_row(OUT_NODES[j], node_new);
                    }
                    //Store node in new netlist with the addition offset.
                    CMOS_NET[CMOS_NET_row][CMOS_NET_col] = node_new;
                    CMOS_NET_col++;
                }

                CMOS_NET_row++;
                CMOS_NET_col = 1;
            }
        }
        else if (component[0] == 'U')
        {
            //Check if is PMOS or NMOS;
            token_node = strtok(NULL, delims); //We already used once before the if condition.
            if (strcmp(token_node, "PMOS") == 0)
                CMOS_NET[CMOS_NET_row][0] = 'P';            
            else if (strcmp(token_node, "NMOS") == 0)
                CMOS_NET[CMOS_NET_row][0] = 'N';

            //Now for each pin.
            int CMOS_NET_col = 1;
            while((token_node = strtok(NULL, delims)) != NULL )
            {
                char buf[5];
                int node_new;
                int pin = atoi(token_node);

                sprintf(buf, "%d%d", offset, pin);
                node_new = atoi(buf);
                //If pin is VCC from info we have about RAILS.
                if (pin == VCC_RAILS)
                {
                    VCC_lib[index_VCC_GND_lib] = node_new;
                    CMOS_NET[CMOS_NET_row][CMOS_NET_col] = node_new;
                    CMOS_NET_col++; //For the next pin.
                    continue;
                }   
                //If pin is GND from info we have about RAILS.
                if (pin == GND_RAILS)
                {
                    GND_lib[index_VCC_GND_lib] = node_new;
                    CMOS_NET[CMOS_NET_row][CMOS_NET_col] = node_new;
                    CMOS_NET_col++; //For the next pin.
                    continue;
                }

                int pos = search_Array(INPUT_new, numOfInputs, pin);
                if (pos >= 0)
                    INPUT_new[pos] = node_new;
                

                pos = search_Array(OUTPUT_new, numOfOutputs, pin);
                if (pos >= 0)
                    OUTPUT_new[pos] = node_new;

                //Now we update BOTH CMOS_NET and EQUAL_NODES arrays.
                //Every pin of transistor is a node for our graph exept those wich adapted with VCC or GND.
                //We used command continue, so the below nodes aren't VCC or GND.
                //So, we need the respective equivalences.
                CMOS_NET[CMOS_NET_row][CMOS_NET_col] = node_new;
                update_EQUAL_NODES_col1(&pin, 1); // Store node if doesn't exist.
                update_EQUAL_NODES_row(pin, node_new); //Create the equivelance pin-node_new
                CMOS_NET_col++;//For the next pin.
                
            }
            CMOS_NET_row++;
        }
        //Increase indexes
        offset++;
        index_VCC_GND_lib++;
    }

    final_configuration(CMOS_NET_row);
    fclose(fp_lib);
    return CMOS_NET_row;
}

void final_configuration(int NET_index)
{
    //Define VCC and GND nodes.
    //Apply node equivelances from array EQUAL_NODES
    //in order to have our connections properly.
    for(int i=0; i<NET_index; i++)
    {
        for(int j=1; j<4; j++)
        {
            
            for(int v=1; v<index_VCC_GND_lib; v++)
            {
                if (VCC_lib[v] == CMOS_NET[i][j])
                {
                    //Update with the first value in table VCC_lib.
                    CMOS_NET[i][j] = VCC_lib[0];
                }
            
                if (GND_lib[v] == CMOS_NET[i][j])
                {
                    //Update with the first value in table GND_lib.
                    CMOS_NET[i][j] = GND_lib[0];
                }
            }

            //Find node equivelance
            int x = 0;
            int y = 1;
            bool isFound = false;
            while (EQUAL_NODES[x][y] != -1)//We termiate with value -1
            {
                while (EQUAL_NODES[x][y] != -1)
                {
                    if (EQUAL_NODES[x][y] == CMOS_NET[i][j]) // Just Found !
                    {
                        CMOS_NET[i][j] = EQUAL_NODES[x][1];//Update with the equivelant one.
                        isFound = true;//Break, no more search needed.
                        break;
                    }
                    y++;
                }

                if (isFound)
                    break;
                x++;
                y = 1;
            }
        }
    }
            
}

void update_EQUAL_NODES_row(int Node, int Node_New)
{
    int i=0;
    int j=1;

    //Find in which row the Node exists.
    while(EQUAL_NODES[i][0] != -1)
    {
        if (EQUAL_NODES[i][0] == Node)
            break;//Break to keep position.
        i++;
    }

    //Search in above row we stopped.
    while(EQUAL_NODES[i][j] != -1)
    {   //If Node_new already exists, don't update and return.
        if (EQUAL_NODES[i][j] == Node_New)        
            return;        
        j++;
    }
    //We didnt find Node_New, so update.
    EQUAL_NODES[i][j] = Node_New;
    EQUAL_NODES[i][j+1] = -1;
}

void update_EQUAL_NODES_col1(int *A, int size)
{
    bool isExist = false;
    //In first col1 of array EQUAL_NODE
    for(int i=0; i<size; i++)
    {
        int j=0;
        //Check if node we are about to update already exists.
        while(EQUAL_NODES[j][0] != -1)
        {
            if (EQUAL_NODES[j][0] == A[i])
            {
                isExist = true;
                break;
            }
            j++;
        }

        //If doesn't exist then update.
        if(!isExist)
        {
            EQUAL_NODES[j][0] = A[i];//Update
            EQUAL_NODES[j][1] = -1; //Update Right cell with terminal value
            EQUAL_NODES[j+1][0] = -1; //Update Βottom cell with terminal value
            EQUAL_NODES[j+1][1] = -1; //Update diagonal right cell with terminal value
        }

        isExist = false;
    }

}

int search_Array(int *A, int size, int x)
{
    for(int i=0; i<size; i++)
    {
        if(A[i] == x)
            return i;
    }
    return -1;
}


void read_lib_file_until_gate(FILE *fp, char *GateName)
{
    char *line_buf = NULL;
    size_t len = 0;
    ssize_t read;
    
    char GATE_X[32];

    sprintf(GATE_X, "## GATE %s\n", GateName);

    while((read = getline(&line_buf, &len, fp)) != -1)
    {
        if (strcmp(line_buf, GATE_X) == 0)
            return;
    }

    printf("Gate does not exist in my library.");
    exit(1);
}
/* =========================== LAB 2 ============================== */
/* ========== BELOW FUNCTIONS ARE FOR MAIN FUNCTIONALITY ========== */
void simulate()
{
    int info_index = 1; //From which graph we take the information about node.
    int update_index = 2; //In whicn graph we update the new information.
    int converges = 0; //To terminate when graph has converge.
    //First of all, we are using Graph[1] (the initial values) 
    reset_graph(); //Reset graph for new simulation.

    while (converges < 2) // 2 stable states for the graph are enough to have convergence.
    {
        for (int i = 0; i < numOfMos; i++)//We are crossing netlist from top->bottom.
        {
            int gate = gate_value(MOS[i][1], info_index);//Info about gate's value

            //Note: Sumbol S/D shorthand for Source/Drain.
            int pin1_info = get_info_node(MOS[i][2], info_index);//Last value of S/D 
            int pin1_update = get_info_node(MOS[i][2], update_index);//Value of S/D we are about to update
            int node1 = MOS[i][2];//Value of node.

            //Same as above, but now, for the other transtistor's pin.
            int pin2_info = get_info_node(MOS[i][3], info_index);
            int pin2_update = get_info_node(MOS[i][3], update_index);
            int node2 = MOS[i][3];
            //We can see above that, for information we use info_index and for update update_index. Keep it in mind.
            
            /*
                Until code's row 145 (Highlighted with "(1)"). We update the pin-node with value
                0 or 1 for VCC or GND respectively. Also the node we are about to
                update have to has the value X. Only the 'X' value-colour is allowed to be updated with 1 or 0.
                Afterwards, if the update has been done, we set that value to the respective pinX_update.
                That new value is going to be usefull for the upcoming algorythm's conditions.
            */
            if (node1 == VCC  &&  pin1_update == 'X' )
            {
                update_nodes_info(node1, update_index, 1);
                pin1_update = 1;
            }

            if (node2 == VCC  &&  pin2_update == 'X' )
            {
                update_nodes_info(node2, update_index, 1);
                pin2_update = 1;
            }

            if (node1 == GND &&  pin1_update == 'X')
            {
                update_nodes_info(node1, update_index, 0);
                pin1_update = 0;
            }

            if (node2 == GND && pin2_update == 'X')
            {
                update_nodes_info(node2, update_index, 0);
                pin2_update = 0;
            } /*(1)*/

            /* If transistor is PMOS with gate value = 0 or NMOS with gate value = 1, 
               then transfer the colour from one pin to other. */
            if ((MOS[i][0] == 'P' && gate == 0) || (MOS[i][0] == 'N' && gate == 1))
            {
                /* If we have set the output node with value Z and in the upcoming iterations
                   we find out that our transistor transfers the current between its pins,
                   we should restore that value with X.
                */
                if (is_node_output(node1) && pin1_update == 'Z')
                    pin1_update = 'X';

                if (is_node_output(node2) && pin2_update == 'Z')
                    pin2_update = 'X';

                /*Case '1' turns to '0'. The pin we are about to update has the value 1
                  and for the other pin we have the information about colour '0'.
                  Also, if the pin takes part in SC, the other pin-node takes part too. */
                if ((pin1_update == 1 && pin2_info == 0) || pin2_info == 'S')
                {
                    update_nodes_info(node1, update_index, 'S');
                    pin1_update = 'S';
                }

                //Symmetric with the above one, but for the other pin.
                if ((pin2_update == 1 && pin1_info == 0) || pin1_info == 'S')
                {
                    update_nodes_info(node2, update_index, 'S');
                    pin2_update = 'S';                    
                }
                /*We update normally because of transistor's transfer ability.
                  The value we update have to has colour 'X' and nothing else.*/
                if (pin1_update == 'X' && pin2_info != 'X' && pin2_info != 'S' && pin2_info != 'Z')
                    update_nodes_info(node1, update_index, pin2_info);
                else if (pin2_update == 'X' && pin1_info != 'X' && pin1_info != 'S' && pin1_info != 'Z')
                    update_nodes_info(node2, update_index, pin1_info);
            }
           
            if ((MOS[i][0] == 'P' && gate == 1) || (MOS[i][0] == 'N' && gate == 0))
            {   /*If the gate of PMOS transistor is 1 and the node is output
                 there is a posibility for OC at output.*/
                if (is_node_output(node1) && pin1_update == 'X')
                    update_nodes_info(node1, update_index, 'Z');

                if (is_node_output(node2) && pin2_update == 'X')
                    update_nodes_info(node2, update_index, 'Z');
            }
        }

        //After we done with the netlist we count converges.
        if (isGraphConverged(info_index, update_index))
            converges++;
        else
            converges = 0;

        info_index++;
        update_index++;
        /*Swap our 2 indicator pointers and discrease them by one
          so we won't be out of bounds.*/
        if (info_index == 4 || update_index == 4)
        {
            int swap = info_index;
            info_index = update_index;
            update_index = swap;
            info_index--;
            update_index--;
        }
    }

    print_results((info_index > update_index) ? info_index : update_index);
}

void create_graph()
{
    bool isNodeExists = false;

    int temp_buff[0xff]; // A HUGE, REALLY HUGE buffer :).
    nodes = 0;

    for (int i = 0; i < numOfMos; i++)//For every transistor in netlist.
    {
        for (int j = 2; j < 4; j++)//Their S/D pins are candidate nodes.
        {
            if (nodes == 0)//If there isn't nodes yet, store the first one and continue to the next loop.
            {
                temp_buff[nodes] = MOS[i][j];
                nodes++;
                continue;
            }

            for (int z = 0; z < nodes; z++) //Check if node already exists in the graph.
            {
                if (temp_buff[z] == MOS[i][j])
                {
                    isNodeExists = true;
                    break;
                }
            }

            if (!isNodeExists) //If node doesn't exist in the graph, then store it and count one more.
            {
                temp_buff[nodes] = MOS[i][j];
                nodes++;
            }
            isNodeExists = false;
        }
    }
    //Now we know the number of nodes, so, we can allocate memory properly.
    graph = (int **)malloc(sizeof(int) * 4);
    graph[0] = (int *)malloc(sizeof(int) * nodes);
    graph[1] = (int *)malloc(sizeof(int) * nodes);
    graph[2] = (int *)malloc(sizeof(int) * nodes);
    graph[3] = (int *)malloc(sizeof(int) * nodes);
    //From our HUGE buffer, store node's value and colour the initiate graph.
    for (int i = 0; i < nodes; i++)
    {
        graph[0][i] = temp_buff[i];
        if (graph[0][i] == VCC)
            graph[1][i] = 1;
        else if (graph[0][i] == GND)
            graph[1][i] = 0;
        else
            graph[1][i] = 'X';
    }

    // Of course we free that HUGE BUFFER :).
    free(temp_buff);
}

void run_testbench(FILE *fp)
{
    char *lineBuffer = NULL;
    bool is_test_end = false;//Helps restore the values TEST_IN and TEST_OUT if more than one testbenches exist.
    size_t len = 0;
    ssize_t read;
    int count_tests = 1; //Just for printing
    create_graph();
    
    
    /* Reads file line by line. Store the information for its section of file (## TEST_IN, ## TEST_OUT...).
       Calls simulate function and terminates when it read ## END_SIMULATION */
    while ((read = getline(&lineBuffer, &len, fp)) != -1)
    {
        if (strcmp(lineBuffer, "## END_SIMULATION\n") == 0)
            break;
        
        if(!is_test_end)
        {
            if (strcmp(lineBuffer, "## TEST_IN\n") == 0)
                update_tests_IOs(fp); //Store values-nodes from section "## TEST_IN"
            else if (strcmp(lineBuffer, "## TEST_VECTORS\n") == 0)
                update_test_vector(fp); //Store values-nodes from section "## TEST_VECTOR"            
            else if (strcmp(lineBuffer, "## SIMULATE\n") == 0)
            {
                simulate(); //Starts simulation.
                if(VECTOR != NULL)
                    free(VECTOR);
            }
            else if (strcmp(lineBuffer, "## END_TEST\n") == 0)
                is_test_end = true;
        }
        else if (strcmp(lineBuffer, "## TESTBENCH\n") == 0)
        {            
            if(TEST_IN != NULL)
                free(TEST_IN);

            if (TEST_OUT != NULL)
                free(TEST_OUT);

            count_tests++;
            is_test_end = false;
        }
    }
}

/* ===== BELOW ARE UTILITY FUNCTIONS ===== */

void update_nodes_info(int Node, int index, int value)
{
    for(int i=0; i<nodes; i++)
    {
        if(graph[0][i] == Node)
        {
            graph[index][i] = value;
            return;
        }
    }
}

int get_info_node(int N, int pos)
{
    int value = 'f'; //'f' from Fail. Just for check in case won't change.

    for (int i = 0; i < nodes; i++)
    {
        if (graph[0][i] == N)
        {
            value = graph[pos][i];
            break;
        }
    }

    return value;
}

int gate_value(int G, int pos)
{
    /* Checks if the gate is used as an input.
       If true, then returns the respective value from the Input Vector */
    for(int i=0; i < numOfTestIn; i++)
    {
        if(G == TEST_IN[i])
            return VECTOR[i];
    }
    int value = 'f'; //'f' from Fail. Just for check in case won't change.

    /* Otherwise, we need to check which node
       this gate represents and return the
       node's value-colour. */
    value = get_info_node(G, pos);

    return value;
}

void reset_graph()
{
    for(int i=0; i<nodes; i++)
    {
        graph[2][i] = 'X';
        graph[3][i] = 'X';
    }
}

bool is_node_output(int N)
{
    for (int i = 0; i < numOfOutputs; i++)
    {
        if (OUTPUT[i] == N)
            return true;
    }

    return false;
}

int isGraphConverged(int i, int j)
{
    for (int node = 0; node < nodes; node++)
    {
        if (graph[i][node] != graph[j][node])
            return 0;
    }
    
    return 1;
}

void update_test_vector(FILE *fp)
{
    char *lineBuffer = NULL;
    size_t len = 0;
    ssize_t read;

    read = getline(&lineBuffer, &len, fp);
    if(read == -1)
    {
        printf("Error: Read File.\n");
        exit(1);
    }

    lineBuffer[strlen(lineBuffer) - 1] = '\0';
    vector_len = call_strtok(lineBuffer, &VECTOR);
}

void update_tests_IOs(FILE *fp)
{
    char *lineBuffer = NULL;
    size_t len = 0;
    ssize_t read;

    read = getline(&lineBuffer, &len, fp);
    if (read == -1)
    {
        printf("Error: Read File.\n");
        exit(1);
    }
    lineBuffer[strlen(lineBuffer)-1] = '\0';
    numOfTestIn = call_strtok(lineBuffer, &TEST_IN);
    read = getline(&lineBuffer, &len, fp);

    if (strcmp(lineBuffer, "## TEST_OUT\n") != 0)
    {
        printf("Wrong file format.\n");
        exit(1);
    }

    read = getline(&lineBuffer, &len, fp);
    lineBuffer[strlen(lineBuffer) - 1] = '\0';

    numOfTestOut = call_strtok(lineBuffer, &TEST_OUT);
}

void store_circuit_infos(FILE *fp)
{    
    char *lineBuffer = NULL;
    size_t len = 0;
    ssize_t read;

    bool rails = false;
    bool inputs = false;
    bool outputs = false;
    bool netlist = false;

    char *token;
    char delims[] = "',', ';', ' '";

    while ((read = getline(&lineBuffer, &len, fp)) != -1)
    {
        if (strcmp(lineBuffer, "## TESTBENCH\n") == 0)
            break;

        lineBuffer[strlen(lineBuffer) - 1] = '\0'; //Strings end with char \0. Overwriting char '\n'.

        if (rails)
        {
            store_RAILS(lineBuffer);
            rails = false;
        }

        if (inputs)
        {
            numOfInputs = call_strtok(lineBuffer, &INPUT);
            inputs = false;
        }

        if (outputs)
        {
            numOfOutputs = call_strtok(lineBuffer, &OUTPUT);
            outputs = false;
        }

        if (netlist)
        {
            token = strtok(lineBuffer, delims);
            token = strtok(NULL, delims);

            if (numOfMos == 0)
            {
                MOS = (int **)malloc(sizeof(int) * 4);
                MOS[numOfMos] = (int *)malloc(sizeof(int) * 4);
            }
            else
            {
                MOS = (int **)realloc(MOS, sizeof(int) * 4 * (numOfMos + 1));
                MOS[numOfMos] = (int *)malloc(sizeof(int) * 4);
            }

            if (strcmp(token, "PMOS") == 0)
                MOS[numOfMos][0] = 'P';
            else if (strcmp(token, "NMOS") == 0)
                MOS[numOfMos][0] = 'N';
            else
            {
                printf("Error in section ## NETLIST.\n");
                printf("Transistor U%d: Wrong keyword for {PMOS/NMOS}.\n", numOfMos + 1);
                exit(1);
            }
            
            for (int i = 1; i <= 3; i++)
            {
                token = strtok(NULL, delims);
                if(token == NULL)
                {
                    printf("Error in section ## NETLIST.\n");
                    printf("Transistor U%d: GATE or SOURCE or DRAIN is missing.\n", numOfMos+1);
                    exit(1);
                }
                MOS[numOfMos][i] = atoi(token);
            }
            numOfMos++;
        }

        token = strtok(lineBuffer, delims);
        if (strcmp(token, "##") == 0)
        {
            token = strtok(NULL, delims);
            if (strcmp(token, "RAILS") == 0)
                rails = true;

            if (strcmp(token, "INPUTS") == 0)
                inputs = true;

            if (strcmp(token, "OUTPUTS") == 0)
                outputs = true;

            if (strcmp(token, "NETLIST") == 0)
                netlist = true;
        }
    }
}

void store_RAILS(char *buff)
{
    char *token1, *token2, *token3, *token4;
    const char delims[] = "';', ',', ' '";

    token1 = strtok(buff, delims);    
    token2 = strtok(NULL, delims);  
    if(token2 == NULL)
        return;  
    token3 = strtok(NULL, delims);
    token4 = strtok(NULL, delims);

    if (strlen(token1) > 3 || strlen(token3) > 3)
    {
        printf("ERROR: File's format: Section \"## RAILS\".\n");
        printf("Possible mistakes: \n");
        printf("1. No delimeter between {VCC/GND} and {Node Number}.\n");
        printf("2. Wrong keyword for {VCC/GND}\n");
        exit(1);
    }

    int node1 = atoi(token2);
    int node2 = atoi(token4);
    
    if (strcmp(token1, "VCC") == 0)
    {
        VCC = node1;
        GND = node2;
    }
    else if (strcmp(token1, "GND") == 0)
    {
        VCC = node2;
        GND = node1;
    }
    else
    {
        printf("ERROR: File's format: Section \"## RAILS\".\n");
        printf("Wrong keyword for {VCC/GND}\n");
        exit(1);
    }
    
}

int call_strtok(char *string, int **array)
{
    char *token;
    int count = 0;
    int buff[0xff];
    char delims[] = "',', ';', ' '";
    
    token = strtok(string, delims);
    if (token == NULL)
    {
        printf("Circuit's input or output doesn't exist.:(\n");
        exit(1);
    }

    buff[count] = atoi(token);
    count++;
    while ((token = strtok(NULL, delims)) != NULL)
    {
        buff[count] = atoi(token);
        count++;
    }

    *array = (int *)malloc(sizeof(int) * count);

    for (int i = 0; i < count; i++)
        (*array)[i] = buff[i];

    free(buff);

    return count;
}

void free_pointer()
{
    //Free Pointers.    
    if (INPUT != NULL)
        free(INPUT);

    if (OUTPUT != NULL)
        free(OUTPUT);
    
    if (TEST_IN != NULL)
        free(TEST_IN);

    if (TEST_OUT != NULL)
        free(TEST_OUT);
    
    for (int i = 0; i < numOfMos; i++)
    {
        if (MOS[i] != NULL)
            free(MOS[i]);
    }
    if (MOS != NULL)
        free(MOS);

    for (int i = 0; i < 4; i++)
    {
        if (graph[i] != NULL)
            free(graph[i]);
    }

    if (graph != NULL)
        free(graph);
}

void print_results(int x)
{
    FILE *fp = fopen(filename_out, "a");
    fprintf(fp, "Input Vector = <");

    for (int i = 0; i < vector_len; i++)
    {
        if (i != (vector_len - 1))
            fprintf(fp, "%d, ", VECTOR[i]);
        else
            fprintf(fp, "%d>\n", VECTOR[i]);
    }
    fprintf(fp, "NODE : Color\n");
    for (int i = 0; i < nodes; i++)
    {
        if (graph[0][i] == VCC)
            fprintf(fp, "%d : 1 VCC", graph[0][i]);
        else if (graph[0][i] == GND)
            fprintf(fp, "%d : 0 GND", graph[0][i]);
        else if (is_node_output(graph[0][i]) && graph[x][i] == 'X')
            fprintf(fp, "%d : Z", graph[0][i]);
        else if (graph[x][i] == 'X')
            fprintf(fp, "%d : %c", graph[0][i], graph[x][i]);
        else if (graph[x][i] == 'S')
            fprintf(fp, "%d : SC", graph[0][i]);
        else if (graph[x][i] == 'Z')        
            fprintf(fp, "%d : %c", graph[0][i], graph[x][i]);
        else 
            fprintf(fp, "%d : %d", graph[0][i], graph[x][i]);
        
        if(is_node_output(graph[0][i]))           
            fprintf(fp, " OUTPUT");

        fprintf(fp, "\n");
    }

    fprintf(fp, "\n");
}

void checkInput(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Incorrect input. Please try again :-).\n");
        usage();
        exit(1);
    }
    else
    {
        //Check if file exists.
        if (access(argv[1], F_OK) != 0)
        {
            printf("File doesn't exist. Unlucky :-(.\n");
            usage();
            exit(1);
        }
        printf("Simulating File \"%s\"... \n", argv[1]);
    }
}

void usage()
{
    printf("- Only one file is accepted and has to be in the same directory. Otherwise enter full path.\n");
    printf("- Via terminal type: ./File.exe File_Path_OR_Name.\n");
    printf("\tWhere File.exe is the executable and File_Path_OR_Name is the file's name or path you wish.\n");
    printf("- If you are using an IDE Platform (ex. CodeBlocks), you have to set the arguments from Preferences before you hit Run Button.\n");
}


void my_test()
{
    printf("VCC: %d\n", VCC);
    printf("GND: %d\n", GND);

    for(int i=0; i<numOfInputs; i++)
        printf("INPUT[%d] = %d\n", i, INPUT[i]);

    for (int i = 0; i < numOfOutputs; i++)
        printf("OUTPUT[%d] = %d\n", i, OUTPUT[i]);

    /* for(int i=0; i<numOfMos; i++)
    {
        for(int j=0; j<4; j++)
        {
            printf("%d ", MOS[i][j]);
        }

        printf("\n");
    }

    for (int i = 0; i < numOfTestIn; i++)
        printf("TEST_IN[%d] = %d\n", i, TEST_IN[i]);

    for (int i = 0; i < numOfTestOut; i++)
        printf("TEST_OUT[%d] = %d\n", i, TEST_OUT[i]); */

    getchar();
}