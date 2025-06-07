#include <argp.h>

#include <algorithm>
#include <chrono>
#include <format>
#include <fstream>
#include <iostream>
#include <random>
#include <vector>

#include <ale/ale_interface.hpp>
#include <opencv4/opencv2/opencv.hpp>

#define STRINGIFY(x) STRINGIFY2(x)
#define STRINGIFY2(x) #x

// default values
#define EPISODES 10
#define NOOP 30
#define SKIP 2
// q-learning parameters
#define ALPHA 0.00025            // learning rate
#define GAMMA 0.99               // discount factor
#define EPSILON 1.0              // exploration rate (starting value)
#define EPSILON_MIN 0.1          // minimum exploration rate
#define EPSILON_DECAY 0.999999   // decay rate for exploration

const char *argp_program_version = "Version 0.1";
const char *argp_program_bug_address = "w@wltjr.com";

const char *CSV_FILE = "space_invaders_q_table.csv";

const int ACTIONS = 6;
const int HEIGHT = 210;
const int WIDTH = 160;
const int LEFT = 38;
const int RIGHT = 120;
const int CROP_X = 20;
const int CROP_Y = 30;
const int CROP_HEIGHT = 165;
const int CROP_WIDTH = 120;

// command line arguments
struct args
{
    bool display = false;
    bool game = false;
    bool load = false;
    bool png = false;
    bool save = false;
    bool sound = false;
    bool train = false;
    int episodes = EPISODES;
    int noop = NOOP;
    int skip = SKIP;
    float alpha = ALPHA;
    float gamma = GAMMA;
    float epsilon = EPSILON;
    float epsilon_min = EPSILON_MIN;
    float epsilon_decay = EPSILON_DECAY;
    std::string load_file = CSV_FILE;
    std::string save_file = CSV_FILE;
};

// help menu
static struct argp_option options[] = {
    {0,0,0,0,"Optional arguments:",1},
    {"audio",'a',0,0," Enable audio/sound ",1},
    {"display",'d',0,0," Enable display on screen ",1},
    {"episodes",'e',STRINGIFY(EPISODES),0," Number of episodes ",1},
    {"game",'g',0,0," Play game using q-table ",1},
    {"load",'l',CSV_FILE,OPTION_ARG_OPTIONAL," Load the q-table from file ",1},
    {"png",'p',0,0," Enable saving a PNG image per episode ",1},
    {"save",'s',CSV_FILE,OPTION_ARG_OPTIONAL," Save the q-table to file ",1},
    {"train",'t',0,0," Train the agent using q-learning ",1},
    {0,0,0,0,"Q-Learning parameters:",2},
    {"alpha",'A',STRINGIFY(ALPHA),0," Alpha learning rate",2},
    {"gamma",'G',STRINGIFY(GAMMA),0," Gamma learning rate discount factor",2},
    {"epsilon",'E',STRINGIFY(EPSILON),0," Epsilon exploration rate (starting value)",2},
    {"min",'M',STRINGIFY(EPSILON_MIN),0," Minimum exploration rate",2},
    {"decay",'D',STRINGIFY(EPSILON_DECAY),0," Decay rate for exploration",2},
    {"noop",'N',STRINGIFY(NOOP),0," Skip initial frames using noop action",2},
    {"skip",'S',STRINGIFY(SKIP),0," Skip frames and repeat actions",2},
    {0,0,0,0,"GNU Options:", 3},
    {0,0,0,0,0,0}
};

/**
 * @brief Parse command line options
 *
 * @param key the integer key provided by the current option to the option parser.
 * @param arg the name of an argument associated with this option
 * @param state points to a struct argp_state
 *
 * @return ARGP_ERR_UNKNOWN for any key value not recognize
 */
