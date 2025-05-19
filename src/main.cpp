#include <format>
#include <iostream>
#include <random>
#include <vector>

#include <ale/ale_interface.hpp>
//#include <ale/games/supported/SpaceInvaders.hpp>

int main(void)
{
    const int ACTIONS = 6;
    int episodes;
    int total_reward;
    std::vector<int> q_table;

    ale::ALEInterface ale;
    ale::ActionVect legal_actions;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> rand_action(0,ACTIONS);

    ale.setInt("random_seed", 123);
    ale.loadROM("space_invaders.bin");

    legal_actions = ale.getLegalActionSet();

    episodes = 10;
    for(int i = 0; i < episodes ;i++)
    {
        total_reward = 0;

        while(!ale.game_over())
        {
            float reward;
            std::vector<unsigned char> rgb;
            ale::Action a;

            a = legal_actions[rand_action(gen)];
            reward = ale.act(a);
            ale.getScreenRGB(rgb);
            total_reward += reward;
        }

        ale.saveScreenPNG(std::format("episode-{}.png", i));
        std::cout << std::format("Episode {} score: {}", i, total_reward)
                  << std::endl;
        ale.reset_game();
    }

    return 0;
}
