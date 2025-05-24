#include <argp.h>

#include <algorithm>
#include <format>
#include <iostream>
#include <random>
#include <vector>

#include <ale/ale_interface.hpp>
#include <opencv4/opencv2/opencv.hpp>

const char *argp_program_version = "Version 0.1";
const char *argp_program_bug_address = "w@wltjr.com";

// command line arguments
struct args
{
    bool display;
    bool sound;
    bool png;
    int episodes;
};

// help menu
static struct argp_option options[] = {
    {0,0,0,0,"Optional arguments:",1},
    {"audio",'a',0,0," Enable audio/sound ",1},
    {"display",'d',0,0," Enable display on screen ",1},
    {"episodes",'e',"10",0," Number of episodes default 10 ",1},
    {"png",'p',0,0," Enable saving a PNG image per episode ",1},
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
        case 'p':
            args->png = true;
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return(0);
}

#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
static struct argp argp	 =  { options, parse_opt };

int main(int argc, char* argv[])
{
    const int ACTIONS = 6;
    const int HEIGHT = 210;
    const int WIDTH = 160;
    int max_episode;
    ale::reward_t max_score;
    float alpha;
    float gamma;
    float epsilon;
    float epsilon_min;
    float epsilon_decay;
    struct args args;
    std::vector<std::vector<int>> q_table;

    // initialize Arcade Learning Environment
    ale::ALEInterface ale;
    ale::ActionVect legal_actions;

    // opencv
    cv::Mat cannon;

    // initialize random device
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> rand_action(0,ACTIONS-1);
    std::uniform_real_distribution<> rand_epsilon(0.0,1.0);

    // default arguments
    args.episodes = 10;
    args.display = false;
    args.sound = false;
    args.png = false;

    // parse command line options
    argp_parse (&argp, argc, argv, 0, 0, &args);

    // initialize game
    ale.setInt("random_seed", 123);
    ale.setBool("display_screen", args.display);
    ale.setBool("sound", args.sound);
    ale.loadROM("space_invaders.bin");

    legal_actions = ale.getLegalActionSet();
    q_table.resize(WIDTH, std::vector<int>(ACTIONS, 0));

    // load opencv template images
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
            std::vector<int>::iterator max;
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

            if(rand_epsilon(gen) < epsilon)
                a = legal_actions[rand_action(gen)];
            else
            {
                max = std::max_element(q_table[cannon_x].begin(), q_table[cannon_x].end());
                a = legal_actions[std::distance(q_table[cannon_x].begin(), max)];
            }

            // take action & collect reward
            reward = ale.act(a);
            total_reward += reward;

            // update q-value
            max = std::max_element(q_table[cannon_x].begin(), q_table[cannon_x].end());
            next_q_value = legal_actions[std::distance(q_table[cannon_x].begin(), max)];
            q_table[cannon_x][a] += alpha * (reward + gamma * next_q_value - q_table[cannon_x][a]);

            // decay epsilon
            epsilon = std::max(epsilon_min, epsilon * epsilon_decay);
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

    return 0;
}
