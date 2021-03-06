//
// Created by Кирилл Голубев on 01/02/2017.
//

#include <algorithm>
#include <cstdlib>
#include "pred_brain.h"
#include "../unit/unit_params.h"
#include "../place/place.h"
#include "../sell/sell.h"
#include "pred_params.h"
#include <iostream>
#include <map>
#include <tuple>


pred_brain::pred_brain() : mind(init_mind()) {
    this->interpret = {
            {0, std::make_tuple(0, 1)},
            {1, std::make_tuple(1, 0)},
            {2, std::make_tuple(-1, 0)},
            {3, std::make_tuple(0, -1)},
            {4, std::make_tuple(1, 1)},
            {5, std::make_tuple(-1, -1)},
            {6, std::make_tuple(-1, 1)},
            {7, std::make_tuple(1, -1)},
            {8, std::make_tuple(2, 0)}
    };
    this->splited = false;
    this->reward = 0;
    this->prev_fat = 0;
}

void pred_brain::set_speed() {
    int N = data->view_range * data->view_range;
    int x0 = data->view_range / 2, y0 = x0;
    int *move_options = new int[N];
    pred_params *pointer = dynamic_cast<pred_params *>(data);
    for (int i = 0; i < N; i++)
        move_options[i] = N;
    for (int i = 0; i < N; i++)
        if (pointer->_view(i)->guest != nullptr)
            if (dynamic_cast<sell *>(pointer->_view(i)->guest) != nullptr)
                move_options[i] = std::max(abs(i % (2 * x0 + 1) - x0), abs(i / (2 * y0 + 1) - y0));
    int p = N;
    for (int i = 0; i < N; i++)
        if (move_options[i] < p)
            p = move_options[i];
    if (p == N) {
        delete[] move_options;
        data->speed[0] = rand() % 3 - 1;
        data->speed[1] = rand() % 3 - 1;
        return;
    }
    for (int i = 0; i < N; i++) {
        if (move_options[i] > p)
            move_options[i] = N;
        else if (move_options[i] == p)
            move_options[i] = i;
    }
    std::random_shuffle(move_options, move_options + N);
    int i;
    for (i = 0; move_options[i] == N; i++);
    i = move_options[i];
    data->speed[0] = i % (2 * x0 + 1) - x0;
    data->speed[1] = i / (2 * x0 + 1) - y0;
    delete[] move_options;
}

void pred_brain::set_speed_using_brain() {
    //const clock_t update_time = clock();
    this->brain_update();
    //std::cout<<"brain update time: "<<float(clock() - update_time) / CLOCKS_PER_SEC<<std::endl;

    //const clock_t increment_time = clock();
    this->mind.apply_param_increment();
    //std::cout<<"increment_time: "<<float(clock() - increment_time) / CLOCKS_PER_SEC<<std::endl;
    //const clock_t observation_time = clock();
    auto obs = observation();
    //std::cout<<"observation_time: "<<float(clock() - observation_time) / CLOCKS_PER_SEC<<std::endl;
    this->prev_fat = dynamic_cast<pred_params *>(this->data)->fat;

    //const clock_t process_time = clock();
    int action = mind.process(obs);
    //std::cout<<"process_time: "<<float(clock() - process_time) / CLOCKS_PER_SEC<<std::endl;
    //std::cout<<"-----------------------------"<<std::endl;
    std::tuple<int, int> inter = this->interpret[action];
    if (std::get<0>(inter) != 2) {
        this->data->speed[0] = std::get<0>(inter);
        this->data->speed[1] = std::get<1>(inter);
    }
    else {
        this->try_to_split();
    }

}

Brain::Input_layer *pred_brain::init_mind() {
    int view_range = 5*5;
    Brain::Input_layer *net = new Brain::Input_layer(view_range + 1);
    Brain::SigmoidLayer *hiddn_layer1 = new Brain::SigmoidReccurentLayer(net, view_range);
    Brain::SigmoidLayer *hiddn_layer2 = new Brain::SigmoidLayer(net, 9);
    return net;
}

Eigen::VectorXd pred_brain::observation() {
    const int N = this->data->view_range * this->data->view_range;
    Eigen::VectorXd observation(N + 1);
    pred_params *pointer = dynamic_cast<pred_params *>(this->data);
    for (int i = 0; i < N; i++) {
        if (pointer->_view(i)->guest != nullptr)
            if (dynamic_cast<sell *>(pointer->_view(i)->guest) != nullptr)
                observation(i) = 1;
            else
                observation(i) = 2;
        else
            observation(i) = 0;
    }
    observation(N) = dynamic_cast<pred_params*>(this->data)->fat;
    return observation;
}

void pred_brain::brain_update() {
    this->form_reward();
    this->mind.update_params(reward);
    this->reward = 0;
    this->splited = false;
    dynamic_cast<pred_params*>(this->data)->want_to_split = false;
}

void pred_brain::form_reward() {
    if (splited)
        reward += 1;
    if (prev_fat < dynamic_cast<pred_params *>(this->data)->fat)
        reward += 1;
    else if(prev_fat > dynamic_cast<pred_params *>(this->data)->fat)
        reward += -1;
    reward += -0.2;

}

void pred_brain::try_to_split() {
    dynamic_cast<pred_params*>(this->data)->want_to_split = true;
}