static error_t parse_opt(int key, char *arg, struct argp_state *state) {

    struct args *args = (struct args*)state->input;

    switch(key) {
        case 'a':
            args->sound = true;
            break;
        case 'd':
            args->display = true;
            break;
        case 'e':
            args->episodes = arg ? atoi (arg) : EPISODES;
            break;
        case 'g':
            args->game = true;
            break;
        case 'l':
            args->load = true;
            args->load_file = arg ? arg : CSV_FILE;
            break;
        case 'p':
            args->png = true;
            break;
        case 's':
            args->save = true;
            args->save_file = arg ? arg : CSV_FILE;
            break;
        case 't':
            args->train = true;
            break;
        case 'A':
            args->alpha = arg ? atof (arg) : ALPHA;
            break;
        case 'G':
            args->gamma = arg ? atof (arg) : GAMMA;
            break;
        case 'E':
            args->epsilon = arg ? atof (arg) : EPSILON;
            break;
        case 'M':
            args->epsilon_min = arg ? atof (arg) : EPSILON_MIN;
            break;
        case 'N':
            args->noop = arg ? atoi (arg) : NOOP;
            break;
        case 'D':
            args->epsilon_decay = arg ? atof (arg) : EPSILON_DECAY;
            break;
        case 'S':
            args->skip = arg ? atoi (arg) : SKIP;
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return(0);
}

#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
static struct argp argp	 =  { options, parse_opt };


/**
 * @brief Load the q-table from csv file
 * 
 * @param filename the file name of the csv file with q-table
 * @param q_table un-allocated q-table
 */
void load_q_table(std::string filename, 
                  std::vector<std::vector<float>> &q_table)
{
    std::ifstream file;
    std::string line;

    file.open(filename);

    if(!file.is_open())
    {
        std::cerr << "Unable to open: " << CSV_FILE << std::endl;
        return;
    }

    std::getline(file, line); // skip first line
    while(std::getline(file, line))
    {
        std::string value;
        std::istringstream ss(std::move(line));
        std::vector<float> row;

        row.reserve(ACTIONS);
        std::getline(ss, value, ','); // skip first value
        while(std::getline(ss, value, ','))
            row.push_back(std::atof(std::move(value).c_str()));

        q_table.push_back(std::move(row));
    }
    file.close();
}


/**
 * @brief Save the q-table to csv file
 * 
 * @param filename the file name of the csv file to save q-table
 * @param q_table q-table of actions for each cannon_x value
 */
void save_q_table(std::string filename, 
                  std::vector<std::vector<float>> &q_table)
{
    std::ofstream file;

    file.open(filename);
    file << "cannon_x,0-Noop,1-Fire,2-Right,3-Left,4-RightFire,5-LeftFire\n";
    for(int r = 0; r < WIDTH; r++)
    {
        file << r << ",";
        for(int c = 0; c < ACTIONS;)
        {
            file << q_table[r][c];
            if(++c < ACTIONS)
                file << ",";
        }
        file << std::endl;
    }
    file.close();
}


/**
 * @brief Convert csv/q-table column to ALE action
 * 
 * @param col column number
 * 
 * @return ale::Action ALE action
 */
ale::Action col_to_action(int col)
{
    ale::Action a;

    if(col == 2)
        a = ale::Action::PLAYER_A_RIGHT;
    else if(col == 3)
        a = ale::Action::PLAYER_A_LEFT;
    else if(col == 4)
        a = ale::Action::PLAYER_A_RIGHTFIRE;
    else if(col == 5)
        a = ale::Action::PLAYER_A_LEFTFIRE;
    else
        a = static_cast<ale::Action>(col);

    return a;
}


/**
 * @brief Train agent using q-learning, generates q-table
 * 
 * @param args reference to args structure
 * @param ale reference to arcade learning environment
 * @param q_table reference to allocated q-table
 */
void train(args &args, 
           ale::ALEInterface &ale,
           std::vector<std::vector<float>> &q_table)
{
    int max_episode;
    ale::reward_t max_score;
    cv::Mat cannon;

    // initialize random device
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> rand_action(0, ACTIONS-1);
    std::uniform_real_distribution<> rand_epsilon(0.0, args.epsilon);

    auto start = std::chrono::high_resolution_clock::now();

    // load opencv template image
    cv::cvtColor(cv::imread("templates/cannon.png"), cannon, cv::COLOR_RGB2GRAY);

    max_episode = -1;
    max_score = -1;

    for(int i = 0; i < args.episodes ;i++)
    {
        ale::reward_t total_reward;
        int steps;
        int lives;

        lives = ale.lives();
        steps = 0;
        total_reward = 0;

        if(args.train)
        {
            // skip initial frames with noop action
            for(int i = 0; i < args.noop; steps++, i++)
                ale.act(ale::Action::PLAYER_A_NOOP);
        }

        for(; !ale.game_over(); steps++)
        {
            int a;
            int next_a;
            int cannon_x;
            int next_x;
            double max_value;
            double min_value;
            ale::reward_t reward;
            std::vector<unsigned char> screen;
            std::vector<float>::iterator max;
            ale::Action action;
            cv::Mat orig;
            cv::Mat result;
            cv::Point max_location;
            cv::Point min_location;

            // prepare current game screen for opencv
            ale.getScreenGrayscale(screen);
            orig = cv::Mat(HEIGHT, WIDTH, CV_8UC1, &screen[0]);
            orig = cv::Mat(orig, cv::Rect(CROP_X, CROP_Y, CROP_WIDTH, CROP_HEIGHT));

            // match cannon template in screen
            result.create(CROP_HEIGHT, CROP_WIDTH, CV_8UC1);
            cv::matchTemplate(orig, cannon, result, cv::TM_CCOEFF_NORMED);
            normalize( result, result, 0, 255, cv::NORM_MINMAX, CV_8UC1);

            // get cannon location
            cv::minMaxLoc(result, &min_value, &max_value, &min_location, &max_location);
            cannon_x = max_location.x + (cannon.cols + 1) / 2 + 20; // + 20 for cropping

            // fix to bounds, prevent values outside
            if(cannon_x < LEFT)
                cannon_x = LEFT;
            if(cannon_x > RIGHT)
                cannon_x = RIGHT;

            // default action to max from q-table
            max = std::max_element(q_table[cannon_x].begin(), q_table[cannon_x].end());
            a = std::distance(q_table[cannon_x].begin(), max);

            // random action if empty q-table or training
            if((a == 0 && q_table[cannon_x][0] == 0) ||
               (args.train && rand_epsilon(gen) < args.epsilon))
                a = rand_action(gen);

            // take action & collect reward
            action = col_to_action(a);
            reward = ale.act(action);
            total_reward += reward;

            if(args.train)
            {
                // normalize reward -1, 0, or 1
                if(reward > 0)
                    reward = 1;

                // skip k frames, repeat action
                for(int k = 0; k < args.skip; steps++, k++)
                    total_reward += ale.act(action);

                // penalty for dying
                if(ale.lives() < lives)
                {
                    reward -= 1;
                    lives = ale.lives();
                }
                // penalty for noop
                else if(a == 0)
                    reward -= 1;

                next_x = cannon_x;

                // move to next state based on action
                if(a == 2 || a == 4 || cannon_x == LEFT)
                    next_x++;
                else if (a == 3 || a == 5 || cannon_x == RIGHT)
                    next_x--;

                // update q-value
                max = std::max_element(q_table[next_x].begin(), q_table[next_x].end());
                next_a = std::distance(q_table[next_x].begin(), max);
                q_table[cannon_x][a] += args.alpha *
                    (reward + args.gamma * q_table[next_x][next_a] - q_table[cannon_x][a]);

                // decay epsilon
                args.epsilon = std::max(args.epsilon_min, 
                                        args.epsilon * args.epsilon_decay);
            }
        }

        // track max episode & score
        if(total_reward > max_score)
        {
            max_episode = i;
            max_score = total_reward;
        }

        // save final episode results to file
        if(args.png)
            ale.saveScreenPNG(std::format("episode-{}.png", i));

        std::cout << std::format("Episode {} score: {} steps: {} epsilon: {}",
                                 i, total_reward, steps, args.epsilon)
                  << std::endl;
        ale.reset_game();
    }

    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(stop - start);

    std::cout << std::endl
              << std::format("Elapsed Time: {}s - Episode {} Max Score: {}",
                             duration.count(), max_episode, max_score)
              << std::endl;
}


int main(int argc, char* argv[])
{
    struct args args;
    std::vector<std::vector<float>> q_table;

    // parse command line options
    argp_parse (&argp, argc, argv, 0, 0, &args);

    // initialize Arcade Learning Environment
    ale::ALEInterface ale;

    // initialize game
    ale.setInt("random_seed", 123);
    ale.setBool("display_screen", args.display);
    ale.setBool("sound", args.sound);
    ale.loadROM("./rom/space_invaders.bin");

    // load q-table from csv file
    if(args.load)
        load_q_table(args.load_file, q_table);

    // allocate q-table if empty
    if(q_table.size() == 0)
        q_table.resize(WIDTH, std::vector<float>(ACTIONS, 0));

    // must load or train
    if(!args.load && !args.train)
        args.train = true;

    // enable q-learning training
    if(args.train)
    {
        std::cout << "Training Parameters:" << std::endl
                  << "Episodes:      " << args.episodes << std::endl
                  << "Alpha:         " << args.alpha << std::endl
                  << "Gamma:         " << args.gamma << std::endl
                  << "Epsilon:       " << args.epsilon << std::endl
                  << "Epsilon Min:   " << args.epsilon_min << std::endl
                  << "Epsilon Decay: " << args.epsilon_decay << std::endl
                  << "Noop:          " << args.noop << std::endl
                  << "Frame Skip:    " << args.skip << std::endl;

        train(args, ale, q_table);

        // only save after training
        if(args.save)
            save_q_table(args.save_file, q_table);
    }

    // play game using trained q-table, random actions if empty
    if(args.game)
    {
        args.train = false;
        train(args, ale, q_table);
    }

    return 0;
}
