/*
    Author: Mylonakis Emmanouil
    Registration Number: 2015030079
    Date: 20/03/2021
    Operating System: Windows 10.

    Description: Exercise 2. A simple simulator similar to RSIM.
                             Simulating CMOS transistor's behavior in digital level
                             by utilizing a mathematical model, known as Colour Graphing.                             
*/

/* NOTE: In order to help you read the code comfortably,
        functions are implemented by evaluating its importance
        in asceding order with bottom-up logic.
        Less important => further on bottom.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

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
    
    //Create and clear output file.
    sprintf(filename_out, "Result_%s", argv[1]);
    FILE *fp_out = fopen(filename_out, "w");
    fclose(fp_out);
    //Store circuit's information.
    store_circuit_infos(fp); 
    //Run Testbench.
    run_testbench(fp);
    //Release memory.
    free_pointer();
    fclose(fp);
    return 0;
}
/* ========== BELOW FUNCTIONS ARE FOR MAIN FUNCTIONALITY ========== */
void simulate()
{
    int info_index = 1;   //From which graph we take the information about node.
    int update_index = 2; //In whicn graph we update the new information.
    int converges = 0;    //To terminate when graph has converge.
    //First of all, we are using Graph[1] (the initial values)
    reset_graph(); //Reset graph for new simulation.

    while (converges < 2) // 2 stable states for the graph are enough to have convergence.
    {
        for (int i = 0; i < numOfMos; i++) //We are crossing netlist from top->bottom.
        {
            int gate = gate_value(MOS[i][1], info_index); //Info about gate's value

            //Note: Sumbol S/D shorthand for Source/Drain.
            int pin1_info = get_info_node(MOS[i][2], info_index);     //Last value of S/D
            int pin1_update = get_info_node(MOS[i][2], update_index); //Value of S/D we are about to update
            int node1 = MOS[i][2];                                    //Value of node.

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
            if (node1 == VCC && pin1_update == 'X')
            {
                update_nodes_info(node1, update_index, 1);
                pin1_update = 1;
            }

            if (node2 == VCC && pin2_update == 'X')
            {
                update_nodes_info(node2, update_index, 1);
                pin2_update = 1;
            }

            if (node1 == GND && pin1_update == 'X')
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
            { /*If the gate of PMOS transistor is 1 and the node is output
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

    for (int i = 0; i < numOfMos; i++) //For every transistor in netlist.
    {
        for (int j = 2; j < 4; j++) //Their S/D pins are candidate nodes.
        {
            if (nodes == 0) //If there isn't nodes yet, store the first one and continue to the next loop.
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
    bool is_test_end = false; //Helps restore the values TEST_IN and TEST_OUT if more than one testbenches exist.
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

        if (!is_test_end)
        {
            if (strcmp(lineBuffer, "## TEST_IN\n") == 0)
                update_tests_IOs(fp); //Store values-nodes from section "## TEST_IN"
            else if (strcmp(lineBuffer, "## TEST_VECTORS\n") == 0)
                update_test_vector(fp); //Store values-nodes from section "## TEST_VECTOR"
            else if (strcmp(lineBuffer, "## SIMULATE\n") == 0)
            {
                simulate(); //Starts simulation.
                if (VECTOR != NULL)
                    free(VECTOR);
            }
            else if (strcmp(lineBuffer, "## END_TEST\n") == 0)
                is_test_end = true;
        }
        else if (strcmp(lineBuffer, "## TESTBENCH\n") == 0)
        {
            if (TEST_IN != NULL)
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
    for (int i = 0; i < nodes; i++)
    {
        if (graph[0][i] == Node)
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
    for (int i = 0; i < numOfTestIn; i++)
    {
        if (G == TEST_IN[i])
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
    for (int i = 0; i < nodes; i++)
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
    if (read == -1)
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
    lineBuffer[strlen(lineBuffer) - 1] = '\0';
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
                if (token == NULL)
                {
                    printf("Error in section ## NETLIST.\n");
                    printf("Transistor U%d: GATE or SOURCE or DRAIN is missing.\n", numOfMos + 1);
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
    if (token2 == NULL)
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

        if (is_node_output(graph[0][i]))
            fprintf(fp, " OUTPUT");

        fprintf(fp, "\n");
    }

    fprintf(fp, "\n");

    fclose(fp);
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

    for (int i = 0; i < numOfInputs; i++)
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