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

const char *argp_program_version = "Version 0.1";
const char *argp_program_bug_address = "w@wltjr.com";

const char *CSV_FILE = "space_invaders_q_table.csv";

const int ACTIONS = 4;
const int HEIGHT = 210;
const int WIDTH = 160;

// command line arguments
struct args
{
    bool display;
    bool game;
    bool load;
    bool png;
    bool save;
    bool sound;
    bool train;
    int episodes;
};

// help menu
static struct argp_option options[] = {
    {0,0,0,0,"Optional arguments:",1},
    {"audio",'a',0,0," Enable audio/sound ",1},
    {"display",'d',0,0," Enable display on screen ",1},
    {"episodes",'e',"10",0," Number of episodes default 10 ",1},
    {"game",'g',0,0," Play game using q-table ",1},
    {"load",'l',0,0," Load the q-table from file ",1},
    {"png",'p',0,0," Enable saving a PNG image per episode ",1},
    {"save",'s',0,0," Save the q-table to file ",1},
    {"train",'t',0,0," Train the agent using q-learning ",1},
    {0,0,0,0,"GNU Options:", 2},
    {0,0,0,0,0,0}
};

/**
 * Parse command line options
 *
 * @key the integer key provided by the current option to the option parser.
 * @arg the name of an argument associated with this option
 * @state points to a struct argp_state
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
            args->episodes = arg ? atoi (arg) : 10;
            break;
        case 'g':
            args->game = true;
            break;
        case 'l':
            args->load = true;
            break;
        case 'p':
            args->png = true;
            break;
        case 's':
            args->save = true;
            break;
        case 't':
            args->train = true;
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
 * @param q_table un-allocated q-table
 */
void load_q_table(std::vector<std::vector<float>> &q_table)
{
    std::ifstream file;
    std::string line;

    file.open(CSV_FILE);

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
 * @param q_table q-table of actions for each cannon_x value
 */
void save_q_table(std::vector<std::vector<float>> &q_table)
{
    std::ofstream file;

    file.open(CSV_FILE);
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
    float alpha;
    float gamma;
    float epsilon;
    float epsilon_min;
    float epsilon_decay;
    ale::ActionVect legal_actions;
    cv::Mat cannon;

    // initialize random device
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> rand_action(0,ACTIONS-1);
    std::uniform_real_distribution<> rand_epsilon(0.0,1.0);

    auto start = std::chrono::high_resolution_clock::now();

    legal_actions = ale.getLegalActionSet();

    // load opencv template image
    cv::cvtColor(cv::imread("templates/cannon.png"), cannon, cv::COLOR_RGB2GRAY);

    // q-learning parameters
    alpha = 0.2;                // learning rate
    gamma = 0.96;               // discount factor
    epsilon = 1.0;              // exploration rate (starting value)
    epsilon_min = 0.1;          // minimum exploration rate
    epsilon_decay = 0.995;      // decay rate for exploration

    max_episode = -1;
    max_score = -1;

    for(int i = 0; i < args.episodes ;i++)
    {
        ale::reward_t total_reward;

        total_reward = 0;

        while(!ale.game_over())
        {
            int cannon_x;
            int next_q_value;
            double max_value;
            double min_value;
            ale::reward_t reward;
            std::vector<unsigned char> screen;
            std::vector<float>::iterator max;
            ale::Action a;
            cv::Mat orig;
            cv::Mat result;
            cv::Point max_location;
            cv::Point min_location;

            // prepare current game screen for opencv
            ale.getScreenGrayscale(screen);
            orig = cv::Mat(HEIGHT, WIDTH, CV_8UC1, &screen[0]);

            // match cannon template in screen
            result.create(HEIGHT, WIDTH, CV_8UC1);
            cv::matchTemplate(orig, cannon, result, cv::TM_CCOEFF_NORMED);
            normalize( result, result, 0, 255, cv::NORM_MINMAX, CV_8UC1);

            // get cannon location
            cv::minMaxLoc(result, &min_value, &max_value, &min_location, &max_location);
            cannon_x = max_location.x + (cannon.cols + 1) / 2;

            // default action to max from q-table
            max = std::max_element(q_table[cannon_x].begin(), q_table[cannon_x].end());
            a = legal_actions[std::distance(q_table[cannon_x].begin(), max)];

            // random action if empty q-table or training
            if((a == 0 && q_table[cannon_x][0] == 0) ||
               (args.train && rand_epsilon(gen) < epsilon))
                a = legal_actions[rand_action(gen)];

            // take action & collect reward
            reward = ale.act(a);
            total_reward += reward;

            if(args.train)
            {
                // update q-value
                max = std::max_element(q_table[cannon_x].begin(), q_table[cannon_x].end());
                next_q_value = legal_actions[std::distance(q_table[cannon_x].begin(), max)];
                q_table[cannon_x][a] += alpha * (reward + gamma * next_q_value - q_table[cannon_x][a]);

                // decay epsilon
                epsilon = std::max(epsilon_min, epsilon * epsilon_decay);
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

        std::cout << std::format("Episode {} score: {}", i, total_reward)
                  << std::endl;
        ale.reset_game();
    }
    std::cout << std::format("Episode {} max score: {}", max_episode, max_score)
                << std::endl;

    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(stop - start);
    std::cout << "Elapsed time: " << duration.count() << "s" << std::endl;
}


int main(int argc, char* argv[])
{
    struct args args;
    std::vector<std::vector<float>> q_table;

    // initialize Arcade Learning Environment
    ale::ALEInterface ale;

    // default arguments
    args.episodes = 10;
    args.display = false;
    args.game = false;
    args.load = false;
    args.sound = false;
    args.png = false;
    args.save = false;
    args.train = false;

    // parse command line options
    argp_parse (&argp, argc, argv, 0, 0, &args);

    // initialize game
    ale.setInt("random_seed", 123);
    ale.setBool("display_screen", args.display);
    ale.setBool("sound", args.sound);
    ale.loadROM("space_invaders.bin");

    if(args.load)
        load_q_table(q_table);

    if(q_table.size() == 0)
        q_table.resize(WIDTH, std::vector<float>(ACTIONS, 0));

    // must load or train
    if(!args.load && !args.train)
        args.train = true;

    // enable q-learning training
    if(args.train)
    {
        train(args, ale, q_table);

        // only save after training
        if(args.save)
            save_q_table(q_table);
    }

    if(args.game)
    {
        args.train = false;
        train(args, ale, q_table);
    }

    return 0;
}
